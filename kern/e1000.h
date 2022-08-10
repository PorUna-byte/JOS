#ifndef JOS_KERN_E1000_H
#include <kern/pmap.h>
#define PCI_VENDOR_ID            0x8086
#define PCI_DEVICE_ID            0x100E
#define E1000_TCTL               0x0400  /* TX Control - RW */
#define E1000_TDBAL              0x3800  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH              0x3804  /* TX Descriptor Base Address High - RW */
#define E1000_TDLEN              0x3808  /* TX Descriptor Length - RW */
#define E1000_TDH                0x3810  /* TX Descriptor Head - RW */
#define E1000_TDT                0x3818  /* TX Descripotr Tail - RW */
#define E1000_TIPG               0x0410  /* TX Inter-packet gap -RW */
#define E1000_TCTL_EN            0x00000002    /* enable tx */
#define E1000_TCTL_BCE           0x00000004    /* busy check enable */
#define E1000_TCTL_PSP           0x00000008    /* pad short packets */
#define E1000_TCTL_CT            0x00000ff0    /* collision threshold */
#define E1000_TCTL_COLD          0x003ff000    /* collision distance */
#define E1000_TXD_CMD_RS         0x08     /* Report Status */
#define E1000_TXD_CMD_EOP        0x01     /* End of packet */
#define E1000_TXD_STAT_DD        0x01     /* Descriptor Done */
#define TX_MAX                   64
#define RX_MIN                   128

///////////////////////////////////////////
#define E1000_RCTL_EN             0x00000002    /* enable */
#define E1000_RCTL_SBP            0x00000004    /* store bad packet */
#define E1000_RCTL_UPE            0x00000008    /* unicast promiscuous enable */
#define E1000_RCTL_MPE            0x00000010    /* multicast promiscuous enab */
#define E1000_RCTL_LPE            0x00000020    /* long packet enable */
#define E1000_RCTL_LBM_NO         0x00000000    /* no loopback mode */
#define E1000_RCTL_BAM            0x00008000    /* broadcast enable */
#define E1000_RCTL_SZ_2048        0x00000000    /* rx buffer size 2048 */
#define E1000_RCTL_SECRC          0x04000000    /* Strip Ethernet CRC */
#define E1000_RXD_STAT_DD       0x01    /* Descriptor Done */
#define E1000_RXD_STAT_EOP      0x02    /* End of Packet */
#define E1000_RCTL     0x00100  /* RX Control - RW */
#define E1000_RDBAL    0x02800  /* RX Descriptor Base Address Low - RW */
#define E1000_RDBAH    0x02804  /* RX Descriptor Base Address High - RW */
#define E1000_RDLEN    0x02808  /* RX Descriptor Length - RW */
#define E1000_RDH      0x02810  /* RX Descriptor Head - RW */
#define E1000_RDT      0x02818  /* RX Descriptor Tail - RW */

#define E1000_MTA      0x05200  /* Multicast Table Array - RW Array */
#define E1000_RA       0x05400  /* Receive Address - RW Array */
#define E1000_RAH_AV  0x80000000        /* Receive descriptor valid */


struct tx_desc
{
  uint64_t addr;   //physical address of the transmit buffer in the host memory
  uint16_t length; 
  uint8_t cso; //The Checksum offset field indicates where to insert a TCP checksum if this mode is enabled. 
  uint8_t cmd;
  uint8_t status;
  uint8_t css; //The Checksum start field (TDESC.CSS) indicates where to begin computing the checksum. 
  uint16_t special;
};
struct rx_desc{
  uint64_t addr;       /* Address of the descriptor's data buffer */
  uint16_t length;     /* Length of data DMAed into data buffer */
  uint16_t csum;       /* Packet checksum */
  uint8_t status;      /* Descriptor status */
  uint8_t errors;      /* Descriptor Errors */
  uint16_t special;
};
struct packet{
  char buffer[2048];
};
/*
  The following arrays are global arrays that reside in virtual address space's kernel data area(start from KERNBASE).
  The kernel data area and kernel text area all starts from KERNBASE and mapped to physical address 0(where kernel reside in physical memeory)
  by all page directories that appear in JOS(i.e. entry_pgdir,kernel_pgdir,all env_pgdir...)
  Look inside what PADDR do,you will find it take into a virtual address and minus KERNBASE to form a physical address,
  which exactly do the mapping from kernel area in virtual address space to kernel area in physical memory
  As we know all pointer like T* are virtual address,so tx_list and tx_buf are virtual address in kernel area
  Hence the physical address of tx_list is PADDR(tx_list)
  the physical address of tx_buf is PADDR(tx_buf) etc...
*/
struct tx_desc tx_ring[TX_MAX];
struct rx_desc rx_ring[RX_MIN];
struct packet tx_bufs[TX_MAX];
struct packet rx_bufs[RX_MIN];

