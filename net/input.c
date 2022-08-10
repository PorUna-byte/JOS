#include "ns.h"

extern union Nsipc nsipcbuf;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";
	int length;
	char buf[2048];
	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	while(1){
		while((length=sys_packet_recv(buf))<0)
			sys_yield();
		//we have read data from device driver, now send it to network server via IPC
		nsipcbuf.pkt.jp_len=length;
		memcpy(nsipcbuf.pkt.jp_data,buf,length);
		ipc_send(ns_envid,NSREQ_INPUT,&nsipcbuf,PTE_P|PTE_U);
		//wait for a while
		for(int i=0; i<50000; i++)
			if(i%1000==0)
				sys_yield();
	}
}
