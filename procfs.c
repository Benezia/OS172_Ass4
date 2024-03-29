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
int ninodes = 0;

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
  char numContainer[10] = {0};
  itoa(numContainer, num);
  appendToBufEnd(buff, numContainer);
}

void appendDirentToBufEnd(char *buff, char * dirName, int inum, int dPlace){
  struct dirent dir;
	dir.inum = inum;
	memmove(&dir.name, dirName, strlen(dirName)+1);
  memmove(buff + dPlace*sizeof(dir) , (void*)&dir, sizeof(dir));
}


int fillProcDirents(char *ansBuf){
	appendDirentToBufEnd(ansBuf,".", namei("/proc")->inum, 0);
	appendDirentToBufEnd(ansBuf,"..", namei("")->inum, 1);
	appendDirentToBufEnd(ansBuf,"blockstat", ninodes+1, 2);
  appendDirentToBufEnd(ansBuf,"inodestat", ninodes+2, 3);
  int pids[NPROC] = {0};
  getValidSlots(pids);
  int i, j = 4;
  char numContainer[3] = {0};
  for (i = 0; i<NPROC; i++){
  	if (pids[i] != 0){
  		numContainer[0] = 0;
  		numContainer[1] = 0;
  		numContainer[2] = 0;
	    itoa(numContainer, pids[i]);
	    appendDirentToBufEnd(ansBuf,numContainer, ninodes+(i+1)*100, j);
	    j++;
  	}
  }
 	return sizeof(struct dirent)*j;
}

int fillPIDDirents(char *ansBuf){
	short slot = ansBuf[0];
	int pid = getSlotPID(slot);
  char dirPath[9] = {0};
  appendToBufEnd(dirPath, "/proc/");
	appendNumToBufEnd(dirPath, pid);
	appendDirentToBufEnd(ansBuf,".", namei(dirPath)->inum, 0);
	appendDirentToBufEnd(ansBuf,"..", namei("/proc")->inum, 1);
  appendDirentToBufEnd(ansBuf,"fdinfo", ninodes+1+(slot+1)*100, 2);
  appendDirentToBufEnd(ansBuf,"status", ninodes+2+(slot+1)*100, 3);
  struct inode *cwd = (struct inode*)getCWDinode(pid);
  appendDirentToBufEnd(ansBuf,"cwd", cwd->inum, 4);
	return sizeof(struct dirent)*5;
}

int fillfdInfoDirents(char *ansBuf){
	short slot = ansBuf[0];
	int pid = getSlotPID(slot);
  char dirPath[17] = {0};
  appendToBufEnd(dirPath, "/proc/");
	appendNumToBufEnd(dirPath, pid);
	appendDirentToBufEnd(ansBuf,"..", namei(dirPath)->inum, 1);
  appendToBufEnd(dirPath, "/fdinfo/");
	appendDirentToBufEnd(ansBuf,".", namei(dirPath)->inum, 0);
	struct file** fdList = getOpenfd(slot);
	int i;
	int j = 2;
	char numContainer[3] = {0};
	for (i = 0; i < NOFILE; i++){
		if (fdList[i] > 0){
	    itoa(numContainer, i);
		  appendDirentToBufEnd(ansBuf,numContainer, ninodes+10+(slot+1)*100+i , j);
		  j++;
		}
	}
	return sizeof(struct dirent)*j;
}

int blockstat(char *ansBuf){
  appendToBufEnd(ansBuf, "Free blocks: ");
	appendNumToBufEnd(ansBuf, getFreeBlockCount());
	appendToBufEnd(ansBuf,"\nTotal blocks: ");
  appendNumToBufEnd(ansBuf, NBUF);
	appendToBufEnd(ansBuf,"\nHit ratio: ");
  appendNumToBufEnd(ansBuf, getHitCount());
	appendToBufEnd(ansBuf,"/");
  appendNumToBufEnd(ansBuf, getAccessCount());
  appendToBufEnd(ansBuf,"\n");
	return strlen(ansBuf);
}

