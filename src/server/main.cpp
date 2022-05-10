#include <agps/agps.h>
#include <agps/check.h>

#include <iostream>
#include <ostream>
#include "mysocket.hpp"

/**
 * @brief 	���ӱ�����ķ�����
 * @param	proxy_ip �����������ַ
 * @param	proxy_port ����������˿�
 * @param	myport ���ӱ�����������Ķ˿ڣ�-1��ʾ����˿�
 * @return 	������Socket_Connect��ָ�룬֮���Է���ָ�룬��Ϊ��ֹ������������close(fd)
 **/
Anakin::Socket_Connect* connect_proxied_server(std::string proxy_ip, int proxy_port, int myport=-1);

int main(int argc, char **argv) {
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

	/* ��ȡ���ӵĵ�ַ�˿���Ϣ */
	int port = p.get("port").Int;
	std::string proxy_ip = p.get("proxy_ip").Str;
	int proxy_port = p.get("proxy_port").Int;

	std::cout << "����������� " << proxy_ip << ":" << proxy_port << std::endl;
	auto client_socket = connect_proxied_server(proxy_ip, proxy_port);
	std::cout << "connect �ɹ�" << std::endl;

	/* ���ô������� */
	std::cout << "������������..." << std::endl;
	Anakin::Socket_Accept server_socket(AF_INET, SOCK_STREAM, 0);

	// ���õ�ַ����
	int opt = 1;
	server_socket.setopt(SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
	// ��������Ϊ��������ʽ
	server_socket.SetSocketBlockingEnable(false);

	struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

	address.sin_addr.s_addr = INADDR_ANY;

	server_socket.Bind((struct sockaddr*)&address, sizeof(address));
	server_socket.Listen(3);
	std::cout << "listen..." << std::endl;


	delete client_socket;
	return 0;
}


Anakin::Socket_Connect* connect_proxied_server(std::string proxy_ip, int proxy_port, int myport)
{
	/* ������Ϊ������������Ӳ�����վ */
	auto client_socket = new Anakin::Socket_Connect(AF_INET, SOCK_STREAM, 0);

	// ����Ϊ��������ʽ
	client_socket->SetSocketBlockingEnable(false);

	struct sockaddr_in client_addr;
	
	// ��� myport �ڲ���������
	if (myport != -1) {
		// bind client�˿�
		client_addr.sin_family = AF_INET;
		client_addr.sin_addr.s_addr = INADDR_ANY;
		client_addr.sin_port = htons(myport);
		client_socket->Bind((struct sockaddr *)&client_addr, sizeof(client_addr));
	}

	// ��������ʽ����
	int ret = client_socket->Connect(proxy_ip, proxy_port, false);
	int sockfd = client_socket->getfd();
	
    if (ret == -1 && errno == EINPROGRESS) {
        // ���ڽ�������
        fd_set set;
        FD_ZERO(&set);
        FD_SET(sockfd , &set);
        int retval = select(sockfd + 1, NULL, &set, NULL, NULL);
        if (retval < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }
        if (FD_ISSET(sockfd, &set)) {
            // ˵���Ѿ���д��ͨ��getpeername�ж�
			struct sockaddr peeraddr {};
			socklen_t peeraddrlen = sizeof(peeraddr);
            if (getpeername(sockfd, &peeraddr, &peeraddrlen) == -1) {
                perror("getpeername");
                exit(EXIT_FAILURE);
            }
        }
    }

	return client_socket;
}