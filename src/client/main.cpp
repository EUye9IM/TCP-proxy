#include <agps/agps.h>
#include <agps/check.h>
#include <logc/logc.h>

#include "http/httplib.h"
#include <fstream>

using namespace LogC;
using namespace std;
int main(int argc, char **argv) {
	log_open(stdout);
	// debug mode
	// log_set(LOG_FLAG_DEBUG);

	// LOG_FLAG_DEBUG
	agps::Parser p;
	p.add(agps::Type::FLAG, 'h', "help", "��������б�");
	p.add(agps::Type::STR, 'H', "ip", "��������� IP ��ַ [10.10.108.117]",
		  false, agps::Value{.Str = "10.10.108.117"}, CHECK_STR_IPADDR);
	p.add(agps::Type::INT, 'P', "port", "����������˿ں� [0-65535:80]", false,
		  agps::Value{.Int = 80}, CHECK_INT_BETWEEN(0, 65535));
	p.add(agps::Type::STR, 'u', "user", "�û���");
	p.add(agps::Type::STR, 'p', "passwd", "�û�����");
	p.add(agps::Type::STR, 'd', "dstfile", "Ŀ���ļ���");
	p.add(agps::Type::STR, 's', "srcfile", "����Դ�ļ�Ŀ¼");
	p.add(agps::Type::STR, 'a', "action", "�½�����Ĭ�ϵ�0��", false,
		  agps::Value{.Str = "��0��"});

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
	log_printf("action  : %s\n", p.get("action").Str);

	ifstream srcfile(p.get("srcfile").Str, ios::binary | ios::ate);
	if (!srcfile.is_open()) {
		log_fatal("File %s open failed.\n", p.get("srcfile").Str);
	}
	int size = srcfile.tellg();
	if (size < 0) {
		log_fatal("Get file %s  size failed.\n", p.get("srcfile").Str);
	}
	string filecontent(size, '\0');
	srcfile.seekg(ios::beg);
	srcfile.read(&filecontent[0], size);
	srcfile.close();
	log_printf("�ļ���ȡ�ɹ����� %d �ֽ�\n", filecontent.size());

	httplib::Client cli("http://"s + p.get("ip").Str + ":" +
						to_string(p.get("port").Int));
	string cookie;
	// ��ȡ��¼�� cookie
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

		// ��֤��
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

		// post ����
		httplib::Params params;
		params.emplace("username", p.get("user").Str);
		params.emplace("password", p.get("passwd").Str);
		params.emplace("input_ans", auth_answer);
		params.emplace("auth_ask", auth_ask + "%3D");
		params.emplace("auth_answer", auth_answer);
		params.emplace("login", "%B5%C7%C2%BC");
		// post
		res = cli.Post(route, httplib::Headers{{"Cookie", cookie}}, params);
		if (res->status != 200) {
			log_fatal("%s %d %s\n", route, res->status, res->reason.c_str());
		}
		if (res->has_header("Set-Cookie")) {
			cookie = res->get_header_value("Set-Cookie");
		}
		// ��֤����
		pos1 = res->body.find("alert");
		log_debug("find(\"alert\") -> %d\n", pos1);
		if (pos1 >= 0) {
			log_fatal("�û������������\n");
		}
	}
	log_println("��¼�ɹ���");

	// �ļ��ݽ�
	{
		string route = "/lib/smain.php?action="s + p.get("action").Str;
		auto res = cli.Get(route.c_str(), httplib::Headers{{"Cookie", cookie}});
		if (res->status != 200) {
			log_fatal("%s %d %s\n", route.c_str(), res->status,
					  res->reason.c_str());
		}
		if (res->has_header("Set-Cookie")) {
			cookie = res->get_header_value("Set-Cookie");
		}

		// Ѱ�ҵݽ��ļ�����

		int pos = res->body.find("<td>"s + p.get("dstfile").Str + "</td>");
		if (pos < 0) {
			log_fatal("�ļ� %s ���� %s ��\n", p.get("dstfile").Str,
					  route.c_str());
		}
		pos = res->body.find("<input type=\"file\"", pos);
		const char *find_str = "name=\"";
		pos = res->body.find(find_str, pos);
		pos += strlen(find_str);
		int pos2 = res->body.find("\"", pos);
		if (pos2 < 0) {
			log_fatal("�ļ� %s ���� %s ��\n", p.get("dstfile").Str,
					  route.c_str());
		}
		string post_name = res->body.substr(pos, pos2 - pos);
		log_debug("post �ֶ��� %s\n", post_name.c_str());

		// post
		httplib::MultipartFormDataItems items = {
			{post_name, filecontent, p.get("dstfile").Str,
			 "application/octet-stream"},
			{"submit", "�ύ1��", "", ""},
		};
		// httplib::MultipartFormDataItems items = {
		// 	{post_name, filecontent, p.get("srcfile").Str,
		// 	 "application/octet-stream"},
		// 	{"submit", "�ύ1��", "", ""},
		// };
		res = cli.Post(route.c_str(), httplib::Headers{{"Cookie", cookie}},
					   items);
		if (res->status != 200) {
			log_fatal("%s %d %s\n", route.c_str(), res->status,
					  res->reason.c_str());
		}
		if (res->has_header("Set-Cookie")) {
			cookie = res->get_header_value("Set-Cookie");
		}
		pos = res->body.find("<input type=\"file\"", pos);

		find_str = "alert(";
		pos = res->body.find(find_str);
		if (pos < 0) {
			log_fatal("�ļ��ύʧ��\n");
		}
		pos = res->body.find("\\n", pos);
		if (pos < 0) {
			log_fatal("�ļ��ύʧ��\n");
		}
		pos += 2;
		pos2 = res->body.find("\\n", pos);
		if (pos2 < 0) {
			log_fatal("�ļ��ύʧ��\n");
		}
		log_println(res->body.substr(pos, pos2 - pos).c_str());
	}

	return 0;
}
