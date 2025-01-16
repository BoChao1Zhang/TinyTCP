
#include <stdio.h>
#include "net.h"
#include "dbg.h"
#include "sys_plat.h"

#include "netif_pcap.h"
#include "nlist.h"

net_err_t netdev_init() {
	netif_pcap_open();

	return NET_ERR_OK;
}

typedef struct _tnode_t {
	int id;
	nlist_node_t node;
} tnode_t;

void nlist_test() {
	#define NODE_CNT 4

	tnode_t node[NODE_CNT];
	nlist_t list;
	nlist_init(&list);
	for (int i =0;i< NODE_CNT;i++) {
		node[i].id = i;
		nlist_insert_first(&list,&node[i].node);
	}


	plat_printf("insert first\n");
	nlist_node_t* p;

	nlist_foreach(p,&list) {
		tnode_t *tnode = nlist_entry(p,tnode_t,node);
		plat_printf("id:%d\n", tnode->id);
	}

	plat_printf("remove first\n");

	for (int i = 0;i<4;i++) {
		p = nlist_remove_first(&list);
		plat_printf("id:%d\n",nlist_entry(p,tnode_t,node)->id);
	}

	plat_printf("insert last\n");

	for (int i = 0;i<4;i++) {
		node[i].id = i;
		nlist_insert_last(&list,&node[i].node);
	}

	nlist_foreach(p,&list) {
		tnode_t *tnode = nlist_entry(p,tnode_t,node);
		plat_printf("id:%d\n", tnode->id);
	}


	plat_printf("remove last\n");

	for (int i = 0;i<4;i++) {
		nlist_node_t* p = nlist_remove_last(&list);
		plat_printf("id=%d\n",nlist_entry(p,tnode_t,node)->id);
	}

	plat_printf("insert pre\n");

	for (int i = 0;i<4;i++) {
		//0
		//0 1
		//0 2 1
		//0 3 2 1
		node[i].id = i;
		nlist_insert(&list,list.first,&node[i].node);
	}

	nlist_foreach(p,&list) {
		tnode_t *tnode = nlist_entry(p,tnode_t,node);
		plat_printf("id:%d\n", tnode->id);
	}

}

void basic_test(void) {
	nlist_test();
}



#define DBG_TEST DBG_LEVEL_INFO

int main (void) {

	net_init();

	basic_test();
	net_start();


	netdev_init();
	// 以下是测试代码，可以删掉
	// 打开物理网卡，设置好硬件地址
	while(1){
		sys_sleep(10);
	}
	return 0;
}