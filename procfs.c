#include "types.h"
#include "stat.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "buf.h"


void blockstat(){
	cprintf("Free blocks: %d\n", getFreeBlockCount());
	cprintf("Total blocks: %d\n", NBUF);
	cprintf("Hit ratio: %d/%d\n", getHitCount(), getAccessCount());
	//TODO: Change cprintf to a char* string
}

void inodestat(){
	int freeInodeCount = getFreeInodeCount();
	cprintf("Free inodes: %d\n", freeInodeCount);
	cprintf("Valid inodes: %d\n", NINODE);
	cprintf("Refs per inode: %d\n", getTotalRefCount()/(NINODE-freeInodeCount));
	//TODO: Change cprintf to a char* string
}

int procfsisdir(struct inode *ip) {
	return (ip->type == T_DEV && ip->major == PROCFS) || ip->type == T_DIR;
}

void procfsiread(struct inode* dp, struct inode *ip) {
	//called from dirlookup.
	//ip = A recycled inode returned in dirlookup (from iget)
	ip->flags |= I_VALID; //prevents ilock (fs.c) from updating this inode
	//ip->type = T_DEV OR T_FILE;

	cprintf("iread\n");
}

int procfsread(struct inode *ip, char *dst, int off, int n) {
	cprintf("procfs read\n");
	//Called by using read syscall on the proc T_DEV fd.
	//Copies n bytes of ip's data to 'dst', starting at 'off'.
	//if ip is a directory: A list of dirents will be copied (used by ls.c for example).
	//if ip is a file: The output string will be copied
	
  return 0;
}

int procfswrite(struct inode *ip, char *buf, int n) {
	panic("Write in a read only system");
  return 0;
}

void procfsinit(void) {
  devsw[PROCFS].isdir = procfsisdir;
  devsw[PROCFS].iread = procfsiread;
  devsw[PROCFS].write = procfswrite;
  devsw[PROCFS].read = procfsread;
}

/*
IDEA OF IMPLEMENTATION:
Reserve node #51 for blockstat
Reserve node #52 for inodestat


*/
