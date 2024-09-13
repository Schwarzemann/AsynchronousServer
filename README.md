# ASYNIOCP
Asynchronous IOCP Server

## How to build

For Linux or *BSD systems the process is quite straightforward. I will add a CMake file in the future but now it is unnecessary.
The command given below can be used to compile it.

```
g++ linux_epoll_srv.cpp -o linux_epoll_srv && g++ linux_epoll_client.cpp -o linux_epoll_client
```

However, for Windows you need Microsoft Visual Studio. The server code can be compiled directly from the IDE itself.
But I personally recommend compiling the client from Developer Command Prompt.

```
cl win_iocp_client.cpp /link Ws2_32.lib
```
