#ifndef SERVER_TOP
#define SERVER_TOP

#include "connection.h"
#include "mysocket.hpp"

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


namespace New {
	class Tcp_Proxy {
	public:
		Anakin::Socket_Accept* proxy_server;	// 用于接收客户端
		int epfd;		// epoll事件标识符
		void Run();		// 运行函数
		void new_connection();	// 创建新的connection并加入epoll
		void delete_connection();	// 删除连接并从epoll删除

	};
	inline void Tcp_Proxy::Run()
	{
		while (1) {
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
};
#endif