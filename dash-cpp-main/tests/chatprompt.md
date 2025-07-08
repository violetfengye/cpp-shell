你可以看一下我的工作区，我有两个文件夹，一个是dash-debian，这个是我从社区下载的比较权威的dash实现代码；另一个是dash-cpp，是我准备将dash-debian改写成c++模式，因为c++的异常处理等更加健壮，而且脚手架也更多。

你可以看一下我的工作区，我有两个文件夹，一个是dash-debian，这个是我从社区下载的比较权威的dash实现代码；另一个是dash-cpp，是我准备将dash │
│                                                                                                                                            │
│        -debian改写成c++模式，因为c++的异常处理等更加健壮，而且脚手架也更多。但是我发现目前有一个问题是，当我使用后台执行的时候，如"sleep 2 │
│                                                                                                                                            │
│        &"时，当后台指令完成时，会直接强制退出整个shell程序，例如“vscode ➜ /workspaces/cpp-shell/dash-cpp/build (main) $ ./dash             │
│                                                                                                                                            │
│        Dash-CPP Shell Created by Isaleafa.                                                                                                 │
│                                                                                                                                            │
│        $ sleep 2 &                                                                                                                         │
│                                                                                                                                            │
│        [18029] Background job started                                                                                                      │
│                                                                                                                                            │
│        $ exit”按理说不应该是这样的,我刚刚测试dash-debian的实现，除了没有返回后台进程pid之外，不会导致整个shell结束“vscode ➜                │
│                                                                                                                                            │
│        /workspaces/cpp-shell/dash-debian/src (main) $ ./dash                                                                               │
│                                                                                                                                            │
│        $ sleep 2 &                                                                                                                         │
│                                                                                                                                            │
│        $ jobs                                                                                                                              │
│                                                                                                                                            │
│        [1] + Done                       sleep 2                                                                                            │
│                                                                                                                                            │
│        $ sleep 20 &                                                                                                                        │
│                                                                                                                                            │
│        $ jobs                                                                                                                              │
│                                                                                                                                            │
│        [1] + Running                    sleep 20                                                                                           │
│                                                                                                                                            │
│                                                                                                                                            │
│        $ ”你可以帮我把dash-debian的jobs和后台处理直接搬运到我的cpp项目吗？只需要做语法上的兼容和规范就行，不要过多的加入你个人主观修改意见 │
│    ，确保修改后的代码可以正常编译且解决问题 

你可以看到文件目录，dash-cpp是我自己做的，dash-debian是我从社区下载的权威代码，我想把他的那套后台任务控制移植到我的dash-cpp里面，请你帮 │
│    我完成移植  