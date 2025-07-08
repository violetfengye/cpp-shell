/**
 * @file job_control.cpp
 * @brief Job control management implementation
 */

#include "job/job_control.h"
#include "job/bg_job_adapter.h"
#include "core/shell.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>
#include <mutex>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace dash
{
    // Add a mutex for thread-safe job control updates
    static std::mutex job_update_mutex;

    JobControl::JobControl(Shell *shell)
        : shell_(shell), bg_job_adapter_(nullptr)
    {
        bg_job_adapter_ = new BGJobAdapter(shell, this);
    }

    JobControl::~JobControl()
    {
        // Cleanup background adapter
        if (bg_job_adapter_)
        {
            delete bg_job_adapter_;
            bg_job_adapter_ = nullptr;
        }

        // Cleanup job list
        for (auto it = jobs_.begin(); it != jobs_.end(); ++it)
        {
            delete *it;
        }
        jobs_.clear();
    }

    bool JobControl::initialize()
    {
        if (!bg_job_adapter_)
        {
            std::cerr << "Background job adapter not initialized" << std::endl;
            return false;
        }

        return bg_job_adapter_->initialize();
    }

    Job* JobControl::createJob(const std::string &command)
    {
        if (!bg_job_adapter_)
        {
            std::cerr << "Background job adapter not initialized" << std::endl;
            return nullptr;
        }

        // Create a new Job object
        Job *job = new Job(command);
        if (!job)
        {
            std::cerr << "Failed to create job object" << std::endl;
            return nullptr;
        }

        // Create underlying job structure
        struct job *bg_job = bg_job_adapter_->createJob(command, 1);
        if (!bg_job)
        {
            std::cerr << "Failed to create underlying job structure" << std::endl;
            delete job;
            return nullptr;
        }

        // Set Job object properties
        job->setJobId(jobno(bg_job));
        job->setBgJob(bg_job);

        // Add to job list
        jobs_.push_back(job);

        return job;
    }

    int JobControl::runInBackground(Job* job, const std::string& cmd, char *const argv[])
    {
        if (!bg_job_adapter_ || !job || !job->getBgJob())
        {
            std::cerr << "Invalid job" << std::endl;
            return -1;
        }

        // Set job state to running
        job->setState(JobState::RUNNING);
        
        // Call underlying API to run in background
        int result = bg_job_adapter_->runInBackground(job->getBgJob(), cmd, argv);
        if (result < 0)
        {
            std::cerr << "Failed to run background job" << std::endl;
            job->setState(JobState::DONE);
            return -1;
        }

        // Get the process ID from the bg_job
        if (job->getBgJob() && job->getBgJob()->ps && job->getBgJob()->nprocs > 0)
        {
            job->setPid(job->getBgJob()->ps[0].pid);
        }

        std::cout << "[" << job->getJobId() << "] " << job->getPid() << std::endl;
        return 0;
    }

    int JobControl::runInForeground(Job* job, const std::string& cmd, char *const argv[])
    {
        if (!bg_job_adapter_ || !job || !job->getBgJob())
        {
            std::cerr << "Invalid job" << std::endl;
            return -1;
        }

        // Set job state to running
        job->setState(JobState::RUNNING);
        
        // Call underlying API to run in foreground
        int result = bg_job_adapter_->runInForeground(job->getBgJob(), cmd, argv);
        
        // Update job state based on return value
        updateJobState(job);
        
        return result;
    }

    int JobControl::waitForJob(Job* job)
    {
        if (!bg_job_adapter_ || !job || !job->getBgJob())
        {
            std::cerr << "Invalid job" << std::endl;
            return -1;
        }

        int result = bg_job_adapter_->waitForJob(job->getBgJob());
        
        // Update job state
        updateJobState(job);
        
        return result;
    }

    int JobControl::waitForJobWithPid(pid_t pid)
    {
        // Find the job with this PID
        Job* job = getJobByPid(pid);
        if (job) {
            return waitForJob(job);
        }
        
        // If we don't have the job object, just wait directly using waitpid
        int status;
        pid_t result = waitpid(pid, &status, WUNTRACED);
        if (result == -1) {
            perror("waitpid");
            return -1;
        }
        
        // Return appropriate status
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            return 128 + WTERMSIG(status);
        }
        
        return 0;
    }

    void JobControl::updateJobState(Job* job)
    {
        std::lock_guard<std::mutex> lock(job_update_mutex);
        
        if (!bg_job_adapter_ || !job || !job->getBgJob())
        {
            return;
        }

        struct job* bg_job = job->getBgJob();
        int status = bg_job_adapter_->getJobStatus(bg_job);

        // Update job state
        if (bg_job->state == JOBSTOPPED)
        {
            job->setState(JobState::STOPPED);
        }
        else if (bg_job->state == JOBDONE)
        {
            job->setState(JobState::DONE);
            
            // Set exit status
            if (WIFEXITED(status))
            {
                job->setExitStatus(WEXITSTATUS(status));
            }
            else if (WIFSIGNALED(status))
            {
                job->setSignaled(true);
                job->setTermSignal(WTERMSIG(status));
            }
        }
        else if (bg_job->state == JOBRUNNING)
        {
            job->setState(JobState::RUNNING);
        }
        
        // Update process information
        if (bg_job->nprocs > 0) {
            for (int i = 0; i < bg_job->nprocs; i++) {
                Process* process = job->getProcessByIndex(i);
                if (!process) {
                    // Create new process if it doesn't exist
                    process = new Process();
                    job->addProcess(process);
                }
                
                process->setPid(bg_job->ps[i].pid);
                process->setStatus(bg_job->ps[i].status);
                
                // Update process state
                if (WIFEXITED(bg_job->ps[i].status)) {
                    process->setExitStatus(WEXITSTATUS(bg_job->ps[i].status));
                    process->setState(ProcessState::DONE);
                } else if (WIFSIGNALED(bg_job->ps[i].status)) {
                    process->setTermSignal(WTERMSIG(bg_job->ps[i].status));
                    process->setState(ProcessState::SIGNALED);
                } else if (WIFSTOPPED(bg_job->ps[i].status)) {
                    process->setStopSignal(WSTOPSIG(bg_job->ps[i].status));
                    process->setState(ProcessState::STOPPED);
                } else {
                    process->setState(ProcessState::RUNNING);
                }
            }
        }
    }

    void JobControl::updateAllJobStates()
    {
        std::lock_guard<std::mutex> lock(job_update_mutex);
        
        for (auto& job : jobs_) {
            updateJobState(job);
        }
    }

    int JobControl::fgCommand(int argc, char** argv)
    {
        if (!bg_job_adapter_)
        {
            std::cerr << "Background job adapter not initialized" << std::endl;
            return -1;
        }

        return bg_job_adapter_->fgCommand(argc, argv);
    }

    int JobControl::bgCommand(int argc, char** argv)
    {
        if (!bg_job_adapter_)
        {
            std::cerr << "Background job adapter not initialized" << std::endl;
            return -1;
        }

        return bg_job_adapter_->bgCommand(argc, argv);
    }

    void JobControl::showJobs(bool show_running, bool show_stopped, bool show_pids)
    {
        if (!bg_job_adapter_)
        {
            std::cerr << "Background job adapter not initialized" << std::endl;
            return;
        }

        // Update all job states before showing
        updateAllJobStates();
        
        bg_job_adapter_->showJobs(show_running, show_stopped, show_pids, false);
    }

    void JobControl::cleanupJobs()
    {
        std::lock_guard<std::mutex> lock(job_update_mutex);
        
        if (!bg_job_adapter_)
        {
            return;
        }

        // Clean up completed jobs
        bg_job_adapter_->cleanupJobs();

        // Remove completed jobs from job list
        auto it = jobs_.begin();
        while (it != jobs_.end())
        {
            Job* job = *it;
            
            // Update job state
            updateJobState(job);
            
            if (job->getState() == JobState::DONE)
            {
                delete job;
                it = jobs_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    bool JobControl::hasStoppedJobs()
    {
        if (!bg_job_adapter_)
        {
            return false;
        }

        // First update all job states
        updateAllJobStates();
        
        for (const auto& job : jobs_)
        {
            if (job->getState() == JobState::STOPPED)
            {
                return true;
            }
        }

        return bg_job_adapter_->hasStoppedJobs();
    }

    Job* JobControl::getJobByJobId(int job_id)
    {
        for (auto& job : jobs_)
        {
            if (job->getJobId() == job_id)
            {
                return job;
            }
        }

        return nullptr;
    }

    Job* JobControl::getJobByPid(pid_t pid)
    {
        updateAllJobStates(); // Ensure job states are current
        
        for (auto& job : jobs_)
        {
            // Check main job PID
            if (job->getPid() == pid)
            {
                return job;
            }
            
            // Check all processes in the job
            for (int i = 0; i < job->getProcessCount(); i++) {
                Process* process = job->getProcessByIndex(i);
                if (process && process->getPid() == pid) {
                    return job;
                }
            }
        }

        return nullptr;
    }

    bool JobControl::killJob(Job* job, int signal)
    {
        if (!job || !job->getBgJob()) {
            return false;
        }
        
        // Send signal to the job's process group
        pid_t pgid = -job->getPid(); // Negative PID sends signal to entire process group
        if (kill(pgid, signal) != 0) {
            perror("kill");
            return false;
        }
        
        return true;
    }

    int JobControl::continueJob(Job* job, bool foreground)
    {
        if (!job || !job->getBgJob()) {
            return -1;
        }
        
        // Continue the job
        if (job->getState() == JobState::STOPPED) {
            if (foreground) {
                return bg_job_adapter_->fgCommand(1, nullptr);
            } else {
                return bg_job_adapter_->bgCommand(1, nullptr);
            }
        }
        
        return 0;
    }

    // Job implementation
    Job::Job(const std::string &command)
        : command_(command), job_id_(-1), pid_(-1), state_(JobState::CREATED), 
          bg_job_(nullptr), exit_status_(0), signaled_(false), term_signal_(0)
    {
    }

    Job::~Job()
    {
        // Clean up process list
        for (auto& process : processes_) {
            delete process;
        }
        processes_.clear();
        
        // Don't delete bg_job_ here as it's managed by the BGJobAdapter
    }

    void Job::setState(JobState state)
    {
        state_ = state;
    }

    void Job::setJobId(int job_id)
    {
        job_id_ = job_id;
    }

    void Job::setPid(pid_t pid)
    {
        pid_ = pid;
    }

    int Job::getExitStatus() const
    {
        return exit_status_;
    }

    bool Job::isSignaled() const
    {
        return signaled_;
    }

    int Job::getTermSignal() const
    {
        return term_signal_;
    }

    void Job::setExitStatus(int exit_status)
    {
        exit_status_ = exit_status;
    }

    void Job::setSignaled(bool signaled)
    {
        signaled_ = signaled;
    }

    void Job::setTermSignal(int term_signal)
    {
        term_signal_ = term_signal;
    }

    void Job::setBgJob(struct job* bg_job)
    {
        bg_job_ = bg_job;
    }

    struct job* Job::getBgJob() const
    {
        return bg_job_;
    }
    
    void Job::addProcess(Process* process)
    {
        processes_.push_back(process);
    }
    
    Process* Job::getProcessByIndex(int index)
    {
        if (index >= 0 && index < static_cast<int>(processes_.size())) {
            return processes_[index];
        }
        return nullptr;
    }
    
    int Job::getProcessCount() const
    {
        return processes_.size();
    }

    // Process implementation
    Process::Process()
        : pid_(-1), state_(ProcessState::CREATED), status_(0), 
          exit_status_(0), term_signal_(0), stop_signal_(0)
    {
    }

    Process::~Process()
    {
        // Nothing to clean up
    }

    void Process::setPid(pid_t pid)
    {
        pid_ = pid;
    }

    pid_t Process::getPid() const
    {
        return pid_;
    }

    void Process::setState(ProcessState state)
    {
        state_ = state;
    }

    ProcessState Process::getState() const
    {
        return state_;
    }

    void Process::setStatus(int status)
    {
        status_ = status;
    }

    int Process::getStatus() const
    {
        return status_;
    }

    void Process::setExitStatus(int exit_status)
    {
        exit_status_ = exit_status;
    }

    int Process::getExitStatus() const
    {
        return exit_status_;
    }

    void Process::setTermSignal(int term_signal)
    {
        term_signal_ = term_signal;
    }

    int Process::getTermSignal() const
    {
        return term_signal_;
    }

    void Process::setStopSignal(int stop_signal)
    {
        stop_signal_ = stop_signal;
    }

    int Process::getStopSignal() const
    {
        return stop_signal_;
    }

} // namespace dash 