/**
 * @file kill_command.cpp
 * @brief kill 内置命令实现
 */

#include <iostream>
#include <stdexcept>
#include <signal.h>
#include <cstring>
#include "builtins/kill_command.h"
#include "core/shell.h"
#include "job/job_control.h"
#include "utils/error.h"

namespace dash
{

    // Helper to decode signal name or number
    static int decode_signal(const std::string& sig_str)
    {
        if (sig_str.empty())
            return -1;

        // Try to convert to integer (signal number)
        try
        {
            int signo = std::stoi(sig_str);
            if (signo > 0 && signo < NSIG)
            {
                return signo;
            }
        }
        catch (const std::invalid_argument &e)
        {
            // Not a number, try signal name
        }
        catch (const std::out_of_range &e)
        {
            // Number out of range
            return -1;
        }

        // Try to match signal name (e.g., SIGTERM, TERM)
        std::string upper_sig_str = sig_str;
        for (char &c : upper_sig_str) {
            c = toupper(c);
        }

        if (upper_sig_str.rfind("SIG", 0) == 0) {
            upper_sig_str = upper_sig_str.substr(3);
        }

        for (int i = 1; i < NSIG; ++i)
        {
            const char* sig_name = strsignal(i);
            if (sig_name && upper_sig_str == sig_name)
            {
                return i;
            }
        }

        return -1; // Not found
    }

    int KillCommand::execute(const std::vector<std::string> &args)
    {
        if (args.size() < 2)
        {
            std::cerr << "Usage: kill [-s signal | -signal] pid | %job_id ..." << std::endl;
            return 1;
        }

        int signo = SIGTERM; // Default signal is SIGTERM
        size_t start_idx = 1;

        if (args[1][0] == '-')
        {
            std::string opt = args[1].substr(1);
            if (opt == "s")
            {
                if (args.size() < 3)
                {
                    std::cerr << "kill: -s: option requires an argument" << std::endl;
                    return 1;
                }
                signo = decode_signal(args[2]);
                if (signo == -1)
                {
                    std::cerr << "kill: invalid signal: " << args[2] << std::endl;
                    return 1;
                }
                start_idx = 3;
            }
            else
            {
                signo = decode_signal(opt);
                if (signo == -1)
                {
                    std::cerr << "kill: invalid signal: " << args[1] << std::endl;
                    return 1;
                }
                start_idx = 2;
            }
        }

        if (start_idx >= args.size())
        {
            std::cerr << "kill: no PID or job ID specified" << std::endl;
            return 1;
        }

        int ret_status = 0;
        for (size_t i = start_idx; i < args.size(); ++i)
        {
            const std::string& target = args[i];
            pid_t pid_to_kill = 0;

            if (target[0] == '%')
            {
                if (!shell_->getJobControl()->isEnabled())
                {
                    std::cerr << "kill: job control not enabled" << std::endl;
                    ret_status = 1;
                    continue;
                }
                try
                {
                    int job_id = std::stoi(target.substr(1));
                    const auto& jobs = shell_->getJobControl()->getJobs();
                    auto it = jobs.find(job_id);
                    if (it != jobs.end())
                    {
                        pid_to_kill = it->second->getPgid(); // Kill process group
                    }
                    else
                    {
                        std::cerr << "kill: no such job: " << target << std::endl;
                        ret_status = 1;
                        continue;
                    }
                }
                catch (const std::invalid_argument &e)
                {
                    std::cerr << "kill: invalid job ID: " << target << std::endl;
                    ret_status = 1;
                    continue;
                }
                catch (const std::out_of_range &e)
                {
                    std::cerr << "kill: job ID out of range: " << target << std::endl;
                    ret_status = 1;
                    continue;
                }
            }
            else
            {
                try
                {
                    pid_to_kill = std::stoi(target);
                }
                catch (const std::invalid_argument &e)
                {
                    std::cerr << "kill: invalid PID: " << target << std::endl;
                    ret_status = 1;
                    continue;
                }
                catch (const std::out_of_range &e)
                {
                    std::cerr << "kill: PID out of range: " << target << std::endl;
                    ret_status = 1;
                    continue;
                }
            }

            if (pid_to_kill != 0)
            {
                if (kill(pid_to_kill, signo) < 0)
                {
                    std::cerr << "kill: (" << pid_to_kill << "): " << strerror(errno) << std::endl;
                    ret_status = 1;
                }
            }
        }

        return ret_status;
    }

} // namespace dash