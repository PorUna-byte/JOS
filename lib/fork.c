// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800
extern void _pgfault_upcall(void);
//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	uint32_t pn=PGNUM(addr);
	if(!(err&FEC_WR)||!(uvpt[pn]&PTE_COW))
		panic("Not a write or the page is not a copy-on-write page\n");	
	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	envid_t envid=sys_getenvid();
	sys_page_alloc(envid,(void*)PFTEMP,PTE_P|PTE_U|PTE_W);
	memcpy((void*)PFTEMP,(void*)ROUNDDOWN(addr,PGSIZE),PGSIZE);
	sys_page_map(envid,(void*)PFTEMP,envid,(void*)ROUNDDOWN(addr,PGSIZE),PTE_W|PTE_U|PTE_P);
	sys_page_unmap(envid,(void*)PFTEMP);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
	// LAB 4: Your code here.
	if((uvpt[pn]&(PTE_W|PTE_COW))){
		if(sys_page_map(sys_getenvid(),(void*)(pn*PGSIZE),envid,(void*)(pn*PGSIZE),PTE_COW|PTE_U|PTE_P)<0)
			panic("Fail to map env %x to env %x at address %x\n",sys_getenvid(),envid,pn*PGSIZE);
		if(sys_page_map(sys_getenvid(),(void*)(pn*PGSIZE),sys_getenvid(),(void*)(pn*PGSIZE),PTE_COW|PTE_U|PTE_P)<0)
			panic("Fail to map env %x to env %x at address %x\n",sys_getenvid(),envid,pn*PGSIZE);	
	}
	else if(sys_page_map(sys_getenvid(),(void*)(pn*PGSIZE),envid,(void*)(pn*PGSIZE),uvpt[pn]&PTE_SYSCALL)<0)
			panic("Fail to map env %x to env %x at address %x\n",sys_getenvid(),envid,pn*PGSIZE);

	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	set_pgfault_handler(pgfault);
	envid_t child_id;
	if((child_id=sys_exofork())<0)
		panic("Fail to create a new environment\n");
	if(child_id==0){
		//child process
		thisenv=&envs[ENVX(sys_getenvid())];
		return 0;
	}	
	for(int pdx=0;pdx<PDX(UTOP);pdx++){
		if(uvpd[pdx]&PTE_P){
			//page-table exist
			for(int ptx=0;ptx<1024;ptx++){
				uint32_t pn=pdx*1024+ptx;
				if(pn!=PGNUM(UXSTACKTOP-PGSIZE)&&(uvpt[pn]&PTE_P)){
					//page exist and not UXSTACK
					duppage(child_id,pn);
				}
			}
		}
	}
	if(sys_page_alloc(child_id,(void*)(UXSTACKTOP-PGSIZE),PTE_P|PTE_U|PTE_W)<0)
		panic("Fail to allocate a page for UXSTACKTOP\n");
	if(sys_env_set_pgfault_upcall(child_id,_pgfault_upcall)<0)
		panic("Fail to set child env's page fault upcall\n");
	if(sys_env_set_status(child_id,ENV_RUNNABLE)<0)
		panic("Fail to makr child runnable\n");
	return child_id;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
