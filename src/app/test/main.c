
#include <stdio.h>
#include "net.h"
#include "dbg.h"
#include "sys_plat.h"
#include "netif_pcap.h"
#include "nlist.h"
#include "mblock.h"
#include "pktbuf.h"
#include "netif.h"

pcap_data_t netdev0_data = {
	.ip = netdev0_phy_ip,
	.hwaddr = netdev0_hwaddr
};

net_err_t netdev_init() {
	// dbg_info(DBG_NETIF, "init netif\n");

	netif_t * netif = netif_open("netif 0",&netdev_ops,&netdev0_data);
	if (!netif) {
		dbg_error(DBG_NETIF,"open netif failed\n");
		return NET_ERR_NONE;
	}
	ipaddr_t ip,mask,gw;
	ipaddr_from_str(&ip,netdev0_ip);
	ipaddr_from_str(&mask,netdev0_mask);
	ipaddr_from_str(&gw,netdev0_gw);
	net_err_t err = netif_set_addr(netif,&ip,&mask,&gw);

	if (err < 0) {
		dbg_error(DBG_NETIF,"set netif failed\n");
	}

	netif_set_active(netif);


	dbg_info(DBG_NETIF,"init netif done\n");
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
	pktbuf_resize(buf,1921);
	pktbuf_resize(buf,288);
	pktbuf_resize(buf,0);
	pktbuf_free(buf);

	buf = pktbuf_alloc(689);
	pktbuf_t *buf2 = pktbuf_alloc(1000);
	pktbuf_t *buf3 = pktbuf_alloc(30);
	pktbuf_join(buf,buf2);
	pktbuf_join(buf,buf3);
	pktbuf_free(buf);

	//32-4-16-54-38
	buf = pktbuf_alloc(32);
	pktbuf_join(buf,pktbuf_alloc(4));
	pktbuf_join(buf,pktbuf_alloc(16));
	pktbuf_join(buf,pktbuf_alloc(54));
	pktbuf_join(buf,pktbuf_alloc(38));

	pktbuf_set_cont(buf,44);
	pktbuf_set_cont(buf,60);
	pktbuf_set_cont(buf,44);
	pktbuf_set_cont(buf,128);
	pktbuf_set_cont(buf,135);
	pktbuf_free(buf);

	buf = pktbuf_alloc(32);
	pktbuf_join(buf,pktbuf_alloc(4));
	pktbuf_join(buf,pktbuf_alloc(16));
	pktbuf_join(buf,pktbuf_alloc(54));
	pktbuf_join(buf,pktbuf_alloc(38));
	pktbuf_join(buf,pktbuf_alloc(512));


	pktbuf_rest_acc(buf);
	static uint16_t temp[1024];
	for (int i = 0;i<1024;i++) {
		temp[i] = i;
	}
	pktbuf_write(buf,(uint8_t *)temp,pktbuf_size(buf));

	static uint16_t read_temp[1024];
	pktbuf_rest_acc(buf);
	plat_memset(read_temp,0,sizeof(read_temp));
	if (pktbuf_read(buf,(uint8_t *)read_temp,pktbuf_size(buf))!=0) {
		plat_printf("read error\n");
	}

	plat_memset(read_temp,0,sizeof(read_temp));
	pktbuf_seek(buf,18 *2);
	pktbuf_read(buf,(uint8_t *)read_temp,56);
	if (plat_memcpy(temp+18,read_temp,56)!=0) {
		plat_printf("read error\n");
	}

	plat_memset(read_temp,0,sizeof(read_temp));
	pktbuf_seek(buf,85 * 2);
	pktbuf_read(buf,(uint8_t *)read_temp,256);
	plat_memcpy(temp+85,read_temp,256);
	for (int i = 0;i<85;i++) {
		if (temp[85 + i] != read_temp[i]) {
			plat_printf("read error\n");
		}
	}

	pktbuf_t *dest = pktbuf_alloc(1024);
	pktbuf_seek(dest,600);
	pktbuf_seek(buf, 200);
	pktbuf_copy(dest,buf,122);

	plat_memset(read_temp,0,sizeof(read_temp));
	pktbuf_seek(dest, 600);
	pktbuf_read(dest,(uint8_t *)read_temp,122);
	if (plat_memcmp(temp+100,read_temp,122)!=0) {
		plat_printf("copy error\n");
	}

	pktbuf_seek(dest,0);
	pktbuf_fill(dest,53,pktbuf_size(dest));
	pktbuf_seek(dest,0);
	pktbuf_read(dest,(uint8_t *)read_temp,pktbuf_size(dest));
	for (int i = 0; i<512;i++) {
		if (read_temp[i]!=0x3535) {
			plat_printf("fill error\n");
			break;
		}
	}

	pktbuf_free(dest);
	pktbuf_free(buf);
}


void netif_test(void) {
	// netif_t * netif = netif_open("loop");
	plat_printf("你好");
}

void basic_test(void) {
	nlist_test();
	mblock_test();
	pktbuf_test();
	netif_test();

}



#define DBG_TEST DBG_LEVEL_INFO

int main (void) {

	net_init();
	netdev_init();

	// basic_test();
	net_start();



	// 以下是测试代码，可以删掉
	// 打开物理网卡，设置好硬件地址
	while(1){
		sys_sleep(1);
	}
	return 0;
}