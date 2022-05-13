#ifndef CLIENT_HTTP
#define CLIENT_HTTP
#include <map>
#include <string>
#include <vector>

using Header = std::map<std::string, std::string>;
using Byteset = std::vector<char>;

enum class METHOD { GET, POST, HEAD, PUT, DELETE, TRACE, CONNECT, OPTIONS };
class Http {
public:
	Http(int fd);
	~Http();
	// function
	int method(METHOD type, Byteset &data, const char *URL = "/",
			   Header header = Header{});
	int method(METHOD type, std::string str, const char *URL = "/",
			   Header header = Header{});
	int recv();

	// cookie
	std::string getCookie();
	void addCookie(Header cookie);
	void clearCookie();

	// result
	int result_code();
	std::string result_msg();
	Header result_header();
	std::string result_data();

	// others
	void set_timeout(int sec);

public:
	static const int BUF_SIZE = 1024;
	int _fd;
	int timeout;
	int ep;

	Byteset _res;
	int _res_code;
	std::string _res_msg;
	Header _res_header;
	std::string _res_data;
	std::string _cookie;

	int _send(Byteset &data);
	void _res_parse();
};

#endif