# Asynchronous Server
Asynchronous IOCP Server

## How to build

For Windows you need Microsoft Visual Studio. The server code can be compiled directly from the IDE itself.
But I personally recommend compiling the client from Developer Command Prompt.
Anyone can use the commands given below.

```
cl WinIOCPClient.cpp /link Ws2_32.lib
```

```
cl WinIOCPServer.cpp /link Ws2_32.lib
```
