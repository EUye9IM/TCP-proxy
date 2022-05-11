#ifndef SERVER_TOP
#define SERVER_TOP

#include "connection.h"
#include "mysocket.hpp"

#include <cstddef>
#include <map>
#include <set>
#include <sys/epoll.h>
class Top {
public: // ������Ҫ��������
	Top();
	~Top();
	/**
	 * @brief ���д������
	 * @param
	 * @return
	 **/
	void run();

private:
	/**
	 * @brief accept ��һ��������
	 * @param fd accept ���� socket
	 * @return
	 **/
	void _newConnection(int fd);
	/**
	 * @brief close ��һ������
	 * @param fd �յ� close �� socket
	 * @return
	 **/
	void _deleteConnection(int fd);
	std::set<Connection *> _connections; // connection �ļ���
	// std::map<int, Connection*>//socket �� Connection ��ӳ��
	// Log _log; // �����е� log ��
};


namespace New {
	class Tcp_Proxy {
	public:
		Tcp_Proxy(std::string _proxy_ip, int _proxy_port, int _port);
		~Tcp_Proxy();

		
		void Run();		// ���к���
		void new_connection();		// �����µ�connection������epoll
		void delete_connection();	// ɾ�����Ӳ���epollɾ��

	private:
		static const size_t MAX_EVENTS = 1024;

		/* ��ʼ����������� */
		void init_proxy_server();
		/* ���ӱ���������� */
		Anakin::Socket_Connect* connect_proxied_server(int=-1);
		
		/* ����epoll�¼�*/
		void create_epoll();

		int port;		// ����Ķ˿�
		std::string proxy_ip;	// ����IP��ַ
		int proxy_port;			// ����˿�
		Anakin::Socket_Accept* proxy_server;	// ���ڽ��տͻ���
		int epfd;		// epoll�¼���ʶ��
		struct epoll_event events[MAX_EVENTS];

		// �洢����������������������
		std::vector<Anakin::Socket_Connect*> conns;	
	};

};
#endif