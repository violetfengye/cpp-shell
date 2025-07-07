#ifndef GETPROMPTINFO_H
#define GETPROMPTINFO_H

#include <stdlib.h>
#include <string>

namespace dash {
    class prompt_string {
    public:
        // 常量，用于表示不同的显示模式
        static const int formatShort = 1;
        static const int formatLong = 1 << 16;
        static const int color = 2;
        static const int noCme = 2 << 16;

        prompt_string();
        ~prompt_string();
        //输出命令提示符
        static void printPromptInfo() ;
        //返回格式化前的命令提示符
        static std::string getRawPrompt() ;
        //返回格式化后的命令提示符
        static std::string getFormattedPrompt() ;
        //重置颜色
        static void resetColors() ;
        //设置显示模式
        static void setPromptMode(unsigned int mode) ;

    private:
        // 静态变量用于存储当前的显示模式
        static int promptMode;
    };
}

#endif // GETPROMPTINFO_H