#ifndef SERVER_TOP
#define SERVER_TOP

#include "connection.h"

#include <map>
#include <set>
class Top {
public: // 可能需要其他参数
	Top();
	~Top();
	/**
	 * @brief 运行代理服务
	 * @param
	 * @return
	 **/
	void run();

private:
	/**
	 * @brief accept 了一个新连接
	 * @param fd accept 的新 socket
	 * @return
	 **/
	void _newConnection(int fd);
	/**
	 * @brief close 了一个连接
	 * @param fd 收到 close 的 socket
	 * @return
	 **/
	void _deleteConnection(int fd);
	std::set<Connection *> _connections; // connection 的集合
	// std::map<int, Connection*>//socket 到 Connection 的映射
	// Log _log; // 可能有的 log 类
};

#endif