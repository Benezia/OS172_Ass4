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

int procfsisdir(struct inode *ip) {
	cprintf("isdir: %d\n", ip->type);
	return (ip->type == T_DEV && ip->major == PROCFS) || ip->type == T_DIR;
}

void procfsiread(struct inode* dp, struct inode *ip) {
	//called from dirlookup.
	//ip = A recycled inode returned in dirlookup (from iget)
	ip->flags |= I_VALID; //prevents ilock (fs.c) from updating this inode 
	//dp = the /proc T_DEV inode

	//TODO: initialize ip correctly.
	cprintf("iread\n");
}

int procfsread(struct inode *ip, char *dst, int off, int n) {
	cprintf("read\n");
	//Called by using read syscall on the proc T_DEV fd.
	//Copies n bytes of ip's data to 'dst', starting at 'off'.
	//if ip is a directory: A list of dirents will be copied (used by ls.c for example).
	//if ip is a file: The output string will be copied
	
  return 0;
}

int procfswrite(struct inode *ip, char *buf, int n) {
	//Should stay unused IMO ("You must implement the procfs as a read-only file system")
	cprintf("procfswrite: %s\n",buf);
  return 0;
}

void procfsinit(void) {
	cprintf("procfsinit\n");
  devsw[PROCFS].isdir = procfsisdir;
  devsw[PROCFS].iread = procfsiread;
  devsw[PROCFS].write = procfswrite;
  devsw[PROCFS].read = procfsread;
}