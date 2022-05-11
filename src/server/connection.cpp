#include "connection.h"
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

namespace New {

Connection::Connection(int _fd) : fd(_fd)
{
    init_connection();
}

Connection::~Connection()
{
    close_pipes();
    other->close_pipes();
    // �˴���otherҲ�ͷ�
    delete other;
    other = NULL;
}

void Connection::init_connection()
{
    // ���ȴ����ܵ�
    if (pipe2(buf.pipe, O_NONBLOCK) < 0) {
        perror("pipe2");
        exit(EXIT_FAILURE);
    }
    // ��ʼ������
    buf.len = 0;
}

void Connection::build_connection(int sfd)
{
    // ��sfd�γ�����
    this->other = new Connection(sfd);
    if (this->other == NULL) {
        std::cerr << "new error!" << std::endl;
        exit(EXIT_FAILURE);
    }
    other->other = this;

}

/* ��other->buf��������fd */
bool Connection::read_from_buf()
{
    while (other->buf.len > 0) {
        ssize_t n = splice(other->buf.pipe[0], NULL, this->fd, NULL,
                    BUF_SIZE, SPLICE_F_NONBLOCK | SPLICE_F_MOVE);
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
bool Connection::write_to_buf()
{
    for (;;) {
        ssize_t n = splice(this->fd, NULL, this->buf.pipe[1], NULL,
                    BUF_SIZE, SPLICE_F_NONBLOCK | SPLICE_F_MOVE);
        
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

void Connection::close_pipes()
{
    close(buf.pipe[0]);
    close(buf.pipe[1]);
}

};