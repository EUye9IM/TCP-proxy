#include "top.h"

void Top::run(){
	while(1){
		// epoll

		// �� fd ��顣
		//    Connection::run()
		// or delete Connection
		// or accept(); new Connection

		// ����� connection run��fd_set���Ƴ����fd���ٵ���Connection::fdSet()��ȡ�µ�
		// �������Ҳ�� fd_set �ı䶯

		// �� fd_set_read �� fd_set_write ����epoll�õĶ���
	}
}
