/**
 * @file job_control.h
 * @brief Job control class definitions
 */

#ifndef DASH_JOB_CONTROL_H
#define DASH_JOB_CONTROL_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <sys/types.h>

// Forward declaration of C-style job struct from bg_job_control.h
struct job;

namespace dash
{
    // Forward declarations
    class Shell;
    class BGJobAdapter;

    /**
     * @brief Process state enumeration
     */
    enum class ProcessState
    {
        CREATED,  // Just created, not yet started
        RUNNING,  // Currently running
        STOPPED,  // Temporarily stopped (SIGSTOP, SIGTSTP, etc.)
        SIGNALED, // Terminated by signal
        DONE      // Normally terminated
    };

    /**
     * @brief Job state enumeration
     */
    enum class JobState
    {
        CREATED,  // Just created, not yet started
        RUNNING,  // Currently running
        STOPPED,  // Temporarily stopped (SIGSTOP, SIGTSTP, etc.)
        DONE      // Completed (normally or by signal)
    };

    /**
     * @brief Process class representing a single process in a job
     */
    class Process
    {
    private:
        pid_t pid_;                // Process ID
        ProcessState state_;       // Current state
        int status_;               // Status returned by waitpid
        int exit_status_;          // Exit code if normally terminated
        int term_signal_;          // Termination signal if killed
        int stop_signal_;          // Stop signal if stopped

    public:
        /**
         * @brief Constructor
         */
        Process();

        /**
         * @brief Destructor
         */
        ~Process();

        /**
         * @brief Set process ID
         * @param pid Process ID
         */
        void setPid(pid_t pid);

        /**
         * @brief Get process ID
         * @return Process ID
         */
        pid_t getPid() const;

        /**
         * @brief Set process state
         * @param state Process state
         */
        void setState(ProcessState state);

        /**
         * @brief Get process state
         * @return Process state
         */
        ProcessState getState() const;

        /**
         * @brief Set status from waitpid
         * @param status Status from waitpid
         */
        void setStatus(int status);

        /**
         * @brief Get status
         * @return Status value
         */
        int getStatus() const;

        /**
         * @brief Set exit status
         * @param exit_status Exit status
         */
        void setExitStatus(int exit_status);

        /**
         * @brief Get exit status
         * @return Exit status
         */
        int getExitStatus() const;

        /**
         * @brief Set termination signal
         * @param term_signal Signal that terminated the process
         */
        void setTermSignal(int term_signal);

        /**
         * @brief Get termination signal
         * @return Termination signal
         */
        int getTermSignal() const;

        /**
         * @brief Set stop signal
         * @param stop_signal Signal that stopped the process
         */
        void setStopSignal(int stop_signal);

        /**
         * @brief Get stop signal
         * @return Stop signal
         */
        int getStopSignal() const;
    };

    /**
     * @brief Job class representing a group of processes
     */
    class Job
    {
    private:
        std::string command_;      // Command string
        int job_id_;               // Job ID
        pid_t pid_;                // Main process ID
        JobState state_;           // Current state
        struct job* bg_job_;       // Pointer to underlying C-style job
        int exit_status_;          // Exit code of the job
        bool signaled_;            // Whether job was terminated by signal
        int term_signal_;          // Signal that terminated the job
        std::vector<Process*> processes_; // Processes in this job

    public:
        /**
         * @brief Constructor
         * @param command Command string
         */
        explicit Job(const std::string &command);

        /**
         * @brief Destructor
         */
        ~Job();

        /**
         * @brief Set job state
         * @param state New job state
         */
        void setState(JobState state);

        /**
         * @brief Get job state
         * @return Current job state
         */
        JobState getState() const { return state_; }

        /**
         * @brief Set job ID
         * @param job_id Job ID
         */
        void setJobId(int job_id);

        /**
         * @brief Get job ID
         * @return Job ID
         */
        int getJobId() const { return job_id_; }

        /**
         * @brief Set process ID
         * @param pid Process ID
         */
        void setPid(pid_t pid);

        /**
         * @brief Get process ID
         * @return Process ID
         */
        pid_t getPid() const { return pid_; }

        /**
         * @brief Get command string
         * @return Command string
         */
        const std::string& getCommand() const { return command_; }

        /**
         * @brief Get exit status
         * @return Exit status
         */
        int getExitStatus() const;

        /**
         * @brief Check if job was terminated by signal
         * @return true if terminated by signal, false otherwise
         */
        bool isSignaled() const;

