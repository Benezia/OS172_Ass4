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
#define min(a, b) ((a) < (b) ? (a) : (b))


typedef int (*procfs_func)(char*);

void itoa(char *s, int n){
	int i = 0;
	int len = 0;
	while(n != 0){
		s[len] = n % 10 + '0';
		n = n / 10; 
		len++;
	}
	for(i = 0; i < len/2; i++){
		char tmp = s[i];
		s[i] = s[len - 1 - i];
		s[len - 1 - i] = tmp;
	}
}


int blockstat(char *ansBuf){
	int sz = 0;
	char numContainer[4] = {0};
	memmove(ansBuf + sz, "Free blocks: ",strlen("Free blocks: "));
	sz += strlen("Free blocks: ");
	itoa(numContainer, getFreeBlockCount());
	memmove(ansBuf + sz, numContainer,strlen(numContainer));
	sz += strlen(numContainer);
	memmove(ansBuf + sz, "\n",1);
	sz += 1;
	return sz;
	//cprintf("Free blocks: %d\n", getFreeBlockCount());
	//cprintf("Total blocks: %d\n", NBUF);
	//cprintf("Hit ratio: %d/%d\n", getHitCount(), getAccessCount());
	//TODO: Change cprintf to a char* string
}

int fillProcDirents(char *ansBuf){
	struct dirent blockstat;
	blockstat.inum = 201;
	memmove(&blockstat.name, "blockstat\0", strlen("blockstat")+1);
	memmove(ansBuf, (void*)&blockstat, sizeof(blockstat));
	return sizeof(blockstat);
}
void fillPIDDirents(char *ansBuf){

}
void fillfdInfoDirents(char *ansBuf){

}
void inodestat(){
	int freeInodeCount = getFreeInodeCount();
	cprintf("Free inodes: %d\n", freeInodeCount);
	cprintf("Valid inodes: %d\n", NINODE);
	cprintf("Refs per inode: %d\n", getTotalRefCount()/(NINODE-freeInodeCount));
	//TODO: Change cprintf to a char* string and represent refs as float
}

void fdinfo(int pid, int fd){
  static char *fileTypes[] = {
    [FD_NONE]   "None",
    [FD_PIPE]   "Pipe",
    [FD_INODE]  "Inode",
  };

  struct file **ofile = getOpenfd(pid);
  if (ofile == 0)
  	return;

  struct file *f = ofile[fd];
  if (f == 0)
    return;

  cprintf("Proc %d fd %d info: \n",pid, fd);
  cprintf("Type: %s\n", fileTypes[f->type]);
  cprintf("Position: %d\n", f->off);
  cprintf("Flags: ");
  if (f->readable)
    cprintf("R");
  if (f->writable)
    cprintf("W");
  cprintf("\n");
  cprintf("Inum: %d\n", f->ip->inum);
}

procfs_func map(struct inode *ip){
	if (ip->inum < 200){ //inode from disk, must be the /proc inode
		//cprintf("map proc dirents\n");
		return &fillProcDirents;
	}
	if (ip->inum == 201){
		//cprintf("map file blockstat\n");
		return &blockstat;
	}
	ip->type = 0;
	cprintf("map failed\n");
	return 0;
	//TODO
}


int procfsisdir(struct inode *ip) {
	if (!(ip->type == T_DEV) || !(ip->major == PROCFS))
		return 0;
	int inum = ip->inum;
	if (inum == 201 || inum == 202)
		return 0; //blockstat and inodestat are files
	return (inum < 200 || inum % 100 == 0 || inum % 100 == 1);
}

void procfsiread(struct inode* dp, struct inode *ip) {
	//cprintf("iread inum: %d\n", ip->inum);
	ip->flags |= I_VALID; 
	ip->major = 2;
	ip->type = T_DEV;
}

int procfsread(struct inode *ip, char *dst, int off, int n) {
	char ansBuf[512];
	int ansSize;
	procfs_func f = map(ip);
	ansSize = f(ansBuf);
	memmove(dst, ansBuf+off, n);
	//cprintf("procfs read: %d, %d\n", ip->inum, min(n, ansSize-off));
	return min(n, ansSize-off);
	//Called by using read syscall on the proc T_DEV fd.
	//Copies n bytes of ip's data to 'dst', starting at 'off'.
	//if ip is a directory: A list of dirents will be copied (used by ls.c for example).
	//if ip is a file: The output string will be copied
	
}

int procfswrite(struct inode *ip, char *buf, int n) {
	panic("Write invoked in a read only system");
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
Reserve inode #201 for blockstat file
Reserve inode #202 for inodestat file

Reserve inode #(200+pid*100) for pid folder (max num: NPROC*100 = 6600)
Reserve inode #(200+pid*100+1) for fdinfo folder
Reserve inode #(200+pid*100+2) for status file
Reserve inode #(200+pid*100+10+fd) for each open fd (max num: 200+pid*100+10+NOFILE = pid*100+225)

Working with cwd:
When procfsread is invoked on pid folder, return following dirent (name, inum) list:
- "cwd", proc->cwd->inum
- "fdinfo", 200+pid*100+1
- "status", 200+pid*100+2
*/
