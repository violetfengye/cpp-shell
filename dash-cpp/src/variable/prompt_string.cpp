#include"variable/prompt_string.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <string.h>

#define PROINFOBLUE "\033[1;34m"
#define PROINFOGREEN "\033[1;32m"
#define PROINFORED "\033[1;31m"
#define PROINFOYELLOW "\033[1;33m"
#define PROINFORESET "\033[0m"
#define PROINFOMAXSIZE 1024
#define PROINFORECSIZE 64

namespace dash{
    int prompt_string::promptMode = 3;
    void prompt_string::setPromptMode(unsigned int mode) {
        int modeHigh = mode >> 16, modeLow = mode & 0xffff;
        promptMode |= modeLow;
        promptMode &= ~modeHigh;
    }
    void prompt_string::printPromptInfo(){
        std::string prompt = prompt_string::getFormattedPrompt();
        printf("%s",prompt);
        resetColors();
    }
    std::string prompt_string::getFormattedPrompt(){
        std::string prompt;
        // 获取当前用户
        std::string user = getlogin();
        if (user == "") {
            user = "unknown";
        }
        int userlen = user.length();

        // 获取主机名
        struct utsname utsname_info;
        if (uname(&utsname_info) == -1) {
            perror("uname");
            return NULL;
        }
        std::string hostname = utsname_info.nodename;
        int hostnamelen = hostname.length();

        // 获取当前工作目录
        std::string cwd = getcwd(NULL, 0);
        if (cwd == "") {
            perror("getcwd");
            cwd = "/";
        }
        int cwdlen = cwd.length();

        // 构建简略提示符
        if((promptMode & prompt_string::formatShort) && userlen + hostnamelen + cwdlen + 2 >= PROINFORECSIZE){
            int lastLen = PROINFORECSIZE - userlen - hostnamelen - 2;
            if (lastLen > 7) {
                std::string start = cwd.substr( 0, lastLen / 2 - 2);
                std::string end = cwd.substr(cwdlen - lastLen / 2 + 3);
                const std::string indicator = "+...+";
                cwd = start + indicator + end;
            }
        }

        // 检查是否为 root 用户
        if (getuid() == 0) {
            const char *indicator = "#";
            if(promptMode & prompt_string::color){
                // 构建提示符
                prompt = PROINFOGREEN + user + "@" + hostname + 
                    PROINFORESET + ":" + 
                    PROINFOBLUE + cwd + 
                    PROINFORED + indicator + 
                    PROINFORESET + " ";
                
                //snprintf(prompt, PROINFOMAXSIZE, "%s%s@%s%s:%s%s%s%s%s ",
                //    PROINFOGREEN, user, hostname, PROINFORESET,PROINFOBLUE, cwd, PROINFORED, indicator, PROINFORESET);
            }
            else{
                // 构建提示符
                prompt = user + "@" + hostname + ":" + cwd + indicator + " ";
                //snprintf(prompt, PROINFOMAXSIZE, "%s@%s:%s%s ", user, hostname, cwd, indicator);
            }
                    
        }else{
            const char *indicator = "$";
            if(promptMode & prompt_string::color){
                // 构建提示符
                prompt = PROINFOGREEN + user + "@" + hostname +
                    PROINFORESET + ":" +
                    PROINFOBLUE + cwd +
                    PROINFOYELLOW + indicator +
                    PROINFORESET + " ";

                //snprintf(prompt, PROINFOMAXSIZE, "%s%s@%s%s:%s%s%s%s%s ",
                //    PROINFOGREEN, user, hostname, PROINFORESET,PROINFOBLUE, cwd, PROINFOYELLOW, indicator, PROINFORESET);
            }
            else{
                // 构建提示符
                prompt = user + "@" + hostname + ":" + cwd + indicator + " ";
                //snprintf(prompt, PROINFOMAXSIZE, "%s@%s:%s%s ", user, hostname, cwd, indicator);
            }
        }
        return prompt;
    }
    std::string prompt_string::getRawPrompt(){
        std::string prompt;
        // 获取当前用户
        std::string user = getlogin();
        if (user == "") {
            user = "unknown";
        }
        int userlen = user.length();

        // 获取主机名
        struct utsname utsname_info;
        if (uname(&utsname_info) == -1) {
            perror("uname");
            return NULL;
        }
        std::string hostname = utsname_info.nodename;
        int hostnamelen = hostname.length();

        // 获取当前工作目录
        std::string cwd = getcwd(NULL, 0);
        if (cwd == "") {
            perror("getcwd");
            cwd = "/";
        }
        int cwdlen = cwd.length();

        // 检查是否为 root 用户
        if (getuid() == 0) {
            prompt = user + "@" + hostname + ":" + cwd + "#" + " ";
        }else{
            prompt = user + "@" + hostname + ":" + cwd + "$" + " ";
        }
        return prompt;
        
    }
    void prompt_string::resetColors() {
        printf("%s",PROINFORESET);
    }
}

void resetColors() {
    printf("\033[0m");
}