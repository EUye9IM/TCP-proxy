#include <agps/agps.h>
#include <agps/check.h>

#include <iostream>
#include <ostream>
#include "mysocket.hpp"

/**
 * @brief 	连接被代理的服务器
 * @param	port 代理端口
 * @param	proxy_ip 代理服务器地址
 * @param	proxy_port 代理服务器端口
 * @return 	建立的Socket_Connect类指针
 **/
Anakin::Socket_Connect* connect_proxied_server(int port, std::string proxy_ip, int proxy_port);

int main(int argc, char **argv) {
	agps::Parser p;
	p.add(agps::Type::FLAG, 'h', "help", "输出参数列表");
	p.add(agps::Type::FLAG, 'd', "daemon", "成为守护进程");
	p.add(agps::Type::INT, 'p', "port", "代理端口号 [0-65535:0]", false,
		  agps::Value{.Int = 0}, CHECK_INT_BETWEEN(0, 65535));
	p.add(agps::Type::STR, 'H', "proxy_ip",
		  "被代理服务器 IP 地址 [192.168.1.236]", false,
		  agps::Value{.Str = "192.168.1.236"}, CHECK_STR_IPADDR);
	p.add(agps::Type::INT, 'P', "proxy_port",
		  "被代理服务器 TCP 端口号 [0-65535:80]", false, agps::Value{.Int = 80},
		  CHECK_INT_BETWEEN(0, 65535));
	p.add(agps::Type::STR, 'l', "logname",
		  "日志文件名（全路径） [/var/log/tcp-proxy-server.log]", false,
		  agps::Value{.Str = "/var/log/tcp-proxy-server.log"});
	p.parse(argc, (const char **)argv);
	if (!p.success() || p.isExist("help")) {
		p.printUsage(argv[0]);
		return 0;
	}

	std::cout << "This is server." << std::endl;
	std::cout << "daemon     : " << (p.get("daemon").Exist ? "yes" : "no")
			  << std::endl;
	std::cout << "port       : " << p.get("port").Int << std::endl;
	std::cout << "proxy_ip   : " << p.get("proxy_ip").Str << std::endl;
	std::cout << "proxy_port : " << p.get("proxy_port").Int << std::endl;
	std::cout << "logname    : " << p.get("logname").Str << std::endl;

	/* 获取连接的地址端口信息 */
	int port = p.get("port").Int;
	std::string proxy_ip = p.get("proxy_ip").Str;
	int proxy_port = p.get("proxy_port").Int;

	std::cout << "被代理服务器 " << proxy_ip << ":" << proxy_port << std::endl;
	auto client_socket = connect_proxied_server(port, proxy_ip, proxy_port);
	std::cout << "connect 成功" << std::endl;

	/* 配置代理服务端 */
	std::cout << "代理服务端启动..." << std::endl;
	Anakin::Socket_Accept proxy_server(AF_INET, SOCK_STREAM, 0);
	Anakin::Socket_Accept server_socket(AF_INET, SOCK_STREAM, 0);

	// 设置地址复用
	int opt = 1;
	server_socket.setopt(SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
	// 首先设置为非阻塞方式
	server_socket.SetSocketBlockingEnable(false);

	struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

	address.sin_addr.s_addr = INADDR_ANY;

	server_socket.Bind((struct sockaddr*)&address, sizeof(address));
	server_socket.Listen(3);
	std::cout << "listen..." << std::endl;


	delete client_socket;
	return 0;
}

// 之所以返回指针，是为防止调用析构函数close(fd)
Anakin::Socket_Connect* connect_proxied_server(int port, std::string proxy_ip, int proxy_port)
{
	/* 首先作为代理服务器连接测试网站 */
	auto client_socket = new Anakin::Socket_Connect(AF_INET, SOCK_STREAM, 0);

	// 设置为非阻塞方式
	client_socket->SetSocketBlockingEnable(false);

	struct sockaddr_in client_addr;

	// bind client端口
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = INADDR_ANY;
	client_addr.sin_port = htons(port);
	client_socket->Bind((struct sockaddr *)&client_addr, sizeof(client_addr));

	// 非阻塞方式连接
	int ret = client_socket->Connect(proxy_ip, proxy_port, false);
	int sockfd = client_socket->getfd();
	
    if (ret == -1 && errno == EINPROGRESS) {
        // 正在建立连接
        fd_set set;
        FD_ZERO(&set);
        FD_SET(sockfd , &set);
        int retval = select(sockfd + 1, NULL, &set, NULL, NULL);
        if (retval < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }
        if (FD_ISSET(sockfd, &set)) {
            // 说明已经可写，通过getpeername判断
			struct sockaddr peeraddr {};
			socklen_t peeraddrlen = sizeof(peeraddr);
            if (getpeername(sockfd, &peeraddr, &peeraddrlen) == -1) {
                perror("getpeername");
                exit(EXIT_FAILURE);
            }
        }
    }

	return client_socket;
}