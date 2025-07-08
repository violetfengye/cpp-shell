   1. 启动一个后台作业:

   1     sleep 10 &

      预期: shell 会打印出作业号和 PID，然后立即返回提示符。

   2. 查看作业列表:

   1     jobs

      预期: 会显示 [1] Running sleep 10。


   3. 将作业切换到前台:

   1     fg %1

      预期: sleep 10 会在前台继续执行，shell 会等待它完成后再显示提示符。

   4. 启动一个前台作业并挂起它:

   1     sleep 20

      在它运行时，按 Ctrl+Z。
      预期: shell 会打印 Stopped，然后返回提示符。


   5. 查看作业列表:

   1     jobs

      预期: 会显示 [1] Stopped sleep 20。


   6. 将挂起的作业在后台恢复:

   1     bg %1

      预期: shell 会打印 [1] Running sleep 20 &，然后立即返回提示符。