
#include <stdio.h>
#include "net.h"
#include "dbg.h"
#include "sys_plat.h"
#include "netif_pcap.h"
#include "nlist.h"
#include "mblock.h"
#include "pktbuf.h"

net_err_t netdev_init() {
	netif_pcap_open();

	return NET_ERR_OK;
}

typedef struct _tnode_t {
	int id;
	nlist_node_t node;
} tnode_t;

void nlist_test(void) {
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

void mblock_test(void) {
	mblock_t blist;
	static uint8_t buffer[10][100];

	mblock_init(&blist,buffer,100,10,NLOKCER_THREAD);

	void* temp[10];
	for (int i = 0;i<10;i++) {
		temp[i] = mblock_alloc(&blist,0);
		plat_printf("block: %p,free_count: %d\n",temp[i],mblock_free_cnt(&blist));
	}

	for (int i = 0;i<10;i++) {
		mblock_free(&blist,temp[i]);
		plat_printf("free count: %d\n",mblock_free_cnt(&blist));
	}


}

void pktbuf_test(void) {
	pktbuf_t *pktbuf = pktbuf_alloc(2000);
	pktbuf_free(pktbuf);

	pktbuf_t *buf = pktbuf_alloc(2000);
	for (int i = 0;i<16;i++) {
		pktbuf_add_header(buf,33,1);
	}

	for (int i = 0;i<16;i++) {
		pktbuf_remove_header(buf,33);
	}

	for (int i = 0;i<16;i++) {
		pktbuf_add_header(buf,33,0);
	}

	for (int i = 0;i<16;i++) {
		pktbuf_remove_header(buf,33);
	}
	pktbuf_free(buf);

	buf = pktbuf_alloc(8);
	pktbuf_resize(buf,32);
	pktbuf_resize(buf,288);
	pktbuf_resize(buf,4922);



}

void basic_test(void) {
	// nlist_test();
	// mblock_test();
	pktbuf_test();
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