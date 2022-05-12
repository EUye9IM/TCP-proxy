#ifndef SERVER_CONNECTION
#define SERVER_CONNECTION
#include <cstddef>
#include <set>

// // 旧的Connection设计
// namespace Old {

// class Sending;
// class Connection {
// public:
// 	/**
// 	 * @brief 建立一个新连接
// 	 * @param fd_server 被代理服务器 socket
// 	 * @param fd_client 客户端 socket
// 	 * @param fd_epoll epoll 描述符
// 	 * @return
// 	 **/
// 	Connection(int fd_server, int fd_client, int fd_epoll);

// 	/**
// 	 * @brief 关闭 socket 并 移除
// 	 * @param fd_server 被代理服务器 socket
// 	 * @param fd_client 客户端 socket
// 	 * @param fd_epoll epoll 描述符
// 	 * @return
// 	 **/
// 	~Connection();
// 	/**
// 	 * @brief 获得被代理服务器 socket
// 	 * @return 被代理服务器 socket
// 	 **/
// 	int getfdServer();
// 	/**
// 	 * @brief 获得客户端 socket
// 	 * @return 客户端 socket
// 	 **/
// 	int getfdClient();

// private:
// 	int _fd_s;	// 被代理服务器 socket
// 	int _fd_c;	// 客户端 socket
// 	int _fd_ep; // epoll 描述符
// 	Sending _server_to_client;
// 	Sending _client_to_server;
// 	/**
// 	 * @brief 从 被代理服务器 发送到 客户端
// 	 * @return 发送字节个数。是 0 的话 top 就要关闭连接了
// 	 **/
// 	int _sendS2C();
// 	/**
// 	 * @brief 从 客户端 发送到 被代理服务器
// 	 * @return 发送字节个数。是 0 的话 top 就要关闭连接了
// 	 **/
// 	int _sendC2S();
// };

// class Sending{
// public:
// 	void set(Connection *connection, int (Connection::*func)());
// 	int send();
// private:
// 	Connection *_conn;
// 	int (Connection::*_func)();
// };
// } // namespace Old

// 新的Connection设计
namespace New {
struct buffer {
	int pipe[2];
	size_t len;
};
// Connect本身为单向连接，但是其内部指针对应另一端
class Connection {
public:
	Connection(int _fd);
	~Connection();

	/* connection初始化 */
	void init_connection();
	Connection *build_connection(int sfd); // 在fd和sfd之间建立连接
	// void delete_connection();		// 删除fd对应的连接，包括other指向的
	void close_pipes();	  // 关闭pipe
	bool write_to_buf();  // fd向buf写数据
	bool read_from_buf(); // 从other->buf读数据至fd
	int get_fd();		  // 获取fd
	Connection* get_other();	// 获取other

private:
	static const size_t BUF_SIZE = 1024;
	Connection *other; // 连接的另一端 	this->other other->this
	int fd;
	struct buffer buf; // buffer也是单向的，方向为fd写入buf
};

}; // namespace New
#endif