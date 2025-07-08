/**
 * @file wait_command.cpp
 * @brief wait 内置命令实现
 */

#include <iostream>
#include <stdexcept>
#include "builtins/wait_command.h"
#include "core/shell.h"
#include "job/job_control.h"
#include "utils/error.h"

namespace dash
{

    int WaitCommand::execute(const std::vector<std::string> &args)
    {
        if (!shell_->getJobControl()->isEnabled())
        {
            std::cerr << "wait: job control not enabled" << std::endl;
            return 1;
        }

        int exit_status = 0;

        if (args.size() == 1)
        {
            // Wait for all jobs
            for (const auto& pair : shell_->getJobControl()->getJobs())
            {
                shell_->getJobControl()->waitForJob(pair.first);
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
                        int job_id = std::stoi(target.substr(1));
                        exit_status = shell_->getJobControl()->waitForJob(job_id);
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
                    // Assuming it's a PID, but wait for specific PIDs is not directly supported by JobControl
                    // For now, we'll just print an error or implement a basic waitpid for it
                    std::cerr << "wait: waiting for specific PID not fully supported yet: " << target << std::endl;
                    exit_status = 1;
                }
            }
        }

        return exit_status;
    }

} // namespace dash