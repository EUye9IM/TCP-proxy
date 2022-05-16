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

// 		// �� fd ��顣
// 		//    Connection::run()
// 		// or delete Connection
// 		// or accept(); new Connection

// 		// ����� connection
// 		// run��fd_set���Ƴ����fd���ٵ���Connection::fdSet()��ȡ�µ�
// 		// �������Ҳ�� fd_set �ı䶯

// 		// �� fd_set_read �� fd_set_write ����epoll�õĶ���
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
			// ���ź��ж�
			if (errno == EINTR)
				continue;
			perror("epoll_wait");
			LogC::log_fatal("%s:%d %s\n", __FILE__, __LINE__, strerror(errno));
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < nfds; ++i) {

			int fd = events[i].data.fd;
			// ���fd������������Ѿ�������
			if (!is_exist(fd))
				continue;
			
			auto conn = conn_map.at(fd);

			// �˴���Ҫ��ӹҶϵ��¼�
			if (events[i].events & (EPOLLERR | EPOLLHUP)) {
				std::cout << "���ֹҶ��¼�" << std::endl;
				close_connection(fd);
				continue;
			}
			
			// �¼��ɶ�
			if (events[i].events & EPOLLIN) {
				// listenfd ���Ӵ���
				if (fd == proxy_server->getfd()) {
					// std::cout << "listenfd �ɶ�" << std::endl;
					new_connection();
					continue;
				}
				// clientfd ���Ӵ���
				else {
					if (!conn->write_to_buf()) {
						close_connection(fd);
						continue;
					}
				}
			}
			// �¼���д
			if (events[i].events & EPOLLOUT) {
				if (!conn->read_from_buf()) {
					close_connection(fd);
				}
			}
		}
	}
}

void Tcp_Proxy::create_epoll() {
	/* ���洴�� epoll ʵ�� */
	epfd = epoll_create1(0);
	Anakin::checkerror(epfd, "epoll_create1");
}

void Tcp_Proxy::epoll_add_listenfd()
{
	struct epoll_event ev;
	int listenfd = proxy_server->getfd();

	// �������ӣ�listen���ӵ�otherΪNULL
	auto conn = new Connection(listenfd);
	if (conn == NULL) {
		std::cerr << "new connection error!" << std::endl;
		LogC::log_fatal("%s:%d %s\n", __FILE__, __LINE__, strerror(errno));
		exit(EXIT_FAILURE);
	}
	// ����� conn_map
	conn_map.insert(std::make_pair(listenfd, conn));

	ev.data.fd = listenfd;
	ev.events = EPOLLIN;   // ʹ��ˮƽ����ģʽ

	// ����epoll�¼�
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
	Anakin::checkerror(ret, "epoll_ctl listen");
}

void Tcp_Proxy::new_connection() {
	// ������������ӿͻ���
	int server_fd = accept_new_socket();
	if (server_fd == -1) {
		// ��Ϊĳ��ԭ���ж�
		std::cout << "accept ʧ��" << std::endl;
		return;
	}

	// ������������ӷ�����
	int client_fd = connect_proxied_server();

	// ��������
	auto conn = new New::Connection(server_fd);
	if (conn == NULL) {
		std::cerr << "new connection error!" << std::endl;
		LogC::log_fatal("%s:%d %s\n", __FILE__, __LINE__, strerror(errno));
		exit(EXIT_FAILURE);
	}
	conn->build_connection(client_fd);
	
	// ����epoll
	struct epoll_event ev;
	ev.data.fd = server_fd;
	ev.events = EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLERR;
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);
	Anakin::checkerror(ret, "epoll_ctl:conn_sock");

	ev.data.fd = client_fd;
	ev.events = EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLERR;
	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
	Anakin::checkerror(ret, "epoll_ctl:conn_sock");

	// ���� conn_map
	conn_map.insert(std::make_pair(server_fd, conn));
	conn_map.insert(std::make_pair(client_fd, conn->get_other()));
}

void Tcp_Proxy::close_connection(int _fd)
{
	// �����ж�_fd�����Ƿ����
	if (!is_exist(_fd))
		return;
	
	auto conn = conn_map.at(_fd);
	// fd �� sfd
	int fd = conn->get_fd();

	if (conn->get_other() == NULL) {
		// otherΪNULL��˵��Ϊlistenfd
		std::cout << "����������ر�" << std::endl;
		delete conn;
		conn_map.erase(fd);
		return;
	}

	int sfd = conn->get_other()->get_fd();

	// ���ȴ� epoll ���Ƴ� fd
	int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
	Anakin::checkerror(ret, "epoll_ctl:delete");
	ret = epoll_ctl(epfd, EPOLL_CTL_DEL, sfd, NULL);
	Anakin::checkerror(ret, "epoll_ctl:delete");

	// �ͷ�conn �Լ� conn->other
	conn->delete_other();
	delete conn;

	// �ͷ��׽���
	if (proxy_server->EraseConn(fd)) {
		// ɾ���ɹ���˵��fdΪ����ˣ�sfdΪ�ͻ��ˣ�ɾ��
		delete_socket_conn(sfd);
	} else {
		proxy_server->EraseConn(sfd);
		delete_socket_conn(fd);
	}

	// conn_mapɾ��
	conn_map.erase(fd);
	conn_map.erase(sfd);

	// std::cout << "�����ж�" << std::endl;
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

	// ���õ�ַ����
	int opt = 1;
	proxy_server->setopt(SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
	// ��������Ϊ��������ʽ
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
	// ����accept����
	int conn_sock = proxy_server->Accept();
	if (conn_sock == -1) {
		// �������´��󣬱���ͻ���ֹ���ӡ����źű������
		if (errno != EAGAIN && errno != ECONNABORTED && errno != EWOULDBLOCK &&
			errno != EPROTO && errno != EINTR) {
			perror("accept");
			LogC::log_fatal("%s:%d %s\n", __FILE__, __LINE__, strerror(errno));
			exit(EXIT_FAILURE);
		}
	} else {
		// ��ӡ���ӵ�client�˵���Ϣ
		struct sockaddr_in address;
		char client_ip[INET_ADDRSTRLEN] = {0};
		address = proxy_server->GetConn();
		inet_ntop(AF_INET, &address.sin_addr, client_ip, sizeof(client_ip));
		std::cout << "connect " << client_ip << ":" << ntohs(address.sin_port)
				  << std::endl;
		LogC::log_printf("connect %s:%d\n", client_ip, ntohs(address.sin_port));

		// ���÷�����
		Anakin::SetSocketBlockingEnable(conn_sock, false);
	}

	return conn_sock;
}

int Tcp_Proxy::connect_proxied_server(int myport) {
	/* ������Ϊ������������Ӳ�����վ */
	auto client_socket = new Anakin::Socket_Connect(AF_INET, SOCK_STREAM, 0);

	// ����Ϊ��������ʽ
	client_socket->SetSocketBlockingEnable(false);

	struct sockaddr_in client_addr;

	// ��� myport �ڲ���������
	if (myport != -1) {
		// bind client�˿�
		client_addr.sin_family = AF_INET;
		client_addr.sin_addr.s_addr = INADDR_ANY;
		client_addr.sin_port = htons(myport);
		client_socket->Bind((struct sockaddr *)&client_addr,
							sizeof(client_addr));
	}

	// ��������ʽ����
	int ret = client_socket->Connect(proxy_ip, proxy_port, false);
	int sockfd = client_socket->getfd();

	if (ret == -1 && errno == EINPROGRESS) {
		// ���ڽ�������
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
			// ˵���Ѿ���д��ͨ��getpeername�ж�
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