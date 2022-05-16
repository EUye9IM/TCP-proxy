#include <netinet/in.h>
#include <sys/epoll.h>
#include <logc/logc.h>
#include <utility>

#include "connection.h"
#include "mysocket.hpp"
#include "top.h"

// namespace Old {
// void Top::run() {
// 	while (1) {

// 		// epoll

// 		// 拿 fd 检查。
// 		//    Connection::run()
// 		// or delete Connection
// 		// or accept(); new Connection

// 		// 如果是 connection
// 		// run，fd_set中移除相关fd，再调用Connection::fdSet()拿取新的
// 		// 其他情况也有 fd_set 的变动

// 		// 以 fd_set_read 和 fd_set_write 更新epoll用的队列
// 	}
// }
// } // namespace Old
namespace New {

Tcp_Proxy::Tcp_Proxy(std::string _proxy_ip, int _proxy_port, int _port)
	: port(_port), proxy_ip(_proxy_ip), proxy_port(_proxy_port) {

	init_proxy_server();
	create_epoll();
	epoll_add_listenfd();

}

Tcp_Proxy::~Tcp_Proxy()
{
	delete proxy_server;
	proxy_server = NULL;

	close(epfd);
	conns.clear();
	conn_map.clear();
}

void Tcp_Proxy::Run() {
	while (1) {
		int nfds =
			epoll_wait(epfd, events, MAX_EVENTS, 100);
		if (nfds == -1) {
			// 被信号中断
			if (errno == EINTR)
				continue;
			perror("epoll_wait");
			LogC::log_fatal("%s:%d %s\n", __FILE__, __LINE__, strerror(errno));
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < nfds; ++i) {

			int fd = events[i].data.fd;
			// 如果fd所代表的连接已经不存在
			if (!is_exist(fd))
				continue;
			
			auto conn = conn_map.at(fd);

			// 此处需要添加挂断等事件
			if (events[i].events & (EPOLLERR | EPOLLHUP)) {
				std::cout << "出现挂断事件" << std::endl;
				close_connection(fd);
				continue;
			}
			
			// 事件可读
			if (events[i].events & EPOLLIN) {
				// listenfd 连接处理
				if (fd == proxy_server->getfd()) {
					// std::cout << "listenfd 可读" << std::endl;
					new_connection();
					continue;
				}
				// clientfd 连接处理
				else {
					if (!conn->write_to_buf()) {
						close_connection(fd);
						continue;
					}
				}
			}
			// 事件可写
			if (events[i].events & EPOLLOUT) {
				if (!conn->read_from_buf()) {
					close_connection(fd);
				}
			}
		}
	}
}

void Tcp_Proxy::create_epoll() {
	/* 下面创建 epoll 实例 */
	epfd = epoll_create1(0);
	Anakin::checkerror(epfd, "epoll_create1");
}

void Tcp_Proxy::epoll_add_listenfd()
{
	struct epoll_event ev;
	int listenfd = proxy_server->getfd();

	// 创建连接，listen连接的other为NULL
	auto conn = new Connection(listenfd);
	if (conn == NULL) {
		std::cerr << "new connection error!" << std::endl;
		LogC::log_fatal("%s:%d %s\n", __FILE__, __LINE__, strerror(errno));
		exit(EXIT_FAILURE);
	}
	// 添加至 conn_map
	conn_map.insert(std::make_pair(listenfd, conn));

	ev.data.fd = listenfd;
	ev.events = EPOLLIN;   // 使用水平触发模式

	// 设置epoll事件
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
	Anakin::checkerror(ret, "epoll_ctl listen");
}

void Tcp_Proxy::new_connection() {
	// 代理服务器连接客户端
	int server_fd = accept_new_socket();
	if (server_fd == -1) {
		// 因为某种原因中断
		std::cout << "accept 失败" << std::endl;
		return;
	}

	// 代理服务器连接服务器
	int client_fd = connect_proxied_server();

	// 创建连接
	auto conn = new New::Connection(server_fd);
	if (conn == NULL) {
		std::cerr << "new connection error!" << std::endl;
		LogC::log_fatal("%s:%d %s\n", __FILE__, __LINE__, strerror(errno));
		exit(EXIT_FAILURE);
	}
	conn->build_connection(client_fd);
	
	// 加入epoll
	struct epoll_event ev;
	ev.data.fd = server_fd;
	ev.events = EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLERR;
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);
	Anakin::checkerror(ret, "epoll_ctl:conn_sock");

