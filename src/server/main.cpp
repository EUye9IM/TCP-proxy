#include <agps/agps.h>
#include <agps/check.h>

#include <iostream>

int main(int argc, char **argv) {
	agps::Parser p;
	p.add(agps::Type::FLAG, 'h', "help", "输出参数列表");
	p.add(agps::Type::FLAG, 'd', "daemon", "成为守护进程");
	p.add(agps::Type::INT, 'p', "port", "代理端口号 [0-65535:0]", false,
		  agps::Value{.Int = 0}, CHECK_INT_BETWEEN(0, 65535));
	p.add(agps::Type::STR, 'H', "proxy_ip",
		  "被代理服务器 IP 地址 [162.168.1.232]", false,
		  agps::Value{.Str = "192.168.1.232"}, CHECK_STR_IPADDR);
	p.add(agps::Type::INT, 'P', "proxy_port",
		  "被代理服务器 TCP 端口号 [0-65535:80]", false, agps::Value{.Int = 80},
		  CHECK_INT_BETWEEN(0, 65535));
	p.add(agps::Type::STR, 'l', "logname",
		  "日志文件名（全路径） [/var/log/tcp-proxy-server.log]", false,
		  agps::Value{.Str = "/var/log/tcp-proxy-server.log"});
	p.parse(argc, (const char **)argv);
	if (!p.success() || p.isExist("help")) {
		p.printUsage(argv[0]);
		return 0;
	}

	std::cout << "This is server." << std::endl;
	std::cout << "daemon     : " << (p.get("daemon").Exist ? "yes" : "no")
			  << std::endl;
	std::cout << "port       : " << p.get("port").Int << std::endl;
	std::cout << "proxy_ip   : " << p.get("proxy_ip").Str << std::endl;
	std::cout << "proxy_port : " << p.get("proxy_port").Int << std::endl;
	std::cout << "logname    : " << p.get("logname").Str << std::endl;

	return 0;
}
