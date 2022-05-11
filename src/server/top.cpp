#include <netinet/in.h>
#include <sys/epoll.h>

#include "top.h"
#include "mysocket.hpp"

void Top::run(){
	while(1){

		// epoll

		// 拿 fd 检查。
		//    Connection::run()
		// or delete Connection
		// or accept(); new Connection

		// 如果是 connection run，fd_set中移除相关fd，再调用Connection::fdSet()拿取新的
		// 其他情况也有 fd_set 的变动

		// 以 fd_set_read 和 fd_set_write 更新epoll用的队列
	}
}

namespace New {

Tcp_Proxy::Tcp_Proxy(std::string _proxy_ip, int _proxy_port, int _port)
		 : port(_port), proxy_ip(_proxy_ip), proxy_port(_proxy_port)
{
	init_proxy_server();
	create_epoll();

}

void Tcp_Proxy::Run()
{
	while (1) {
		int nfds = epoll_wait(epfd, events, MAX_EVENTS, 0); // 超时设为0，立刻返回
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
				if (events[i].data.fd == proxy_server->getfd()) {
					new_connection();
					this->connect_proxied_server();
				}
			}
		}
		// epoll 事件产生
		/**************************************
		connection *conn = ptr;
		if (可读事件)
			if (fd == listenfd)
				new_connection();
			else 
				conn->write_buf();
				...错误处理
		
		if (可写事件)
			conn->read_buf();	// buf缓冲数据至fd

		*************************************/
	}

}


void Tcp_Proxy::create_epoll()
{
	/* 下面创建 epoll 实例 */
	struct epoll_event ev;
	epfd = epoll_create1(0);
	Anakin::checkerror(epfd, "epoll_create1");


	int listenfd = proxy_server->getfd();
	ev.data.fd = listenfd; // listenfd
	ev.events = EPOLLIN;   // 使用水平触发模式

	// 设置epoll事件
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
	Anakin::checkerror(ret, "epoll_ctl");
}

void Tcp_Proxy::new_connection()
{
	// 首先accept连接
	int conn_sock = proxy_server->Accept();
	if (conn_sock == -1) {
		// 忽略以下错误，比如客户中止连接、有信号被捕获等
		if (errno != EAGAIN && errno != ECONNABORTED &&
			errno != EWOULDBLOCK && errno != EPROTO &&
			errno != EINTR) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
	} else {
		// 打印连接的client端的信息
		struct sockaddr_in address;
		char client_ip[INET_ADDRSTRLEN] = {0};
		address = proxy_server->GetConn();
		inet_ntop(AF_INET, &address.sin_addr, client_ip,
					sizeof(client_ip));
		std::cout << "connect " << client_ip << ":"
					<< ntohs(address.sin_port) << std::endl;

		// 设置非阻塞
		Anakin::SetSocketBlockingEnable(conn_sock, false);
		// 加入epoll
		// struct epoll_event ev;
		// ev.events = EPOLLIN;
		// ev.data.fd = conn_sock;
		// ret = epoll_ctl(epfd, EPOLL_CTL_ADD, conn_sock, &ev);
		// Anakin::checkerror(ret, "epoll_ctl:conn_sock");
	}
}


void Tcp_Proxy::init_proxy_server()
{
	proxy_server = new Anakin::Socket_Accept(AF_INET, SOCK_STREAM, 0);

	// 设置地址复用
	int opt = 1;
	proxy_server->setopt(SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
	// 首先设置为非阻塞方式
	proxy_server->SetSocketBlockingEnable(false);

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = INADDR_ANY;

	proxy_server->Bind((struct sockaddr *)&address, sizeof(address));
	proxy_server->Listen(3);
}


Anakin::Socket_Connect* Tcp_Proxy::connect_proxied_server(int myport) 
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
		client_socket->Bind((struct sockaddr *)&client_addr,
							sizeof(client_addr));
	}

	// 非阻塞方式连接
	int ret = client_socket->Connect(proxy_ip, proxy_port, false);
	int sockfd = client_socket->getfd();

	if (ret == -1 && errno == EINPROGRESS) {
		// 正在建立连接
		fd_set set;
		FD_ZERO(&set);
		FD_SET(sockfd, &set);
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

	conns.push_back(client_socket);
	return client_socket;
}

};