	ev.data.fd = client_fd;
	ev.events = EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLERR;
	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
	Anakin::checkerror(ret, "epoll_ctl:conn_sock");

	// 加入 conn_map
	conn_map.insert(std::make_pair(server_fd, conn));
	conn_map.insert(std::make_pair(client_fd, conn->get_other()));
}

void Tcp_Proxy::close_connection(int _fd)
{
	// 首先判断_fd连接是否存在
	if (!is_exist(_fd))
		return;
	
	auto conn = conn_map.at(_fd);
	// fd 和 sfd
	int fd = conn->get_fd();

	if (conn->get_other() == NULL) {
		// other为NULL，说明为listenfd
		std::cout << "代理服务器关闭" << std::endl;
		delete conn;
		conn_map.erase(fd);
		return;
	}

	int sfd = conn->get_other()->get_fd();

	// 首先从 epoll 中移除 fd
	int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
	Anakin::checkerror(ret, "epoll_ctl:delete");
	ret = epoll_ctl(epfd, EPOLL_CTL_DEL, sfd, NULL);
	Anakin::checkerror(ret, "epoll_ctl:delete");

	// 释放conn 以及 conn->other
	conn->delete_other();
	delete conn;

	// 释放套接字
	if (proxy_server->EraseConn(fd)) {
		// 删除成功，说明fd为服务端，sfd为客户端，删除
		delete_socket_conn(sfd);
	} else {
		proxy_server->EraseConn(sfd);
		delete_socket_conn(fd);
	}

	// conn_map删除
	conn_map.erase(fd);
	conn_map.erase(sfd);

	// std::cout << "连接中断" << std::endl;
}

int Tcp_Proxy::delete_socket_conn(int fd)
{
	for (auto it = conns.begin(); it != conns.end(); it++) {
		if ((*it)->getfd() == fd) {
			delete *it;
			conns.erase(it);
			return 1;
		}
	}
	return 0;
}

bool Tcp_Proxy::is_exist(int _fd)
{
	if (conn_map.find(_fd) != conn_map.end()) {
		return true;
	}
	return false;
}

void Tcp_Proxy::init_proxy_server() {
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
	proxy_server->Listen(500);
}

int Tcp_Proxy::accept_new_socket()
{
	// 首先accept连接
	int conn_sock = proxy_server->Accept();
	if (conn_sock == -1) {
		// 忽略以下错误，比如客户中止连接、有信号被捕获等
		if (errno != EAGAIN && errno != ECONNABORTED && errno != EWOULDBLOCK &&
			errno != EPROTO && errno != EINTR) {
			perror("accept");
			LogC::log_fatal("%s:%d %s\n", __FILE__, __LINE__, strerror(errno));
			exit(EXIT_FAILURE);
		}
	} else {
		// 打印连接的client端的信息
		struct sockaddr_in address;
		char client_ip[INET_ADDRSTRLEN] = {0};
		address = proxy_server->GetConn();
		inet_ntop(AF_INET, &address.sin_addr, client_ip, sizeof(client_ip));
		std::cout << "connect " << client_ip << ":" << ntohs(address.sin_port)
				  << std::endl;
		LogC::log_printf("connect %s:%d\n", client_ip, ntohs(address.sin_port));

		// 设置非阻塞
		Anakin::SetSocketBlockingEnable(conn_sock, false);
	}

	return conn_sock;
}

int Tcp_Proxy::connect_proxied_server(int myport) {
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
			LogC::log_fatal("%s:%d %s\n", __FILE__, __LINE__, strerror(errno));
			exit(EXIT_FAILURE);
		}
		if (FD_ISSET(sockfd, &set)) {
			// 说明已经可写，通过getpeername判断
			struct sockaddr peeraddr {};
			socklen_t peeraddrlen = sizeof(peeraddr);
			if (getpeername(sockfd, &peeraddr, &peeraddrlen) == -1) {
				perror("getpeername");
				LogC::log_fatal("%s:%d %s\n", __FILE__, __LINE__,
								strerror(errno));
				exit(EXIT_FAILURE);
			}
		}
	}

	conns.push_back(client_socket);
	return client_socket->getfd();
}

}; // namespace New