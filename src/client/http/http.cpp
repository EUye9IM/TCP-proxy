#include "http.h"

#include <iterator>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

static const char CRLF[] = {0x0D, 0x0A, 0};
static const char CRLFCRLF[] = {0x0D, 0x0A, 0x0D, 0x0A, 0};
static const char HTTP_VERSION[] = "HTTP/1.1";
static const char DEVIDE[] = " ";
static const int ER = -1;
static const int OK = 0;

Http::Http(int fd) : _fd(fd) {
	timeout = 30;
	ep = epoll_create1(0);
	if (ep == -1) {
		throw "epoll_create error";
	}
	epoll_event ev;
	ev.events = EPOLLOUT;
	ev.data.fd = _fd;
	if (epoll_ctl(ep, EPOLL_CTL_ADD, _fd, &ev) == -1) {
		throw "epoll_ctl error";
	}
}
Http::~Http() { close(ep); }

static void bytesetAppend(Byteset &b, const char *s) {
	const char *p = s;
	while (p != nullptr && *p != 0) {
		b.push_back(*p);
		p++;
	}
	return;
}
static void bytesetAppend(Byteset &b, const char *s, int n) {
	const char *p = s;
	while (p - s < n) {
		b.push_back(*p);
		p++;
	}
	return;
}
static void bytesetAppend(Byteset &b, const Header &h) {
	auto p = begin(h);
	while (p != end(h)) {
		bytesetAppend(b, (*p).first.c_str());
		bytesetAppend(b, ": ");
		bytesetAppend(b, (*p).second.c_str());
		bytesetAppend(b, CRLF);
		p++;
	}
	return;
}
int Http::method(METHOD type, std::string str, const char *URL, Header header) {
	Byteset data;
	bytesetAppend(data, str.c_str());
	return this->method(type, data, URL, header);
}
int Http::method(METHOD type, Byteset &data, const char *URL, Header header) {
	static Byteset content;
	content.clear();
	switch (type) {
	case METHOD::GET:
		bytesetAppend(content, "GET");
		break;
	case METHOD::POST:
		bytesetAppend(content, "POST");
		break;
	default:
		return ER;
	}
	bytesetAppend(content, DEVIDE);
	bytesetAppend(content, URL);
	bytesetAppend(content, DEVIDE);
	bytesetAppend(content, HTTP_VERSION);
	bytesetAppend(content, CRLF);
	bytesetAppend(content, header);

	if (_cookie.size() > 1) {
		bytesetAppend(content, "Cookie:");
		bytesetAppend(content, _cookie.c_str());
		bytesetAppend(content, CRLF);
	}

	bytesetAppend(content, CRLF);
	content.insert(content.end(), data.begin(), data.end());

	return _send(content);
}

// cookie
std::string Http::getCookie() { return _cookie; }
void Http::addCookie(Header cookie) {
	auto p = begin(cookie);
	while (p != end(cookie)) {
		_cookie += " " + (*p).first + "=" + (*p).second + ";";
		p++;
	}
}
void Http::clearCookie() { _cookie = ""; }

// result
int Http::result_code() { return _res_code; }
std::string Http::result_msg() { return _res_msg; }
Header Http::result_header() { return _res_header; }
std::string Http::result_data() { return _res_data; }

// others
void set_timeout(int sec);
void Http::set_timeout(int sec) { timeout = sec; }
int Http::_send(Byteset &data) {
	epoll_event ev;
	ev.events = EPOLLOUT;
	ev.data.fd = _fd;
	if (epoll_ctl(ep, EPOLL_CTL_MOD, _fd, &ev) == -1)
		return ER;

	if (epoll_wait(ep, &ev, 1, timeout) <= 0)
		return ER;

	int ret = write(_fd, data.data(), data.size());
	if (ret < (int)data.size())
		return ER;
	bytesetAppend(data, "", 1);

	// printf("==============\n%s\n", data.data());
	return ret;
}
int Http::recv() {
	static char _buf[BUF_SIZE];
	epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = _fd;
	if (epoll_ctl(ep, EPOLL_CTL_MOD, _fd, &ev) == -1)
		return ER;

	if (epoll_wait(ep, &ev, 1, timeout) <= 0)
		return ER;

	int cnt = 0;
	_res.clear();
	while (1) {
		int ret = read(_fd, _buf, BUF_SIZE);
		if (ret <= 0) {
			break;
		}
		cnt += ret;
		bytesetAppend(_res, _buf, ret);

		if (epoll_wait(ep, &ev, 1, 1) <= 0)
			break;
	}

	_res_parse();

	return cnt;
}
void Http::_res_parse() {
	string res(_res.begin(), _res.end());

	// printf("==============\n%s\n",res.c_str());

	string head_part = res.substr(0, res.find(CRLFCRLF));
	_res_data = res.substr(res.find(CRLFCRLF) + 4);

	int pos = res.find(DEVIDE);			  // 200 OK ..
	int pos2 = res.find(DEVIDE, pos + 1); // OK crlf ...
	_res_code = stoi(res.substr(pos + 1, pos2 - pos - 1));
	_res_msg = res.substr(pos2 + 1, res.find(CRLF) - pos2 - 1);

	pos = head_part.find(CRLF) + 2;
	_res_header.clear();

	while (pos < (int)head_part.size()) {
		pos2 = head_part.find(":", pos + 1);
		int pos3 = head_part.find(CRLF, pos2 + 1);
		if (pos3 == -1)
			pos3 = head_part.size();
		_res_header[head_part.substr(pos, pos2 - pos)] =
			head_part.substr(pos2 + 1, pos3 - pos2 - 1);

		pos = pos3 + 2;
	}
	if (_res_header.count("Set-Cookie"))
		_cookie = _res_header["Set-Cookie"];
	return;
}