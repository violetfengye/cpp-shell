/**
 * @file wait_command.cpp
 * @brief wait built-in command implementation
 */

#include <iostream>
#include <stdexcept>
#include <string>
#include "builtins/wait_command.h"
#include "core/shell.h"
#include "job/job_control.h"
#include "utils/error.h"

namespace dash
{

    int WaitCommand::execute(const std::vector<std::string> &args)
    {
        JobControl* jobControl = shell_->getJobControl();
        if (!jobControl)
        {
            std::cerr << "wait: job control not enabled" << std::endl;
            return 1;
        }

        int exit_status = 0;

        if (args.size() == 1)
        {
            // Wait for all jobs
            // First update all job states
            jobControl->updateAllJobStates();
            
            // Find all running jobs and wait for them
            Job* job = jobControl->getJobByJobId(1); // Start with job ID 1
            int jobId = 1;
            
            while (job != nullptr) {
                if (job->getState() == JobState::RUNNING) {
                    exit_status = jobControl->waitForJob(job);
                }
                
                // Try next job ID
                jobId++;
                job = jobControl->getJobByJobId(jobId);
            }
        }
        else
        {
            for (size_t i = 1; i < args.size(); ++i)
            {
                const std::string& target = args[i];
                if (target[0] == '%')
                {
                    try
                    {
                        // Extract job ID from string (remove the % prefix)
                        int job_id = std::stoi(target.substr(1));
                        
                        // Find the job with this ID
                        Job* job = jobControl->getJobByJobId(job_id);
                        
                        if (job) {
                            exit_status = jobControl->waitForJob(job);
                        } else {
                            std::cerr << "wait: job not found: " << target << std::endl;
                            exit_status = 1;
                        }
                    }
                    catch (const std::invalid_argument &e)
                    {
                        std::cerr << "wait: invalid job ID: " << target << std::endl;
                        exit_status = 1;
                    }
                    catch (const std::out_of_range &e)
                    {
                        std::cerr << "wait: job ID out of range: " << target << std::endl;
                        exit_status = 1;
                    }
                }
                else
                {
                    try {
                        // Try to parse as a PID
                        pid_t pid = std::stoi(target);
                        exit_status = jobControl->waitForJobWithPid(pid);
                    }
                    catch (const std::exception &e) {
                        std::cerr << "wait: invalid process ID: " << target << std::endl;
                        exit_status = 1;
                    }
                }
            }
        }

        return exit_status;
    }

} // namespace dash 