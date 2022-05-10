#ifndef _MYSOCKET_HPP_
#define _MYSOCKET_HPP_

#include <cerrno>
#include <cstdio>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstring>
#include <unistd.h>
#include <linux/if_link.h>
#include <utility>
#include <vector>
#include <ifaddrs.h>
#include <netdb.h>
#include <fcntl.h>
#include <map>

namespace Anakin {
    class Socket_Base {
    public:
        Socket_Base(int _domain, int _type, int _protocol);
        /* 析构函数关闭fd描述符 */
        ~Socket_Base() {
            close(fd);
        }

        /* 获取socket描述符 */
        int getfd() {return fd;}

        /* 改变socket描述符 */
        void setfd(int _fd) {
            fd = _fd;
        }
        void setopt(int _optname, const void *_optval, socklen_t _optlen, int _level=SOL_SOCKET);
        /* Read 和 Write 函数是为阻塞模式设计，return < 0 会直接退出程序，慎用 */
        int Read(void *_buf, size_t _n);
        int Write(void *_buf, size_t _n);
        /* 是否启动阻塞方式，异常退出，否则设置成功 */
        void SetSocketBlockingEnable(bool blocking);

    protected:
        int fd;                 // socket 描述符，并且在后续继承中指向当前最新操作的socket
        int domain;             // 协议域
        int type;               // socket 类型
        int protocol;           // 指定协议
        
    };

    inline Socket_Base::Socket_Base(int _domain, int _type, int _protocol)
        :domain(_domain), type(_type), protocol(_protocol)
    {
        // 创建socket文件描述符
        if ((fd = socket(domain, type, protocol)) == -1) {
            perror("socket 创建失败");
            exit(EXIT_FAILURE);
        }

    }

    inline int Socket_Base::Read(void *_buf, size_t _n)
    {
        // return read(fd, _buf, _n);
        int valread;
        if ((valread = read(fd, _buf, _n)) == -1) {
            perror("read 失败");
            std::cerr << "errno = " << errno << std::endl;
            exit(EXIT_FAILURE);
        }
        return valread;
    }

    inline int Socket_Base::Write(void *_buf, size_t _n)
    {
        // return write(fd, _buf, _n);
        int valwrite;
        if ((valwrite = write(fd, _buf, _n)) == -1) {
            perror("write 失败");
            exit(EXIT_FAILURE);
        }
        return valwrite;
    }


    inline void Socket_Base::setopt(int _optname, const void *_optval, socklen_t _optlen, int _level)
    {
        if (setsockopt(fd, _level, _optname, _optval, _optlen) == -1) {
            perror("setsockopt 失败");
            exit(EXIT_FAILURE);
        }
    }
    

