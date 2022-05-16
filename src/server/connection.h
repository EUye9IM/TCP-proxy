#ifndef SERVER_CONNECTION
#define SERVER_CONNECTION
#include <cstddef>
#include <set>


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
	void build_connection(int sfd); // 在fd和sfd之间建立连接
	
	/* 删除other */
	void delete_other();
	void close_pipes();	  // 关闭pipe
	bool write_to_buf();  // fd向buf写数据
	bool read_from_buf(); // 从other->buf读数据至fd
	const int get_fd();		  // 获取fd
	Connection*& get_other();	// 获取other，此处返回引用值，注意！

private:
	static const size_t BUF_SIZE = 1024;
	Connection *other; // 连接的另一端 	this->other other->this
	int fd;
	struct buffer buf; // buffer也是单向的，方向为fd写入buf
};

}; // namespace New
#endif