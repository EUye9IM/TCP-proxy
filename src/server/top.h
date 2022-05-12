#ifndef SERVER_TOP
#define SERVER_TOP

#include "connection.h"
#include "mysocket.hpp"

#include <cstddef>
#include <map>
#include <set>
#include <sys/epoll.h>

// namespace Old {
// class Top {
// public: // ������Ҫ��������
// 	Top();
// 	~Top();
// 	/**
// 	 * @brief ���д������
// 	 * @param
// 	 * @return
// 	 **/
// 	void run();

// private:
// 	/**
// 	 * @brief accept ��һ��������
// 	 * @param fd accept ���� socket
// 	 * @return
// 	 **/
// 	void _newConnection(int fd);
// 	/**
// 	 * @brief close ��һ������
// 	 * @param fd �յ� close �� socket
// 	 * @return
// 	 **/
// 	void _deleteConnection(int fd);
// 	std::set<Connection *> _connections; // connection �ļ���
// 	// std::map<int, Connection*>//socket �� Connection ��ӳ��
// 	// Log _log; // �����е� log ��
// };
// }

namespace New {
	class Tcp_Proxy {
	public:
		Tcp_Proxy(std::string _proxy_ip, int _proxy_port, int _port);
		~Tcp_Proxy();

		
		void Run();		// ���к���
		

	private:
		static const size_t MAX_EVENTS = 1024;

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

		/* ɾ�����Ӳ���epollɾ�� */
		void close_connection(Connection* conn);	

		/* ɾ�����������������������ӣ�����conns���Ƴ� */
		int delete_socket_conn(int fd);

		int port;				// ����Ķ˿�
		std::string proxy_ip;	// ����IP��ַ
		int proxy_port;			// ����˿�
		Anakin::Socket_Accept* proxy_server;	// ���ڽ��տͻ���
		int epfd;				// epoll�¼���ʶ��
		struct epoll_event events[MAX_EVENTS];

		// �洢����������������������
		std::vector<Anakin::Socket_Connect*> conns;	
	};

};
#endif