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
// public: // 可能需要其他参数
// 	Top();
// 	~Top();
// 	/**
// 	 * @brief 运行代理服务
// 	 * @param
// 	 * @return
// 	 **/
// 	void run();

// private:
// 	/**
// 	 * @brief accept 了一个新连接
// 	 * @param fd accept 的新 socket
// 	 * @return
// 	 **/
// 	void _newConnection(int fd);
// 	/**
// 	 * @brief close 了一个连接
// 	 * @param fd 收到 close 的 socket
// 	 * @return
// 	 **/
// 	void _deleteConnection(int fd);
// 	std::set<Connection *> _connections; // connection 的集合
// 	// std::map<int, Connection*>//socket 到 Connection 的映射
// 	// Log _log; // 可能有的 log 类
// };
// }

namespace New {
	class Tcp_Proxy {
	public:
		Tcp_Proxy(std::string _proxy_ip, int _proxy_port, int _port);
		~Tcp_Proxy();

		
		void Run();		// 运行函数
		

	private:
		static const size_t MAX_EVENTS = 10240;

		/* 初始化代理服务器 */
		void init_proxy_server();

		/* accept连接，return 新的socket fd */
		int accept_new_socket();

		/* 连接被代理服务器，返回fd */
		int connect_proxied_server(int=-1);
		
		/* 创建epoll事件 */
		void create_epoll();

		/* epoll 中添加listenfd */
		void epoll_add_listenfd();

		/* 创建新的connection并加入epoll */
		void new_connection();	

		/* 删除连接并从epoll删除 */
		void close_connection(Connection* conn);	

		/* 删除服务器与代理服务器的连接，并从conns中移除 */
		int delete_socket_conn(int fd);

		int port;				// 处理的端口
		std::string proxy_ip;	// 代理IP地址
		int proxy_port;			// 代理端口
		Anakin::Socket_Accept* proxy_server;	// 用于接收客户端
		int epfd;				// epoll事件标识符
		struct epoll_event events[MAX_EVENTS];

		// 存储服务器与代理服务器的连接
		std::vector<Anakin::Socket_Connect*> conns;	
	};

};
#endif