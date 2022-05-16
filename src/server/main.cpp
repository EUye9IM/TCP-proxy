#include "connection.h"
#include "top.h"
#include <memory>
#include <mydaemon.h>
#include <agps/agps.h>
#include <agps/check.h>
#include <exception>
#include <logc/logc.h>
#include <stdexcept>

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
		  "��־�ļ�����ȫ·���� [./tcp-proxy-server.log]", false,
		  agps::Value{.Str = "./tcp-proxy-server.log"});
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
	/* �Ƿ������ػ����� */
	if (p.get("daemon").Exist) {
		my_daemon(1);
	}

	LogC::log_open(p.get("logname").Str);
	
	LogC::log_println("Server load");
	LogC::log_printf("daemon     : %s\n",
					 (p.get("daemon").Exist ? "yes" : "no"));
	LogC::log_printf("port       : %d\n", p.get("port").Int);
	LogC::log_printf("proxy_ip   : %s\n", p.get("proxy_ip").Str);
	LogC::log_printf("proxy_port : %d\n", p.get("proxy_port").Int);

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

	for (int i = 0; i < 10; i++) {
		std::cout << "�������� " << i + 1 << " ������" << std::endl;
		auto tcp_proxy = 
			std::make_unique<New::Tcp_Proxy>(proxy_ip, proxy_port, port);
		try {
			tcp_proxy->Run();
		}
		catch (const std::runtime_error& e) {
			std::cerr << "Run time error: " << e.what() << std::endl;
			std::cout << "���������������..." << std::endl;
		}
		sleep(10);
	}

	LogC::log_close();
	
	return 0;
}