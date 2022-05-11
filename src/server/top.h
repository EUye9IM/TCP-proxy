#ifndef SERVER_TOP
#define SERVER_TOP

#include "connection.h"

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

#endif