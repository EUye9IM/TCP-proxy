#ifndef SERVER_TOP
#define SERVER_TOP

#include "connection.h"
#include "mysocket.hpp"

#include <map>
#include <set>
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
		Anakin::Socket_Accept* proxy_server;	// ���ڽ��տͻ���
		int epfd;		// epoll�¼���ʶ��
		void Run();		// ���к���
		void new_connection();	// �����µ�connection������epoll
		void delete_connection();	// ɾ�����Ӳ���epollɾ��

	};
	inline void Tcp_Proxy::Run()
	{
		while (1) {
			// epoll �¼�����
			/**************************************
			connection *conn = ptr;
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
};
#endif