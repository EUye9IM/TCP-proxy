#ifndef SERVER_CONNECTION
#define SERVER_CONNECTION
#include <cstddef>
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
		Connection(int _fd);
		~Connection();
		
		/* connection��ʼ�� */
		void init_connection();
		void build_connection(int sfd);	// ��fd��sfd֮�佨������
		// void delete_connection();		// ɾ��fd��Ӧ�����ӣ�����otherָ���
		void close_pipes();		// �ر�pipe
		bool write_to_buf();	// fd��bufд����
		bool read_from_buf();	// ��other->buf��������fd
	
	private:
		static const size_t BUF_SIZE = 1024;
		Connection* other;	// ���ӵ���һ�� 	this->other other->this
		int fd;
		struct buffer buf;		// bufferҲ�ǵ���ģ�����Ϊfdд��buf
	};

};
#endif