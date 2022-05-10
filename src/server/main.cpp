#include <agps/agps.h>
#include <agps/check.h>

#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <ostream>
#include <sys/epoll.h>
#include "mysocket.hpp"

const size_t MAX_EVENTS = 10000;

/**
 * @brief 	连接被代理的服务器
 * @param	proxy_ip 代理服务器地址
 * @param	proxy_port 代理服务器端口
 * @param	myport 连接被代理服务器的端口，-1表示任意端口
 * @return 	建立的Socket_Connect类指针，之所以返回指针，是为防止调用析构函数close(fd)
 **/
Anakin::Socket_Connect* connect_proxied_server(std::string proxy_ip, int proxy_port, int myport=-1);

/**
 * @brief 	在两个socket之间传递信息
 * @param 	src_sock 源socket
 * @param	dst_sock 目标socket
 **/
void forward_message(int src_sock, int dst_sock);

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
	if (p.isExist("help")) {
		p.printUsage();
		return 0;
	}
	if (!p.success() || p.isExist("help")) {
		std::cout << "Error:" << std::endl;
		std::cout << p.error() << std::endl;
		p.printUsage();
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
	auto client_socket = connect_proxied_server(proxy_ip, proxy_port);
	std::cout << "connect 成功" << std::endl;

	/* 配置代理服务端 */
	std::cout << "代理服务端启动..." << std::endl;
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

	/* 下面创建 epoll 实例 */
	
	struct epoll_event ev, events[MAX_EVENTS];
	int epfd = epoll_create1(0);
	Anakin::checkerror(epfd, "epoll_create1");
	int listenfd = server_socket.getfd();
	ev.data.fd = listenfd;		// listenfd
	ev.events = EPOLLIN;		// 使用水平触发模式		
	// 设置epoll事件
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
	Anakin::checkerror(ret, "epoll_ctl");

	char buf[1024];
	ssize_t rval;

	// 进入循环判断
	for (;;) {
		int nfds = epoll_wait(epfd, events, MAX_EVENTS, 0);	// 超时设为0，立刻返回
		if (nfds == -1) {
			// 被信号中断
			if (errno == EINTR)
				continue;
			perror("epoll_wait");
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < nfds; ++i) {
			// 事件可读
			if (events[i].events & EPOLLIN) {
				// listenfd 连接处理
				if (events[i].data.fd == listenfd) {
					int conn_sock = server_socket.Accept();
					if (conn_sock == -1) {
						// 忽略以下错误，比如客户中止连接、有信号被捕获等
						if (errno != EAGAIN && errno != ECONNABORTED && errno != EWOULDBLOCK 
							&& errno != EPROTO && errno != EINTR) {
							perror("accept");
							exit(EXIT_FAILURE);
						}
					} else {
						// 打印连接的client端的信息
						char client_ip[INET_ADDRSTRLEN] = { 0 };
						address = server_socket.GetConn();
						inet_ntop(AF_INET, &address.sin_addr, client_ip, sizeof(client_ip));
						std::cout << "connect " << client_ip << ":" << ntohs(address.sin_port) << std::endl;

						// 设置非阻塞
						Anakin::SetSocketBlockingEnable(conn_sock, false);
						// 加入epoll
						ev.events = EPOLLIN;
						ev.data.fd = conn_sock;
						ret = epoll_ctl(epfd, EPOLL_CTL_ADD, conn_sock, &ev);
						Anakin::checkerror(ret, "epoll_ctl:conn_sock");
					}
				}	// end listenfd
				else {
					int connfd = events[i].data.fd;

					// forward_message(connfd, client_socket->getfd());

					
					rval = recv(connfd, buf, sizeof(buf), 0);
					if (rval == 0) {
						// 对端关闭连接，移除clientfd
						ret = epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL);
						Anakin::checkerror(ret, "epoll_ctl:delete");
						// 从类中删除这个连接
						server_socket.EraseConn(connfd);
					} else if (rval < 0) {
						// 出错
						if (errno != EWOULDBLOCK && errno != EINTR) {
							ret = epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL);
							Anakin::checkerror(ret, "epoll_ctl:delete");
							// 从类中删除这个连接
							server_socket.EraseConn(connfd);
						}
					} else {
						// 正常收发数据
						for (ssize_t n = 0; n < rval; n++)
							std::cout << buf[n] << std::flush;
						// std::cout << buf;
					}	// end recv返回值判断
					
					
				}	// end client fd
			
			}	// end 可读事件
			
			if (events[i].events & EPOLLOUT) {
				// 可写事件处理
				// todo...
			}
			
		}

	}
	


	delete client_socket;
	return 0;
}


Anakin::Socket_Connect* connect_proxied_server(std::string proxy_ip, int proxy_port, int myport)
{
	/* 首先作为代理服务器连接测试网站 */
	auto client_socket = new Anakin::Socket_Connect(AF_INET, SOCK_STREAM, 0);

	// 设置为非阻塞方式
	client_socket->SetSocketBlockingEnable(false);

	struct sockaddr_in client_addr;
	
	// 如果 myport 在参数中输入
	if (myport != -1) {
		// bind client端口
		client_addr.sin_family = AF_INET;
		client_addr.sin_addr.s_addr = INADDR_ANY;
		client_addr.sin_port = htons(myport);
		client_socket->Bind((struct sockaddr *)&client_addr, sizeof(client_addr));
	}

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

void forward_message(int src_sock, int dst_sock)
{
	// 假设为理想的情况，可读可写，进行测试
	char buf[1024];
	ssize_t rval;
	while ((rval = recv(src_sock, buf, sizeof(buf), 0)) > 0) {
		send(dst_sock, buf, rval, 0);
	}

}