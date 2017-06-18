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
  if (n == 0){
    s[0] = '0';
    return;
  }
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

void appendToBufEnd(char *buff, char * text){
  int textlen = strlen(text);
  int sz = strlen(buff);
  memmove(buff + sz, text,textlen);
}

void appendNumToBufEnd(char *buff, int num){
  char numContainer[4] = {0};
  itoa(numContainer, num);
  appendToBufEnd(buff, numContainer);
}

void appendDirentToBufEnd(char *buff, char * dirName, int inum, int dPlace){
  struct dirent dir;
	dir.inum = inum;
	memmove(&dir.name, dirName, strlen(dirName)+1);
  memmove(buff + dPlace*sizeof(dir) , (void*)&dir, sizeof(dir));
}


int blockstat(char *ansBuf){
  appendToBufEnd(ansBuf, "Free blocks: ");
	appendNumToBufEnd(ansBuf, getFreeBlockCount());
	appendToBufEnd(ansBuf,"\nTotal blocks: ");
  appendNumToBufEnd(ansBuf, NBUF);
	appendToBufEnd(ansBuf,"\nHit ratio: ");
  appendNumToBufEnd(ansBuf, getHitCount()/getAccessCount());
  appendToBufEnd(ansBuf,"\n");
  //Implement as float??
	return strlen(ansBuf);
}

int fillProcDirents(char *ansBuf){
	appendDirentToBufEnd(ansBuf,"blockstat", 201, 0);
  appendDirentToBufEnd(ansBuf,"inodestat", 202, 1);
  int pids[NPROC];
  int initPIDs = getValidPIDs(pids);
  int i;
  char numContainer[2] = {0};
  for (i = 0; i<initPIDs; i++){
    itoa(numContainer, pids[i]);
    appendDirentToBufEnd(ansBuf,numContainer, 200+pids[i]*100, i+2);
  }
  
  //TODO: add pid dirents
	return sizeof(struct dirent)*(i+2);
}
int fillPIDDirents(char *ansBuf, int pid){
  appendDirentToBufEnd(ansBuf,"fdinfo", 201 + pid*100, 0);
  appendDirentToBufEnd(ansBuf,"status", 202, 1);
	return sizeof(struct dirent)*3;
  //appendDirentToBufEnd(ansBuf,"cwd", , 2);

}

void fillfdInfoDirents(char *ansBuf){

}
int inodestat(char *ansBuf){
	int freeInodeCount = getFreeInodeCount();
	appendToBufEnd(ansBuf, "Free inodes: ");
  appendNumToBufEnd(ansBuf, freeInodeCount);
  appendToBufEnd(ansBuf, "\nValid inodes: ");
  appendNumToBufEnd(ansBuf, NINODE);
  appendToBufEnd(ansBuf, "\nRefs per inode: ");
  appendNumToBufEnd(ansBuf, getTotalRefCount()/(NINODE-freeInodeCount));
  appendToBufEnd(ansBuf,"\n");
	//TODO: Represent refs as float
  return strlen(ansBuf);
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
	if (ip->inum < 200) //inode from disk, must be the /proc inode
		return &fillProcDirents;
	
	if (ip->inum == 201)
		return &blockstat;
	
  if (ip->inum == 202)
		return &inodestat;
	
	cprintf("map failed\n");
	return 0;
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
	char ansBuf[512] = {0};
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
