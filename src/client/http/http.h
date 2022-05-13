#ifndef CLIENT_HTTP
#define CLIENT_HTTP
#include <map>
#include <string>
#include <vector>

typedef std::map<std::string, std::string> Header;
typedef std::vector<char> Byteset;

enum class METHOD { GET, POST, HEAD, PUT, DELETE, TRACE, CONNECT, OPTIONS };
class Http {
public:
	Http(int fd);
	// function
	int method(METHOD type = METHOD::GET, const char *URL = "/",
			   Header header = Header{}, Byteset data = Byteset{});

	// cookie
	std::string getCookie();
	std::string addCookie(Header cookie);
	std::string clearCookie();

	// result
	const Byteset &result_raw();
	int result_code();
	std::string result_msg();
	Header result_header();

	// others
	void set_timeout(int sec);

private:
	int _fd;
	int timeout;
	Byteset _res;
	Header _cookie;

	int _res_code;
	std::string _res_msg;
	Header res_header;

	void _res_parse();
};

#endif