    inline void Socket_Base::SetSocketBlockingEnable(bool blocking)
    {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) {
            perror("fcntl");
            exit(EXIT_FAILURE);
        }
        flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
        if (fcntl(fd, F_SETFL, flags) == -1) {
            perror("fcntl");
            exit(EXIT_FAILURE);
        }
    }


    /********************************************************/
    /* Socket_Listen 类用于Server端用于Bind的socket的后续处理 */
    class Socket_Listen : public Socket_Base {
    public:
        // 继承构造函数
        using Socket_Base::Socket_Base;
        void Bind(const sockaddr *_addr, socklen_t _len);
        void Listen(int _n);


    protected:
        sockaddr_in server_addr;
    };

    inline void Socket_Listen::Bind(const sockaddr *_addr, socklen_t _len)
    {
        // bind
        if (bind(fd, _addr, _len) == -1) {
            perror("bind 失败");
            exit(EXIT_FAILURE);
        }
        // 将地址内容拷贝
        memcpy(&server_addr, _addr, _len);
    }

    inline void Socket_Listen::Listen(int _n) {
        // 进入监听状态
        if (listen(fd, _n) == -1) {
            perror("listen 失败");
            exit(EXIT_FAILURE);
        }
    }


    /* Socket_Accept 类用于Server端用于Accept的socket的后续处理 */
    class Socket_Accept : public Socket_Listen {
    public:
        using Socket_Listen::Socket_Listen;
        using conns = std::map<int, sockaddr_in>;

        ~Socket_Accept();
        int Accept();
        // 获取连接关系
        sockaddr_in GetConn();

        // 获取所有客户端的连接关系
        conns GetConns() {
            return client_conns;
        }

        // 删除某个客户端连接
        void EraseConn(int socketfd) {
            close(socketfd);
            client_conns.erase(socketfd);
        }
        
    
    protected:
        int sfd;            // 接收后的socket描述符，临时接收，随之与fd交换
        sockaddr_in client_addr;          // 连接客户端的地址
        
        std::map<int, sockaddr_in> client_conns;    // 考虑到多客户端连接的情况，使用map存储
    };

    inline Socket_Accept::~Socket_Accept()
    {
        // 关闭所有的客户端fd
        for (auto &conn : client_conns)
            close(conn.first);
        // 清空map
        client_conns.clear();
    }

    inline int Socket_Accept::Accept()
    {
        int addrlen = sizeof(client_addr);
        /* 为了非阻塞操作，不可以直接退出 */
        // if ((sfd = accept(fd, (struct sockaddr*)&client_addr,
        //                 (socklen_t*)&addrlen)) == -1) {
        //     perror("accept 失败");
        //     exit(EXIT_FAILURE);
        // }
        sfd = accept(fd, (struct sockaddr*)&client_addr, (socklen_t*)&addrlen);

        // 需要将新的客户端的连接信息存储
        client_conns.insert(std::make_pair(sfd, client_addr));

        /* 因为考虑到多客户端的连接，修改Socket_Accept维护的描述符 */
        // 交换socket描述符，使fd指向最新
        // std::swap(fd, sfd);
        return sfd;
    }

    inline sockaddr_in Socket_Accept::GetConn() {
        return client_addr;
    }

    /* Socket_Connect 类用于 Client 端 socket 的后续处理 */
    class Socket_Connect : public Socket_Base {
    public:
        using Socket_Base::Socket_Base;
        ~Socket_Connect();

        void Bind(const sockaddr *_client_addr, socklen_t _len);
        int Connect(const sockaddr *_server_addr, socklen_t _len, bool blocking=true);
        int Connect(const std::string _server_ip, int _server_port, bool blocking=true);
        // 获取连接的服务端的信息
        sockaddr_in GetConn();

        // 获取服务端地址
        sockaddr_in GetServer() {
            return server_addr;
        }
    
    protected:
        sockaddr_in client_addr;            // 客户端用于绑定的地址
        sockaddr_in server_addr;            // 绑定的服务端地址

    };

    inline Socket_Connect::~Socket_Connect()
    {

    }

    inline sockaddr_in Socket_Connect::GetConn()
    {
        return server_addr;
    }

    inline void Socket_Connect::Bind(const sockaddr *_client_addr, socklen_t _len)
    {
        if (bind(fd, _client_addr, _len) == -1) {
            perror("bind 失败");
            exit(EXIT_FAILURE);
        }
        memcpy(&client_addr, _client_addr, _len);
    }
    
    inline int Socket_Connect::Connect(const sockaddr *_server_addr, socklen_t _len, bool blocking)
    {   
        int ret = connect(fd, _server_addr, _len);
        // 阻塞情况下直接退出
        if (ret == -1 && blocking) {
            perror("connect 失败");
            exit(EXIT_FAILURE);
        }
        memcpy(&server_addr, _server_addr, _len);
        return ret;
    }

    inline int Socket_Connect::Connect(const std::string _server_ip, int _server_port, bool blocking)
    {
        server_addr.sin_family = domain;
        server_addr.sin_port = htons(_server_port);

        // 转换 ip 地址
        if (inet_pton(domain, _server_ip.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "IP 地址转换失败，请检查输入\n";
            exit(EXIT_FAILURE);
        }
        return this->Connect((struct sockaddr*)&server_addr, sizeof(sockaddr), blocking);
    }

}

namespace Anakin {
    /* 获取本地网卡IPv4地址 */
    inline std::vector<std::string> GetAddrInfo()
    {
        std::vector<std::string> ips;     // 存储本机的ipv4地址
        struct ifaddrs *ifaddr, *ifa;
        int family, s, n;
        char host[NI_MAXHOST];

        if (getifaddrs(&ifaddr) == -1) {
            perror("getifaddrs");
            exit(EXIT_FAILURE);
        }

        // 遍历链表
        for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
            if (ifa->ifa_addr == NULL)
                continue;
            
            family = ifa->ifa_addr->sa_family;
        
            /* 对于 AF_INET 的网卡，展示其地址 */
            if (family == AF_INET) {
                s = getnameinfo(ifa->ifa_addr,
                        (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                            sizeof(struct sockaddr_in6),
                        host, NI_MAXHOST,
                        NULL, 0, NI_NUMERICHOST);
                if (s != 0) {
                    printf("getnameinfo() failed: %s\n", gai_strerror(s));
                    exit(EXIT_FAILURE);
                } 

                ips.push_back(host);
            } else if (family == AF_PACKET && ifa->ifa_data != NULL) {
                // AF_PACKET
            }
        }

        freeifaddrs(ifaddr);
        return ips;
    }

    /* 判断IP地址是否是本机网卡地址 */
    inline bool IsLocalInterface(std::string ip)
    {
        std::vector<std::string> ips = Anakin::GetAddrInfo();
        for (std::string s:ips) {
            if (s.compare(ip) == 0) {
                // ip 地址存在
                return true;
            }
        }
        return false;
    }

    // 错误检查处理函数，ret < 0退出程序
    inline void checkerror(int ret, const char*msg)
    {
        if (ret < 0) {
            perror(msg);
            exit(EXIT_FAILURE);
        }
    }

    // 设置套接字描述符是否阻塞
    inline void SetSocketBlockingEnable(int fd, bool blocking)
    {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) {
            perror("fcntl");
            exit(EXIT_FAILURE);
        }
        flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
        if (fcntl(fd, F_SETFL, flags) == -1) {
            perror("fcntl");
            exit(EXIT_FAILURE);
        }
    }

}

#endif