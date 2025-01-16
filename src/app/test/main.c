
#include <stdio.h>
#include "net.h"
#include "sys_plat.h"



int main (void) {
	net_init();
	net_start();
	// 以下是测试代码，可以删掉
	// 打开物理网卡，设置好硬件地址
	while(1){
		sys_sleep(10);
	}
	return 0;
}