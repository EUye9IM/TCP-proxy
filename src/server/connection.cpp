#include "connection.h"
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>
namespace Old {
void Sending::set(Connection *connection, int (Connection::*func)()) {
	_conn = connection;
	_func = func;
}
int Sending::send() { return (_conn->*_func)(); }

Connection::Connection(int fd_server, int fd_client, int fd_epoll)
	: _fd_s(fd_server), _fd_c(fd_client), _fd_ep(fd_epoll) {
	//��ʼ�� Sending
	_client_to_server.set(this, Connection::_sendC2S);
	_server_to_client.set(this, Connection::_sendS2C);

	// ע�� epoll
	struct epoll_event ev;
	ev.events = EPOLLIN; // ʹ��ˮƽ����ģʽ
	ev.data.ptr = &(this->_client_to_server);
	int ret = epoll_ctl(_fd_ep, EPOLL_CTL_ADD, _fd_c, &ev);
	if (ret < 0) {
		throw "epoll_ctl eror";
	}
	ev.data.ptr = &(this->_server_to_client);
	int ret = epoll_ctl(_fd_ep, EPOLL_CTL_ADD, _fd_s, &ev);
	if (ret < 0) {
		throw "epoll_ctl eror";
	}
}
} // namespace Old
namespace New {

Connection::Connection(int _fd) : fd(_fd) { init_connection(); }

Connection::~Connection() {
	close_pipes();
	other->close_pipes();
	// �˴���otherҲ�ͷ�
	delete other;
	other = NULL;
}

void Connection::init_connection() {
	// ���ȴ����ܵ�
	if (pipe2(buf.pipe, O_NONBLOCK) < 0) {
		perror("pipe2");
		exit(EXIT_FAILURE);
	}
	// ��ʼ������
	buf.len = 0;
}

Connection *Connection::build_connection(int sfd) {
	// ��sfd�γ�����
	this->other = new Connection(sfd);
	if (this->other == NULL) {
		std::cerr << "new error!" << std::endl;
		exit(EXIT_FAILURE);
	}
	other->other = this;
	return other;
}

/* ��other->buf��������fd */
bool Connection::read_from_buf() {
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

/* fd��bufд���� */
bool Connection::write_to_buf() {
	for (;;) {
		ssize_t n = splice(this->fd, NULL, this->buf.pipe[1], NULL, BUF_SIZE,
						   SPLICE_F_NONBLOCK | SPLICE_F_MOVE);

		if (n > 0)
			this->buf.len += n;
		if (n == 0)
			break;

		if (n < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return true;
			return false;
		}
	}

	return true;
}
void Connection::close_pipes() {
	close(buf.pipe[0]);
	close(buf.pipe[1]);
}
}; // namespace New