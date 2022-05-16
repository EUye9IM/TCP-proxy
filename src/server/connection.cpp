#include "connection.h"
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <logc/logc.h>
#include <sys/epoll.h>
#include <unistd.h>

namespace New {

Connection::Connection(int _fd) : fd(_fd) { init_connection(); }

Connection::~Connection() {
	close_pipes();
}

void Connection::delete_other()
{
	if (other == NULL)
		return;
	other->close_pipes();
	delete other;
	other = NULL;
}

void Connection::init_connection() {
	// 首先创建管道
	if (pipe2(buf.pipe, O_NONBLOCK) < 0) {
		perror("pipe2");
		LogC::log_fatal("%s:%d %s\n", __FILE__, __LINE__, strerror(errno));
		exit(EXIT_FAILURE);
	}
	// 初始化长度
	buf.len = 0;

	// NULL
	other = NULL;
}

const int Connection::get_fd() { return fd; }

size_t Connection::get_count()
{
	return _count;
}

// 返回引用的设计只是为了删除操作的维护
Connection *&Connection::get_other() { return other; }

void Connection::build_connection(int sfd) {
	// 与sfd形成连接
	this->other = new Connection(sfd);
	if (this->other == NULL) {
		std::cerr << "new error!" << std::endl;
		LogC::log_fatal("%s:%d %s\n", __FILE__, __LINE__, strerror(errno));
		exit(EXIT_FAILURE);
	}
	other->other = this;
}

/* 从other->buf读数据至fd */
bool Connection::read_from_buf() {
	if (other == NULL) {
		return false;
	}

	while (other->buf.len > 0) {
		ssize_t n = splice(other->buf.pipe[0], NULL, this->fd, NULL, BUF_SIZE,
						   SPLICE_F_NONBLOCK | SPLICE_F_MOVE);
		if (n == 0)
			break;
		if (n < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			return false;
		}
		other->buf.len -= n;
	}

	return true;
}

/* fd向buf写数据 */
bool Connection::write_to_buf() {
	// 添加判断
	if (other == NULL)
		return false;;
	
	ssize_t cnt = 0;	// 记录本次从fd中读取的数据
	for (;;) {
		ssize_t n = splice(this->fd, NULL, this->buf.pipe[1], NULL, BUF_SIZE,
						   SPLICE_F_NONBLOCK | SPLICE_F_MOVE);

		if (n > 0) {
			this->buf.len += n;
			cnt += n;
			_count += n;
		}
			
		if (n == 0)
			break;

		if (n < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return true;
			return false;
		}
	}
	// 说明连接断开
	if (cnt == 0)
		return false;
	return true;
}

void Connection::close_pipes() {
	close(buf.pipe[0]);
	close(buf.pipe[1]);
}
}; // namespace New