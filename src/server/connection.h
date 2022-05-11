#ifndef SERVER_CONNECTION
#define SERVER_CONNECTION
#include <set>

class Connection {
public:
	/**
	 * @brief ����һ��������
	 * @param fd_client
	 * @return
	 **/
	Connection(int fd_server, int fd_client);
	~Connection();
	/**
	 * @brief ����״̬����ʱ���Է�������
	 * @return
	 **/
	void run();
	/**
	 * @brief �� set ������Լ���Ҫ������ socket �������ʽ
	 * @param fd_client
	 * @return
	 **/
	void fdSet(std::set<int> &fd_set_read, std::set<int> &fd_set_write);
	/**
	 * @brief Ϊ�˱���֮����Ҫ����һ�� Connection
	 *һֱ�ڶ�������������Ԥ���Ľӿڣ�Ҳ��run���غ����ǻ״̬��
	 * @return true � false ����
	 **/
	bool isActive();
	/**
	 * @brief ��ñ���������� socket
	 * @return ����������� socket
	 **/
	int getfdServer();
	/**
	 * @brief ��ÿͻ��� socket
	 * @return �ͻ��� socket
	 **/
	int getfdClient();

private:
	static const int BUF_SIZE = 1024; // ��������С���ɸ�
	char _buf[BUF_SIZE];	   // �������������ظ�����
	int _fd_s;				   // ����������� socket
	int _fd_c;				   // �ͻ��� socket
};

// �µ�Connection���
namespace New {
	struct buffer {
		int pipe[2];
		size_t len;
	};
	// Connect����Ϊ�������ӣ��������ڲ�ָ���Ӧ��һ��
	class Connection {
	public:
		Connection* other;	// ���ӵ���һ�� 	this->other other->this
		int fd;
		struct buffer;		// bufferҲ�ǵ���ģ�����Ϊfdд��buf
		
		void build_connection(int fd, int sfd);	// ��fd��sfd֮�佨������
		void delete_connection(int fd);		// ɾ��fd��Ӧ�����ӣ�����otherָ���
		bool write_buf();	// fd��bufд����
		bool read_buf();	// ��other->buf��������fd
	};

};
#endif