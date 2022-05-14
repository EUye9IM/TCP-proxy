#include <agps/agps.h>
#include <agps/check.h>
#include <logc/logc.h>

#include "http/httplib.h"

using namespace LogC;
using namespace std;
int main(int argc, char **argv) {
	log_open(stdout);
	log_set(LOG_FLAG_DEBUG, LOG_FLAG_DATE);
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
	log_println("=== HTTP CLIENT ===");
	log_printf("ip      : %s\n", p.get("ip").Str);
	log_printf("port    : %d\n", p.get("port").Int);
	log_printf("user    : %s\n", p.get("user").Str);
	log_debug("passwd  : %s\n", p.get("passwd").Str);
	log_printf("dstfile : %s\n", p.get("dstfile").Str);
	log_printf("srcfile : %s\n", p.get("srcfile").Str);

	httplib::Client cli("http://"s + p.get("ip").Str + ":" +
						to_string(p.get("port").Int));
	string cookie;
	// 获取登录的 cookie
	{
		const char route[] = "/";
		auto res = cli.Get(route);
		// cookie
		if (res->status != 200) {
			log_fatal("%s %d %s\n", route, res->status, res->reason.c_str());
		}
		if (res->has_header("Set-Cookie")) {
			cookie = res->get_header_value("Set-Cookie");
		}
		log_debug("get cookie: %s\n", cookie.c_str());

		// 验证码
		string verify_find =
			"<input type=\"hidden\" name=\"auth_answer\" value=";
		int pos1 = res->body.find(verify_find) + verify_find.size();
		int pos2 = res->body.find(" >", pos1);
		string auth_answer = res->body.substr(pos1, pos2 - pos1);
		log_debug("auth_answer: %s\n", auth_answer.c_str());
		verify_find = "<input type=\"hidden\" name=\"auth_ask\" value=";
		pos1 = res->body.find(verify_find) + verify_find.size();
		pos2 = res->body.find("= >", pos1);
		string auth_ask = res->body.substr(pos1, pos2 - pos1);
		log_debug("auth_ask: %s\n", auth_ask.c_str());

		// post 数据
		string data = "username="s + p.get("user").Str +
					  "&password=" + p.get("passwd").Str +
					  "&input_ans=" + auth_answer + "&auth_ask=" + auth_ask +
					  "%3D&auth_answer=" + auth_answer + "&login=%B5%C7%C2%BC";
		log_debug("post data: %s\n", data.c_str());

		//post
		res = cli.Post(route, httplib::Headers{{"Cookie", cookie}}, data);
	}

	// 获得 cookie 与验证码 post 登录
	// {
	// 	//验证码

	// 	// post
	// 	string post_str = "username="s + p.get("user").Str +
	// 					  "&password=" + p.get("passwd").Str +
	// 					  "&input_ans=89&auth_ask=70+19=&auth_answer=89&"
	// 					  "login=%B5%C7%C2%BC";

	// 	log_debug("data = %s\n", post_str.c_str());
	// 	//递交 POST
	// 	int ret = http.method(METHOD::POST, post_str, "/", header);
	// 	if (ret < 0)
	// 		log_fatal("%s:%d %s\n", __FILE__, __LINE__, strerror(errno));
	// 	sleep(1);
	// 	ret = http.recv();
	// 	if (ret < 0) {
	// 		log_debug("%d %s\n", http.result_code(),
	// http.result_msg().c_str()); 		log_debug("%s\n",
	// http.result_data().c_str()); 		log_fatal("%s:%d %s\n",
	// __FILE__,
	// __LINE__, strerror(errno));
	// 	}
	// }
	// log_println("login...");
	// log_printf("%d %s\n", http.result_code(), http.result_msg().c_str());

	// {
	// 	http.clearCookie() ;
	// 	http.addCookie(
	// 		Header{{"PHPSESSID", "gattado47gpcsbl9m6421rrvpl"}});
	// 	//递交 POST
	// 	int ret = http.method(METHOD::GET, ""s, "/lib/smain.php", header);
	// 	if (ret < 0)
	// 		log_fatal("%s:%d %s\n", __FILE__, __LINE__, strerror(errno));
	// 	sleep(1);
	// 	ret = http.recv();
	// 	if (ret < 0) {
	// 		log_debug("%d %s\n", http.result_code(),
	// http.result_msg().c_str()); 		log_debug("%s\n",
	// http.result_data().c_str()); 		log_fatal("%s:%d %s\n",
	// __FILE__,
	// __LINE__, strerror(errno));
	// 	}
	// }
	// log_println("login...");
	// log_printf("%d %s\n", http.result_code(), http.result_msg().c_str());
	// log_printf("%s\n", http.result_data().c_str());

	return 0;
}