int inodestat(char *ansBuf){
	int freeInodeCount = getFreeInodeCount();
	appendToBufEnd(ansBuf, "Free inodes: ");
  appendNumToBufEnd(ansBuf, freeInodeCount);
  appendToBufEnd(ansBuf, "\nValid inodes: ");
  appendNumToBufEnd(ansBuf, getValidInodeCount());
  appendToBufEnd(ansBuf, "\nRefs per inode: ");
  appendNumToBufEnd(ansBuf, getTotalRefCount());
  appendToBufEnd(ansBuf,"/");
  appendNumToBufEnd(ansBuf, NINODE-freeInodeCount);
  appendToBufEnd(ansBuf,"\n");
  return strlen(ansBuf);
}


int fdinfo(char *ansBuf){
  short slot = ansBuf[0];
  ansBuf[0] = 0;
  int fd = ansBuf[1];
  ansBuf[1] = 0;

  static char *fileTypes[] = {
    [FD_NONE]   "none",
    [FD_PIPE]   "pipe",
    [FD_INODE]  "inode",
  };

  struct file **ofile = getOpenfd(slot);
  if (ofile == 0)
  	return 0;

  struct file *f = ofile[fd];
  if (f == 0)
    return 0;


	appendToBufEnd(ansBuf, "type\t");
	appendToBufEnd(ansBuf, fileTypes[f->type]);
	appendToBufEnd(ansBuf, "\ninum\t");
  appendNumToBufEnd(ansBuf, f->ip->inum);
	appendToBufEnd(ansBuf, "\nref\t");
  appendNumToBufEnd(ansBuf, f->ip->ref);
	appendToBufEnd(ansBuf, "\npos\t");
  appendNumToBufEnd(ansBuf, f->off);
	appendToBufEnd(ansBuf, "\nflags\t");
  if (f->readable)
		appendToBufEnd(ansBuf, "R");
  if (f->writable)
		appendToBufEnd(ansBuf, "W");
	appendToBufEnd(ansBuf, "\n");
	return strlen(ansBuf);
}

procfs_func map(struct inode *ip){
	if (ip->inum < ninodes) 					// proc folder
		return &fillProcDirents;
	if (ip->inum == (ninodes+1))			// blockstat file
		return &blockstat;
  if (ip->inum == (ninodes+2))			// inodestat file
		return &inodestat;
	if (ip->inum % 100 == 0)
		return &fillPIDDirents;					// pid folder
	if (ip->inum % 100 == 1)
		return &fillfdInfoDirents;			// fd foler
	if (ip->inum % 100 == 2)
		return &status;									// status file
	int fd = (ip->inum % 100) - 10;
	if (fd >= 0 && fd <= NOFILE)
		return &fdinfo; 								// fdinfo file
	
	cprintf("Invalid inum\n");
	return 0;
}


int procfsisdir(struct inode *ip) {
	if (ninodes == 0){
		struct superblock sb;
  	readsb(ip->dev, &sb);
  	ninodes = sb.ninodes;
  }
	if (!(ip->type == T_DEV) || !(ip->major == PROCFS))
		return 0;
	int inum = ip->inum;
	if (inum == (ninodes+1) || inum == (ninodes+2))
		return 0; //blockstat and inodestat are files
	return (inum < ninodes || inum % 100 == 0 || inum % 100 == 1);
}

void procfsiread(struct inode* dp, struct inode *ip) {
	ip->flags |= I_VALID; 
	ip->major = PROCFS;
	ip->type = T_DEV;
}

int procfsread(struct inode *ip, char *dst, int off, int n) {
	if (ninodes == 0){
		struct superblock sb;
  	readsb(ip->dev, &sb);
  	ninodes = sb.ninodes;
  }
	char ansBuf[1056] = {0}; //longest data is 66 dirents * 16 bytes
	int ansSize;
	procfs_func f = map(ip);
	short slot = 0;
	if (ip->inum >= ninodes+100){
		slot = (ip->inum-ninodes)/100 - 1;
		ansBuf[0] = slot;
		short midInum = ip->inum % 100;
		if (midInum >= 10)
			ansBuf[1] = midInum-10;
	}
	ansSize = f(ansBuf);
	memmove(dst, ansBuf+off, n);
	return min(n, ansSize-off);
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