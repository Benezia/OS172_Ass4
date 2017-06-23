#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

//Puts the string between c1 to c2 from index 'from' from 'buf' to 'dst'
int getContent(char * dst, char *buf, char c1, char c2, int from){
	int i;
  for (i=0; i<10; i++){
  	dst[i] = 0; //clear buffer
  }
	int s = indexof(buf,c1,from);
	int e = indexof(buf,c2,s+1);
  memmove(dst, buf+s+1, e-s-1);
  return e;
}

void printfd(char * buf, char * path){
	char tempBuf[10];
	char type[6];

  int e = getContent(tempBuf, path, '/', '/', 0);
  int pid = atoi(tempBuf);

  e = getContent(tempBuf, path, '/', 0, e+1);
  int fd = atoi(tempBuf);

  e = getContent(type, buf, '\t', '\n', 0);

  e = getContent(tempBuf, buf, '\t', '\n', e);
  int inum = atoi(tempBuf);

  getContent(tempBuf, buf, '\t', '\n', e);
  int refs = atoi(tempBuf);

  printf(1, "%d %d %d %d %s\n",pid, fd, refs, inum, type);
//<PID> <FD number> <refs for file descriptor> <Inode number> <file type>
}

void openfd(char *path){
	char buf[512];
	int fd;
  if((fd = open(path, 0)) < 0){
    printf(2, "cannot open %s\n", path);
    return;
  }
  int n;
  if((n = read(fd, buf, sizeof(buf))) > 0)
    printfd(buf, path);
  if(n < 0)
    printf(2, "read error\n");
  close(fd);
}

int isdir(char * dirName, int inum){
	if (strcmp(dirName, "fdinfo") == 0)
		return 1;
	return (inum % 100 == 0) && (*dirName != '.');
}

int isfd(int inum){
	return (inum % 100 >= 10 && inum % 100 <= 25 && inum > 200);
}

void lsof(char *path){
  char buf[512], *p;
  int fd;
  struct dirent de;

  if((fd = open(path, 0)) < 0){
    printf(2, "lsof: cannot open %s\n", path);
    return;
  }
  
  if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
    printf(1, "lsof: path too long\n");
  strcpy(buf, path);
  p = buf+strlen(buf);
  *p++ = '/';
  while(read(fd, &de, sizeof(de)) == sizeof(de)){
    if(de.inum == 0)
      continue;
    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;
    if (isfd(de.inum))
    	openfd(buf);
    if (isdir(de.name, de.inum)) //a dir of pid/fd
    	lsof(buf);
  }
  close(fd);
}

int main(int argc, char *argv[]){
  lsof("proc");
  exit();
}
