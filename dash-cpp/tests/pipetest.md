  测试 1：简单的双命令管道

  这个测试验证最基本的管道功能是否正常工作。

   * 命令:

   1     ls | wc -l

   * 说明:
      ls 命令会列出当前目录（dash-cpp/build）下的所有文件和文件夹，每个占一行。wc -l 命令会统计它从标准输入接收到的行数。
   * 预期输出:
      一个数字，表示 dash-cpp/build 目录下的文件和文件夹总数。根据我之前看到的文件列表，这个数字应该是 10
  左右（具体数字可能会因编译过程产生的文件而略有不同），然后是一个换行符。

  ---

  测试 2：三命令管道与 grep 过滤

  这个测试检查更长的管道以及 grep 在中间作为过滤器是否正常工作。

   * 命令:

   1     ls -F /workspaces/cpp-shell/dash-cpp/src/core | grep .cpp | wc -l

   * 说明:
       1. ls -F /workspaces/cpp-shell/dash-cpp/src/core 列出 src/core 目录下的所有 .cpp 文件。
       2. grep .cpp 过滤出包含 ".cpp" 的行。
       3. wc -l 统计最终的行数。
   * 预期输出:
      一个数字，表示 src/core 目录中 .cpp 文件的数量。根据我之前看到的文件列表，这个数字应该是 12，然后是一个换行符。

  ---

  测试 3：数据流处理

  这个测试验证数据是否能正确地从一个命令流到另一个命令进行处理。

   * 命令:

   1     echo "hello world from dash-cpp" | tr 'a-z' 'A-Z'

   * 说明:
      echo 命令输出一个字符串，tr 命令将输入中的小写字母转换为大写字母。
   * 预期输出:

   1     HELLO WORLD FROM DASH-CPP


  ---

  测试 4：管道与文件重定向结合

  这个测试验证管道中的命令是否能正确地将最终输出重定向到文件中。

   * 命令:

   1     echo "pipeline test" | tee output.txt

   * 说明:
      tee 命令会把它从标准输入接收到的内容同时输出到标准输出和指定的文件中。
   * 预期输出:
       1. 屏幕上会显示:

   1         pipeline test

       2. 在 `build` 目录下会创建一个名为 `output.txt` 的文件，你可以用 cat output.txt 命令查看它的内容，应该也是 pipeline test。

  ---

  测试 5：管道中包含错误命令

  这个测试检查你的 shell 如何处理管道中某个命令执行失败的情况。

   * 命令:

   1     ls /nonexistent_directory | wc -c

   * 说明:
      ls /nonexistent_directory 会因为目录不存在而失败，并向标准错误（stderr）输出一条错误信息。wc -c
  会因为没有从标准输入（stdin）接收到任何数据而输出 0。
   * 预期输出:
      你的 shell 应该同时显示来自 ls 的错误信息和来自 wc -c 的输出。顺序可能会有所不同，但大致如下：

   1     ls: cannot access '/nonexistent_directory': No such file or directory
   2     0


  ---