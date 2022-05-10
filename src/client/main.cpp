#include <agps/agps.h>
#include <agps/check.h>

#include <iostream>

int main(int argc, char **argv) {
	agps::Parser p;
	p.add(agps::Type::FLAG, 'h', "help", "��������б�");
	p.add(agps::Type::STR, 'H', "ip", "��������� IP ��ַ [162.168.1.236]",
		  false, agps::Value{.Str = "192.168.1.236"}, CHECK_STR_IPADDR);
	p.add(agps::Type::INT, 'P', "port", "����������˿ں� [0-65535:80]", false,
		  agps::Value{.Int = 80}, CHECK_INT_BETWEEN(0, 65535));
	p.add(agps::Type::STR, 'u', "user", "�û���");
	p.add(agps::Type::STR, 'p', "passwd", "�û�����");
	p.add(agps::Type::STR, 'd', "dstfile", "Ŀ���ļ���");
	p.add(agps::Type::STR, 's', "srcfile", "����Դ�ļ�Ŀ¼");
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

	std::cout << "This is client." << std::endl;
	std::cout << "ip      : " << p.get("ip").Str << std::endl;
	std::cout << "port    : " << p.get("port").Int << std::endl;
	std::cout << "user    : " << p.get("user").Str << std::endl;
	std::cout << "passwd  : " << p.get("passwd").Str << std::endl;
	std::cout << "dstfile : " << p.get("dstfile").Str << std::endl;
	std::cout << "srcfile : " << p.get("srcfile").Str << std::endl;
	return 0;
}
