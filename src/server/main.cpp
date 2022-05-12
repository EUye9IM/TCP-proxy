#include "connection.h"
#include "top.h"
#include <agps/agps.h>
#include <agps/check.h>
#include <exception>

int main(int argc, char **argv) {
	// Top top;
	// // ��ʼ��
	// top.run();

	agps::Parser p;
	p.add(agps::Type::FLAG, 'h', "help", "��������б�");
	p.add(agps::Type::FLAG, 'd', "daemon", "��Ϊ�ػ�����");
	p.add(agps::Type::INT, 'p', "port", "����˿ں� [0-65535:0]", false,
		  agps::Value{.Int = 0}, CHECK_INT_BETWEEN(0, 65535));
	p.add(agps::Type::STR, 'H', "proxy_ip",
		  "����������� IP ��ַ [192.168.1.236]", false,
		  agps::Value{.Str = "192.168.1.236"}, CHECK_STR_IPADDR);
	p.add(agps::Type::INT, 'P', "proxy_port",
		  "����������� TCP �˿ں� [0-65535:80]", false, agps::Value{.Int = 80},
		  CHECK_INT_BETWEEN(0, 65535));
	p.add(agps::Type::STR, 'l', "logname",
		  "��־�ļ�����ȫ·���� [/var/log/tcp-proxy-server.log]", false,
		  agps::Value{.Str = "/var/log/tcp-proxy-server.log"});
	p.parse(argc, (const char **)argv);
	if (p.isExist("help")) {
		p.printUsage();
		return 0;
	}
	if (!p.success() || p.isExist("help")) {
		std::cout << "Error:" << std::endl;
		std::cout << p.error() << std::endl;
		p.printUsage();
		return 0;
	}

	std::cout << "This is server." << std::endl;
	std::cout << "daemon     : " << (p.get("daemon").Exist ? "yes" : "no")
			  << std::endl;
	std::cout << "port       : " << p.get("port").Int << std::endl;
	std::cout << "proxy_ip   : " << p.get("proxy_ip").Str << std::endl;
	std::cout << "proxy_port : " << p.get("proxy_port").Int << std::endl;
	std::cout << "logname    : " << p.get("logname").Str << std::endl;

	/* ��ȡ���ӵĵ�ַ�˿���Ϣ */
	int port = p.get("port").Int;
	std::string proxy_ip = p.get("proxy_ip").Str;
	int proxy_port = p.get("proxy_port").Int;
	auto tcp_proxy = new New::Tcp_Proxy(proxy_ip, proxy_port, port);
	try {
		tcp_proxy->Run();

	}
	catch (std::exception& e) {
		std::cerr << "Exception caught: " << e.what() << std::endl;
	}
	
	delete tcp_proxy;

	return 0;
}