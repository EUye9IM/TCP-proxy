#ifndef SERVER_CONNECTION
#define SERVER_CONNECTION
#include <cstddef>
#include <set>

// // �ɵ�Connection���
// namespace Old {

// class Sending;
// class Connection {
// public:
// 	/**
// 	 * @brief ����һ��������
// 	 * @param fd_server ����������� socket
// 	 * @param fd_client �ͻ��� socket
// 	 * @param fd_epoll epoll ������
// 	 * @return
// 	 **/
// 	Connection(int fd_server, int fd_client, int fd_epoll);

// 	/**
// 	 * @brief �ر� socket �� �Ƴ�
// 	 * @param fd_server ����������� socket
// 	 * @param fd_client �ͻ��� socket
// 	 * @param fd_epoll epoll ������
// 	 * @return
// 	 **/
// 	~Connection();
// 	/**
// 	 * @brief ��ñ���������� socket
// 	 * @return ����������� socket
// 	 **/
// 	int getfdServer();
// 	/**
// 	 * @brief ��ÿͻ��� socket
// 	 * @return �ͻ��� socket
// 	 **/
// 	int getfdClient();

// private:
// 	int _fd_s;	// ����������� socket
// 	int _fd_c;	// �ͻ��� socket
// 	int _fd_ep; // epoll ������
// 	Sending _server_to_client;
// 	Sending _client_to_server;
// 	/**
// 	 * @brief �� ����������� ���͵� �ͻ���
// 	 * @return �����ֽڸ������� 0 �Ļ� top ��Ҫ�ر�������
// 	 **/
// 	int _sendS2C();
// 	/**
// 	 * @brief �� �ͻ��� ���͵� �����������
// 	 * @return �����ֽڸ������� 0 �Ļ� top ��Ҫ�ر�������
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

// �µ�Connection���
namespace New {
struct buffer {
	int pipe[2];
	size_t len;
};
// Connect����Ϊ�������ӣ��������ڲ�ָ���Ӧ��һ��
class Connection {
public:
	Connection(int _fd);
	~Connection();

	/* connection��ʼ�� */
	void init_connection();
	Connection *build_connection(int sfd); // ��fd��sfd֮�佨������
	
	/* ɾ��other */
	void delete_other();
	void close_pipes();	  // �ر�pipe
	bool write_to_buf();  // fd��bufд����
	bool read_from_buf(); // ��other->buf��������fd
	int get_fd();		  // ��ȡfd
	Connection* get_other();	// ��ȡother

private:
	static const size_t BUF_SIZE = 1024;
	Connection *other; // ���ӵ���һ�� 	this->other other->this
	int fd;
	struct buffer buf; // bufferҲ�ǵ���ģ�����Ϊfdд��buf
};

}; // namespace New
#endif