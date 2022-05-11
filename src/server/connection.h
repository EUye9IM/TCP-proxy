#ifndef SERVER_CONNECTION
#define SERVER_CONNECTION
#include <set>

class Connection {
public:
	/**
	 * @brief 建立一个新连接
	 * @param fd_client
	 * @return
	 **/
	Connection(int fd_server, int fd_client);
	~Connection();
	/**
	 * @brief 进入活动状态。此时可以发送数据
	 * @return
	 **/
	void run();
	/**
	 * @brief 向 set 中添加自己需要监听的 socket 与监听方式
	 * @param fd_client
	 * @return
	 **/
	void fdSet(std::set<int> &fd_set_read, std::set<int> &fd_set_write);
	/**
	 * @brief 为了避免之后需要处理一个 Connection
	 *一直在读而其他读不了预留的接口（也许run返回后仍是活动状态）
	 * @return true 活动 false 空闲
	 **/
	bool isActive();
	/**
	 * @brief 获得被代理服务器 socket
	 * @return 被代理服务器 socket
	 **/
	int getfdServer();
	/**
	 * @brief 获得客户端 socket
	 * @return 客户端 socket
	 **/
	int getfdClient();

private:
	static const int BUF_SIZE = 1024; // 缓冲区大小（可改
	char _buf[BUF_SIZE];	   // 缓冲区。避免重复申请
	int _fd_s;				   // 被代理服务器 socket
	int _fd_c;				   // 客户端 socket
};

#endif