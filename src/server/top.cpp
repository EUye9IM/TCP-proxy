#include <netinet/in.h>
#include <sys/epoll.h>

#include "mysocket.hpp"
#include "top.h"

namespace Old {
void Top::run() {
	while (1) {

		// epoll

		// �� fd ��顣
		//    Connection::run()
		// or delete Connection
		// or accept(); new Connection

		// ����� connection
		// run��fd_set���Ƴ����fd���ٵ���Connection::fdSet()��ȡ�µ�
		// �������Ҳ�� fd_set �ı䶯

		// �� fd_set_read �� fd_set_write ����epoll�õĶ���
	}
}
} // namespace Old
namespace New {

Tcp_Proxy::Tcp_Proxy(std::string _proxy_ip, int _proxy_port, int _port)
	: port(_port), proxy_ip(_proxy_ip), proxy_port(_proxy_port) {
	init_proxy_server();
	create_epoll();
}

void Tcp_Proxy::Run() {
	while (1) {
		int nfds =
			epoll_wait(epfd, events, MAX_EVENTS, 0); // ��ʱ��Ϊ0�����̷���
		if (nfds == -1) {
			// ���ź��ж�
			if (errno == EINTR)
				continue;
			perror("epoll_wait");
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < nfds; ++i) {
			// �¼��ɶ�
			if (events[i].events & EPOLLIN) {
				// listenfd ���Ӵ���
				if (events[i].data.fd == proxy_server->getfd()) {
					new_connection();
					this->connect_proxied_server();
				}
			}
		}
		// epoll �¼�����
		/**************************************
		Connection *conn = ptr;
		if (�ɶ��¼�)
			if (fd == listenfd)
				new_connection();
			else
				conn->write_buf();
				...������

		if (��д�¼�)
			conn->read_buf();	// buf����������fd

		*************************************/
	}
}

void Tcp_Proxy::create_epoll() {
	/* ���洴�� epoll ʵ�� */
	struct epoll_event ev;
	epfd = epoll_create1(0);
	Anakin::checkerror(epfd, "epoll_create1");

	int listenfd = proxy_server->getfd();
	ev.data.fd = listenfd; // listenfd
	ev.events = EPOLLIN;   // ʹ��ˮƽ����ģʽ

	// ����epoll�¼�
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
	Anakin::checkerror(ret, "epoll_ctl");
}

void Tcp_Proxy::new_connection() {
	// ����accept����
	int conn_sock = proxy_server->Accept();
	if (conn_sock == -1) {
		// �������´��󣬱���ͻ���ֹ���ӡ����źű������
		if (errno != EAGAIN && errno != ECONNABORTED && errno != EWOULDBLOCK &&
			errno != EPROTO && errno != EINTR) {
			perror("accept");
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

		// ���÷�����
		Anakin::SetSocketBlockingEnable(conn_sock, false);

		// ����һ���� connection
		Connection *conn_client = new Connection(conn_sock);
		if (conn_client == NULL) {
			std::cerr << "new error!" << std::endl;
			exit(EXIT_FAILURE);
		}
		// todo: connect �� �����������
		Anakin::Socket_Connect *sock_server =
			new Anakin::Socket_Connect(AF_INET, SOCK_STREAM, 0); 
		sock_server->Connect(proxy_ip, proxy_port,false);
		conns.push_back(sock_server);
		int fd_server = sock_server->getfd();
		Connection *conn_server = conn_client->build_connection(fd_server);

		// ����epoll
		struct epoll_event ev;
		ev.data.ptr = conn_client;
		ev.events = EPOLLIN;
		int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, conn_sock, &ev);
		Anakin::checkerror(ret, "epoll_ctl:conn_sock");
		ev.data.ptr = conn_server;
		int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd_server, &ev);
		Anakin::checkerror(ret, "epoll_ctl:conn_sock");
	}
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
	proxy_server->Listen(3);
}

Anakin::Socket_Connect *Tcp_Proxy::connect_proxied_server(int myport) {
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
			exit(EXIT_FAILURE);
		}
		if (FD_ISSET(sockfd, &set)) {
			// ˵���Ѿ���д��ͨ��getpeername�ж�
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

}; // namespace New