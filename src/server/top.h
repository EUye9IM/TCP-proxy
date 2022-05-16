#ifndef SERVER_TOP
#define SERVER_TOP

#include "connection.h"
#include "mysocket.hpp"

#include <cstddef>
#include <map>
#include <set>
#include <sys/epoll.h>

namespace New {
	class Tcp_Proxy {
	public:
		Tcp_Proxy(std::string _proxy_ip, int _proxy_port, int _port);
		~Tcp_Proxy();

		
		void Run();		// ���к���
		

	private:
		static const size_t MAX_EVENTS = 10240;

		/* ��ʼ����������� */
		void init_proxy_server();

		/* accept���ӣ�return �µ�socket fd */
		int accept_new_socket();

		/* ���ӱ����������������fd */
		int connect_proxied_server(int=-1);
		
		/* ����epoll�¼� */
		void create_epoll();

		/* epoll �����listenfd */
		void epoll_add_listenfd();

		/* �����µ�connection������epoll */
		void new_connection();	

		/* ɾ��_fd����ʾ�����Ӳ���epollɾ�� */
		void close_connection(int _fd);	

		/* ɾ�����������������������ӣ�����conns���Ƴ� */
		int delete_socket_conn(int _fd);

		/* �ж�ĳһ��fd�Ƿ���� */
		bool is_exist(int _fd);

		int port;				// ����Ķ˿�
		std::string proxy_ip;	// ����IP��ַ
		int proxy_port;			// ����˿�
		Anakin::Socket_Accept* proxy_server;	// ���ڽ��տͻ���
		int epfd;				// epoll�¼���ʶ��
		struct epoll_event events[MAX_EVENTS];

		// �洢����������������������
		std::vector<Anakin::Socket_Connect*> conns;	

		// ��Ҫά��һ��fd->connection*��map��������Ҫʹ��ָ���ָ��
		std::map<int, Connection*> conn_map;
	};

};
#endif