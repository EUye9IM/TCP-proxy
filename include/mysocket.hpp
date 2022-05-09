#ifndef _MYSOCKET_HPP_
#define _MYSOCKET_HPP_

#include <iostream>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstring>
#include <unistd.h>
#include <linux/if_link.h>
#include <vector>
#include <ifaddrs.h>
#include <netdb.h>
#include <fcntl.h>

namespace Anakin {
    class Socket_Base {
    public:
        Socket_Base(int _domain, int _type, int _protocol);
        ~Socket_Base() {};
        /* ��ȡsocket������ */
        int getfd() {return fd;}

        /* �ı�socket������ */
        void setfd(int _fd) {
            fd = _fd;
        }
        void setopt(int _optname, const void *_optval, socklen_t _optlen, int _level=SOL_SOCKET);
        /* Read �� Write ������Ϊ����ģʽ��ƣ�return < 0 ��ֱ���˳��������� */
        int Read(void *_buf, size_t _n);
        int Write(void *_buf, size_t _n);
        /* �Ƿ�����������ʽ���쳣�˳����������óɹ� */
        void SetSocketBlockingEnable(bool blocking);

    protected:
        int fd;                 // socket �������������ں����̳���ָ��ǰ���²�����socket
        int domain;             // Э����
        int type;               // socket ����
        int protocol;           // ָ��Э��
        
    };

    inline Socket_Base::Socket_Base(int _domain, int _type, int _protocol)
        :domain(_domain), type(_type), protocol(_protocol)
    {
        // ����socket�ļ�������
        if ((fd = socket(domain, type, protocol)) == -1) {
            perror("socket ����ʧ��");
            exit(EXIT_FAILURE);
        }

    }

    inline int Socket_Base::Read(void *_buf, size_t _n)
    {
        // return read(fd, _buf, _n);
        int valread;
        if ((valread = read(fd, _buf, _n)) == -1) {
            perror("read ʧ��");
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
            perror("write ʧ��");
            exit(EXIT_FAILURE);
        }
        return valwrite;
    }


    inline void Socket_Base::setopt(int _optname, const void *_optval, socklen_t _optlen, int _level)
    {
        if (setsockopt(fd, _level, _optname, _optval, _optlen) == -1) {
            perror("setsockopt ʧ��");
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
    /* Socket_Listen ������Server������Bind��socket�ĺ������� */
    class Socket_Listen : public Socket_Base {
    public:
        // �̳й��캯��
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
            perror("bind ʧ��");
            exit(EXIT_FAILURE);
        }
        // ����ַ���ݿ���
        memcpy(&server_addr, _addr, _len);
    }

    inline void Socket_Listen::Listen(int _n) {
        // �������״̬
        if (listen(fd, _n) == -1) {
            perror("listen ʧ��");
            exit(EXIT_FAILURE);
        }
    }


    /* Socket_Accept ������Server������Accept��socket�ĺ������� */
    class Socket_Accept : public Socket_Listen {
    public:
        using Socket_Listen::Socket_Listen;
        ~Socket_Accept();
        int Accept();
        // ��ȡ���ӹ�ϵ
        sockaddr_in GetConn();
    
    protected:
        int sfd;            // ���պ��socket����������ʱ���գ���֮��fd����
        sockaddr_in client_addr;          // ���ӿͻ��˵ĵ�ַ

    };

    inline Socket_Accept::~Socket_Accept()
    {
        // close(fd);
        // perror("close server socket");
        // close(sfd);
        // perror("close socket");
    }

    inline int Socket_Accept::Accept()
    {
        int addrlen = sizeof(client_addr);
        /* Ϊ�˷�����������������ֱ���˳� */
        // if ((sfd = accept(fd, (struct sockaddr*)&client_addr,
        //                 (socklen_t*)&addrlen)) == -1) {
        //     perror("accept ʧ��");
        //     exit(EXIT_FAILURE);
        // }
        sfd = accept(fd, (struct sockaddr*)&client_addr, (socklen_t*)&addrlen);
        // ����socket��������ʹfdָ������
        std::swap(fd, sfd);
        return fd;
    }

    inline sockaddr_in Socket_Accept::GetConn() {
        return client_addr;
    }

    /* Socket_Connect ������ Client �� socket �ĺ������� */
    class Socket_Connect : public Socket_Base {
    public:
        using Socket_Base::Socket_Base;
        ~Socket_Connect();

        void Bind(const sockaddr *_client_addr, socklen_t _len);
        int Connect(const sockaddr *_server_addr, socklen_t _len, bool blocking=true);
        int Connect(const std::string _server_ip, int _server_port, bool blocking=true);
        // ��ȡ���ӵķ���˵���Ϣ
        sockaddr_in GetConn();
    
    protected:
        sockaddr_in client_addr;            // �ͻ������ڰ󶨵ĵ�ַ
        sockaddr_in server_addr;            // �󶨵ķ���˵�ַ

    };

    inline Socket_Connect::~Socket_Connect()
    {
        // close(fd);
        // perror("close client socket");
    }

    inline sockaddr_in Socket_Connect::GetConn()
    {
        return server_addr;
    }

    inline void Socket_Connect::Bind(const sockaddr *_client_addr, socklen_t _len)
    {
        if (bind(fd, _client_addr, _len) == -1) {
            perror("bind ʧ��");
            exit(EXIT_FAILURE);
        }
        memcpy(&client_addr, _client_addr, _len);
    }
        
    inline int Socket_Connect::Connect(const sockaddr *_server_addr, socklen_t _len, bool blocking)
    {   
        int ret = connect(fd, _server_addr, _len);
        // ���������ֱ���˳�
        if (ret == -1 && blocking) {
            perror("connect ʧ��");
            exit(EXIT_FAILURE);
        }
        memcpy(&server_addr, _server_addr, _len);
        return ret;
    }

    inline int Socket_Connect::Connect(const std::string _server_ip, int _server_port, bool blocking)
    {
        server_addr.sin_family = domain;
        server_addr.sin_port = htons(_server_port);

        // ת�� ip ��ַ
        if (inet_pton(domain, _server_ip.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "IP ��ַת��ʧ�ܣ���������\n";
            exit(EXIT_FAILURE);
        }
        return this->Connect((struct sockaddr*)&server_addr, sizeof(sockaddr), blocking);
    }

}

namespace Anakin {
    /* ��ȡ��������IPv4��ַ */
    inline std::vector<std::string> GetAddrInfo()
    {
        std::vector<std::string> ips;     // �洢������ipv4��ַ
        struct ifaddrs *ifaddr, *ifa;
        int family, s, n;
        char host[NI_MAXHOST];

        if (getifaddrs(&ifaddr) == -1) {
            perror("getifaddrs");
            exit(EXIT_FAILURE);
        }

        // ��������
        for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
            if (ifa->ifa_addr == NULL)
                continue;
            
            family = ifa->ifa_addr->sa_family;
        
            /* ���� AF_INET ��������չʾ���ַ */
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

    /* �ж�IP��ַ�Ƿ��Ǳ���������ַ */
    inline bool IsLocalInterface(std::string ip)
    {
        std::vector<std::string> ips = Anakin::GetAddrInfo();
        for (std::string s:ips) {
            if (s.compare(ip) == 0) {
                // ip ��ַ����
                return true;
            }
        }
        return false;
    }

    // �����鴦������ret < 0�˳�����
    inline void checkerror(int ret, const char*msg)
    {
        if (ret < 0) {
            perror(msg);
            exit(EXIT_FAILURE);
        }
    }

    // �����׽����������Ƿ�����
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