extern void pci_func_enable(struct pci_func *pcif);
void e1000_transmit_init();
void e1000_receive_init();
volatile uint32_t *pci_e1000;

int e1000_init(struct pci_func *pcif){
  pci_func_enable(pcif);
  pci_e1000=mmio_map_region(pcif->reg_base[0],pcif->reg_size[0]);
  e1000_transmit_init();
  e1000_receive_init();
  return 1;
}

void e1000_transmit_init(){
  memset(tx_ring, 0, sizeof(struct tx_desc)*TX_MAX);
  memset(tx_bufs, 0, sizeof(struct packet)*TX_MAX);
  for(int i=0; i<TX_MAX; i++){
    tx_ring[i].addr = PADDR(tx_bufs[i].buffer); 
    tx_ring[i].cmd = E1000_TXD_CMD_EOP| E1000_TXD_CMD_RS;
    tx_ring[i].status = E1000_TXD_STAT_DD;
  }
  pci_e1000[E1000_TDBAL>>2] = PADDR(tx_ring);
  pci_e1000[E1000_TDBAH>>2] = 0;
  pci_e1000[E1000_TDLEN>>2] = TX_MAX*sizeof(struct tx_desc);
  pci_e1000[E1000_TDH>>2] = 0;
  pci_e1000[E1000_TDT>>2] = 0;
  pci_e1000[E1000_TCTL>>2] |= (E1000_TCTL_EN | E1000_TCTL_PSP |
                                (E1000_TCTL_CT & (0x10<<4)) |
                                (E1000_TCTL_COLD & (0x40<<12)));
  pci_e1000[E1000_TIPG>>2] |= (10) | (4<<10) | (6<<20);
}

void
e1000_receive_init()
{    
  memset(rx_ring, 0, sizeof(struct rx_desc)*RX_MIN);
  memset(rx_bufs, 0, sizeof(struct packet)*RX_MIN);
  for(int i=0; i<RX_MIN; i++)
    rx_ring[i].addr = PADDR(rx_bufs[i].buffer);

  pci_e1000[E1000_MTA>>2] = 0;
  pci_e1000[E1000_RDBAL>>2] = PADDR(rx_ring);
  pci_e1000[E1000_RDBAH>>2] = 0;
  pci_e1000[E1000_RDLEN>>2] = RX_MIN*sizeof(struct rx_desc);
  pci_e1000[E1000_RDH>>2] = 0;
  pci_e1000[E1000_RDT>>2] = RX_MIN-1;
  pci_e1000[E1000_RCTL>>2] = (E1000_RCTL_EN | E1000_RCTL_BAM |
                              E1000_RCTL_LBM_NO | E1000_RCTL_SZ_2048 |
                                E1000_RCTL_SECRC);
  pci_e1000[E1000_RA>>2] = 0x52 | (0x54<<8) | (0x00<<16) | (0x12<<24);
  pci_e1000[(E1000_RA>>2) + 1] = (0x34) | (0x56<<8) | E1000_RAH_AV;
}

int e1000_transmit(char* data,uint16_t length){
  if(length>2048)
    return -1;
  int tail=pci_e1000[E1000_TDT>>2];
  if(tx_ring[tail].status & E1000_TXD_STAT_DD){
    //We have available buffer to transmit data
    //we can't use physical address in program directly
    memset(KADDR(tx_ring[tail].addr),0,2048);
    memcpy(KADDR(tx_ring[tail].addr),data,length);
    tx_ring[tail].length=length;
    tx_ring[tail].status &= ~E1000_TXD_STAT_DD; 
    pci_e1000[E1000_TDT>>2]=(tail+1)%TX_MAX;
    return 0;
  }
  //full transmit queue
  return -1;
}

int e1000_receive(char* buf){
  int tail=(pci_e1000[E1000_RDT>>2]+1)%RX_MIN;
  if(rx_ring[tail].status & E1000_RXD_STAT_DD){ 
    //We have available data to receive
    memcpy(buf,KADDR(rx_ring[tail].addr),rx_ring[tail].length);
    rx_ring[tail].status &= ~E1000_RXD_STAT_DD;
    pci_e1000[E1000_RDT>>2]=tail;
    return rx_ring[tail].length;
  }
  //empty receive queue
  return -1;
}

#define JOS_KERN_E1000_H
#endif  // SOL >= 6