        /**
         * @brief Get termination signal
         * @return Termination signal
         */
        int getTermSignal() const;

        /**
         * @brief Set exit status
         * @param exit_status Exit status
         */
        void setExitStatus(int exit_status);

        /**
         * @brief Set signaled flag
         * @param signaled Whether job was terminated by signal
         */
        void setSignaled(bool signaled);

        /**
         * @brief Set termination signal
         * @param term_signal Termination signal
         */
        void setTermSignal(int term_signal);

        /**
         * @brief Set underlying C-style job
         * @param bg_job Pointer to C-style job
         */
        void setBgJob(struct job* bg_job);

        /**
         * @brief Get underlying C-style job
         * @return Pointer to C-style job
         */
        struct job* getBgJob() const;

        /**
         * @brief Add process to job
         * @param process Process to add
         */
        void addProcess(Process* process);

        /**
         * @brief Get process by index
         * @param index Process index
         * @return Pointer to process or nullptr if index is out of range
         */
        Process* getProcessByIndex(int index);
        
        /**
         * @brief Get number of processes in job
         * @return Number of processes
         */
        int getProcessCount() const;
    };

    /**
     * @brief Job control manager class
     */
    class JobControl
    {
    private:
        Shell *shell_;                // Shell instance
        BGJobAdapter *bg_job_adapter_; // Background job adapter
        std::vector<Job*> jobs_;     // List of jobs

    public:
        /**
         * @brief Constructor
         * @param shell Shell instance
         */
        explicit JobControl(Shell *shell);

        /**
         * @brief Destructor
         */
        ~JobControl();

        /**
         * @brief Initialize job control subsystem
         * @return true if successful, false otherwise
         */
        bool initialize();

        /**
         * @brief Create a new job
         * @param command Command string
         * @return Pointer to created job or nullptr on failure
         */
        Job* createJob(const std::string &command);

        /**
         * @brief Run command in background
         * @param job Job to run
         * @param cmd Command to execute
         * @param argv Command arguments
         * @return 0 on success, negative value on failure
         */
        int runInBackground(Job* job, const std::string& cmd, char *const argv[]);

        /**
         * @brief Run command in foreground
         * @param job Job to run
         * @param cmd Command to execute
         * @param argv Command arguments
         * @return Command exit status
         */
        int runInForeground(Job* job, const std::string& cmd, char *const argv[]);

        /**
         * @brief Wait for job to complete
         * @param job Job to wait for
         * @return Exit status
         */
        int waitForJob(Job* job);
        
        /**
         * @brief Wait for specific PID to complete
         * @param pid Process ID to wait for
         * @return Exit status
         */
        int waitForJobWithPid(pid_t pid);

        /**
         * @brief Update job state
         * @param job Job to update
         */
        void updateJobState(Job* job);
        
        /**
         * @brief Update all job states
         */
        void updateAllJobStates();

        /**
         * @brief Implement foreground command
         * @param argc Argument count
         * @param argv Arguments
         * @return Exit status
         */
        int fgCommand(int argc, char** argv);

        /**
         * @brief Implement background command
         * @param argc Argument count
         * @param argv Arguments
         * @return Exit status
         */
        int bgCommand(int argc, char** argv);

        /**
         * @brief Show job list
         * @param show_running Whether to show running jobs
         * @param show_stopped Whether to show stopped jobs
         * @param show_pids Whether to show process IDs
         */
        void showJobs(bool show_running, bool show_stopped, bool show_pids);

        /**
         * @brief Clean up completed jobs
         */
        void cleanupJobs();

        /**
         * @brief Check if there are stopped jobs
         * @return true if there are stopped jobs, false otherwise
         */
        bool hasStoppedJobs();

        /**
         * @brief Get job by job ID
         * @param job_id Job ID
         * @return Pointer to job or nullptr if not found
         */
        Job* getJobByJobId(int job_id);

        /**
         * @brief Get job by process ID
         * @param pid Process ID
         * @return Pointer to job or nullptr if not found
         */
        Job* getJobByPid(pid_t pid);
        
        /**
         * @brief Send signal to job
         * @param job Job to send signal to
         * @param signal Signal to send
         * @return true if successful, false otherwise
         */
        bool killJob(Job* job, int signal);
        
        /**
         * @brief Continue stopped job
         * @param job Job to continue
         * @param foreground Whether to continue in foreground
         * @return 0 on success, negative value on failure
         */
        int continueJob(Job* job, bool foreground);
    };

} // namespace dash

#endif // DASH_JOB_CONTROL_H 