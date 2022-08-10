#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";
	envid_t fromenv;
	int perm;
	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	while(1){
		if(ipc_recv(&fromenv,&nsipcbuf,&perm)==NSREQ_OUTPUT){
			while(sys_packet_send(nsipcbuf.pkt.jp_data,nsipcbuf.pkt.jp_len)<0)
				sys_yield(); //CPU friendly
		}
	}
}
