#ifndef SERVER_CONNECTION
#define SERVER_CONNECTION
#include <cstddef>
#include <set>


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
	void build_connection(int sfd); // ��fd��sfd֮�佨������
	
	/* ɾ��other */
	void delete_other();
	void close_pipes();	  // �ر�pipe
	bool write_to_buf();  // fd��bufд����
	bool read_from_buf(); // ��other->buf��������fd
	const int get_fd();		  // ��ȡfd
	Connection*& get_other();	// ��ȡother���˴���������ֵ��ע�⣡

private:
	static const size_t BUF_SIZE = 1024;
	Connection *other; // ���ӵ���һ�� 	this->other other->this
	int fd;
	struct buffer buf; // bufferҲ�ǵ���ģ�����Ϊfdд��buf
};

}; // namespace New
#endif