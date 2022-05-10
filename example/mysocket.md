# mysocket.hpp 使用说明
## 简述
`mysocket.hpp`是对socket过程相关函数的封装，只用于socket的初始化部分。

声明实现都在hpp文件中完成，直接include即可，命名空间为 Anakin::

## 服务端
以下为服务端初始化代码：

```C++
Anakin::Socket_Accept server_socket(AF_INET, SOCK_STREAM, 0);

// 设置地址复用
int opt = 1;
server_socket.setopt(SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

// 首先设置为非阻塞方式
server_socket.SetSocketBlockingEnable(false);

server_socket.Bind((struct sockaddr*)&address, sizeof(address));
server_socket.Listen(3);
cout << "listen..." << endl;
```

类内进行了错误封装处理，如果出错则退出程序。


## 客户端
以下为客户端初始化代码：

```C++
Anakin::Socket_Connect client_socket(AF_INET, SOCK_STREAM, 0);

// 首先设置为非阻塞方式
client_socket.SetSocketBlockingEnable(false);

struct sockaddr_in client_addr;

int myport = 80;
// 如果 myport 在参数中输入
if (myport != -1) {
    // bind client端口
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = htons(myport);
    client_socket.Bind((struct sockaddr *)&client_addr, sizeof(client_addr));
}

// 非阻塞方式连接
string ip = "192.168.80.230";
int port = 1024;
int ret = client_socket.Connect(ip, port, false);

```

对于connect，第三个参数决定是否阻塞，默认为true，对于非阻塞的情形，需要结合返回值和errno进行判断。


## 关于非阻塞下的运行
socket类并没有对非阻塞的函数进行封装，只是提供了getfd函数获取描述符，其余操作自定义吧，比如read,write,select等

当然，你也可以使用setfd设置socket的描述符，但是此时不可以再获取连接的地址，设置功能有效

## Socket_Accept类说明
关于Socket_Accept类，此类维护一个listenfd接收连接，当accept后，新的fd会返回，并存储在类中的map中，具体可以查看

## 关于描述符的关闭
目前已经定义了析构函数关闭描述符，包括Socket_Accept中的连接的客户端socket也在析构函数中有关闭处理。

## 其他
头文件还有其他函数，比如checkerror函数，判断某个返回值是否<0，若是，则perror打印并退出。
还有诸如获取本地ip地址、判断是否是本地网卡地址的函数，皆为历史遗留产物
