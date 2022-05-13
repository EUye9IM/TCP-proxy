#include <agps/agps.h>
#include <agps/check.h>
#include <logc/logc.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include "http/http.h"

using namespace LogC;
using namespace std;
int main(int argc, char **argv) {
	log_open(stdout);
	log_set(0, LOG_FLAG_DATE);
	// LOG_FLAG_DEBUG
	agps::Parser p;
	p.add(agps::Type::FLAG, 'h', "help", "输出参数列表");
	p.add(agps::Type::STR, 'H', "ip", "代理服务器 IP 地址 [162.168.1.236]",
		  false, agps::Value{.Str = "192.168.1.236"}, CHECK_STR_IPADDR);
	p.add(agps::Type::INT, 'P', "port", "代理服务器端口号 [0-65535:80]", false,
		  agps::Value{.Int = 80}, CHECK_INT_BETWEEN(0, 65535));
	p.add(agps::Type::STR, 'u', "user", "用户名");
	p.add(agps::Type::STR, 'p', "passwd", "用户密码");
	p.add(agps::Type::STR, 'd', "dstfile", "目的文件名");
	p.add(agps::Type::STR, 's', "srcfile", "本地源文件目录");

	p.parse(argc, (const char **)argv);
	if (p.isExist("help")) {
		p.printUsage();
		return 0;
	}
	if (!p.success()) {
		log_printf("Error : Argument parse failed.\n%s\n", p.error());
		p.printUsage();
		return 0;
	}
	LogC::log_println("=== HTTP CLIENT ===");
	LogC::log_printf("ip      : %s\n", p.get("ip").Str);
	LogC::log_printf("port    : %d\n", p.get("port").Int);
	LogC::log_printf("user    : %s\n", p.get("user").Str);
	LogC::log_debug("passwd  : %s\n", p.get("passwd").Str);
	LogC::log_printf("dstfile : %s\n", p.get("dstfile").Str);
	LogC::log_printf("srcfile : %s\n", p.get("srcfile").Str);

	int con;
	// 连接
	{
		con = socket(AF_INET, SOCK_STREAM, 0);
		if (con < 0)
			log_fatal("Socket failed.\n");

		sockaddr_in server_addr;
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(p.get("port").Int);

		// 转换 ip 地址
		if (inet_pton(AF_INET, p.get("ip").Str, &server_addr.sin_addr) <= 0) {
			log_fatal("Bad IP.\n");
		}

		int ret =
			connect(con, (const sockaddr *)&server_addr, sizeof(server_addr));
		if (ret < 0)
			log_fatal("Connect failed.\n");

		ret = fcntl(con, F_GETFL, 0);
		if (ret < 0)
			log_fatal("Fcntl(F_GETFL) failed.\n");
		ret |= O_NONBLOCK;
		if (fcntl(con, F_SETFL, ret) < 0)
			log_fatal("Fcntl(F_SETFL) failed.\n");
	}
	log_println("connect success");

	Http http(con);

	Header header;
	header["Host"] = p.get("ip").Str;

	// {
	// 	int ret = http.method(METHOD::GET, ""s, "/lib/smain.php", header);

	// 	sleep(1);
	// 	ret = http.recv();
	// 	if (ret < 0) {
	// 		log_debug("%d %s\n", http.result_code(), http.result_msg().c_str());
	// 		log_debug("%s\n", http.result_data().c_str());
	// 		log_fatal("%s:%d %s\n", __FILE__, __LINE__, strerror(errno));
	// 	}
	// 	log_debug("%d %s\n", http.result_code(), http.result_msg().c_str());
	// }
	// log_printf("%d %s\n", http.result_code(), http.result_msg().c_str());
	// log_printf("%s\n",  http.result_data().c_str());
	// return 0;

	//发送请求
	{
		int ret = http.method(METHOD::GET, ""s, "/", header);
		if (ret < 0)
			log_fatal("%s:%d %s\n", __FILE__, __LINE__, strerror(errno));
		log_debug("Write -> %d\n", ret);
		sleep(1);
		ret = http.recv();
		if (ret < 0) {
			log_debug("%d %s\n", http.result_code(), http.result_msg().c_str());
			log_debug("%s\n", http.result_data().c_str());
			log_fatal("%s:%d %s\n", __FILE__, __LINE__, strerror(errno));
		}
		log_debug("%d %s\n", http.result_code(), http.result_msg().c_str());
	}
	log_println("get cookie...");
	log_printf("%d %s\n", http.result_code(), http.result_msg().c_str());
	log_printf("cookie : %s\n", http.getCookie().c_str());

	// 获得 cookie 与验证码 post 登录
	{
		//验证码
		// const char *verify_find =
		// 	"<input type=\"hidden\" name=\"auth_answer\" value=";
		// string data = http.result_data();
		// int pos = data.find(verify_find) + strlen(verify_find);
		// int pos2 = data.find(">", pos);

		// post
		string post_str = "username="s + p.get("user").Str +
						  "&password=" + p.get("passwd").Str +
						  "&input_ans=89&auth_ask=70+19=&auth_answer=89&"
						  "login=%B5%C7%C2%BC";

		log_debug("data = %s\n", post_str.c_str());
		//递交 POST
		int ret = http.method(METHOD::POST, post_str, "/", header);
		if (ret < 0)
			log_fatal("%s:%d %s\n", __FILE__, __LINE__, strerror(errno));
		sleep(1);
		ret = http.recv();
		if (ret < 0) {
			log_debug("%d %s\n", http.result_code(), http.result_msg().c_str());
			log_debug("%s\n", http.result_data().c_str());
			log_fatal("%s:%d %s\n", __FILE__, __LINE__, strerror(errno));
		}
	}
	log_println("login...");
	log_printf("%d %s\n", http.result_code(), http.result_msg().c_str());

	{
		http.clearCookie() ;
		http.addCookie(
			Header{{"PHPSESSID", "gattado47gpcsbl9m6421rrvpl"}});
		//递交 POST
		int ret = http.method(METHOD::GET, ""s, "/lib/smain.php", header);
		if (ret < 0)
			log_fatal("%s:%d %s\n", __FILE__, __LINE__, strerror(errno));
		sleep(1);
		ret = http.recv();
		if (ret < 0) {
			log_debug("%d %s\n", http.result_code(), http.result_msg().c_str());
			log_debug("%s\n", http.result_data().c_str());
			log_fatal("%s:%d %s\n", __FILE__, __LINE__, strerror(errno));
		}
	}
	log_println("login...");
	log_printf("%d %s\n", http.result_code(), http.result_msg().c_str());
	log_printf("%s\n", http.result_data().c_str());

	return 0;
}
