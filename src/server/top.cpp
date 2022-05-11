#include "top.h"

void Top::run(){
	while(1){
		// epoll

		// 拿 fd 检查。
		//    Connection::run()
		// or delete Connection
		// or accept(); new Connection

		// 如果是 connection run，fd_set中移除相关fd，再调用Connection::fdSet()拿取新的
		// 其他情况也有 fd_set 的变动

		// 以 fd_set_read 和 fd_set_write 更新epoll用的队列
	}
}