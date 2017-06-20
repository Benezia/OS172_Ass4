#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

void printfd(char * buf, char * path){
	char tempBuf[10] = {0};
	char type[6] = {0};

	int s = indexof(path,'/',0);
	int e = indexof(path,'/',s+1);
  memmove(tempBuf, path+s+1, e-s);
  int pid = atoi(tempBuf);

  int i;
  for (i=0; i<10; i++){
  	tempBuf[i] = 0; //clear buffer
  }
	s = indexof(path,'/',e+1);
	e = strlen(path);
  memmove(tempBuf, path+s+1, e-s);
  int fd = atoi(tempBuf);

	s = indexof(buf,'\t',0);
	e = indexof(buf,'\n',0);
  memmove(type, buf+s+1, e-s-1);

 	for (i=0; i<10; i++){
  	tempBuf[i] = 0; //clear buffer
  }
	s = indexof(buf,'\t',e);
	e = indexof(buf,'\n',s);
  memmove(tempBuf, buf+s+1, e-s);
  int inum = atoi(tempBuf);

 	for (i=0; i<10; i++){
  	tempBuf[i] = 0; //clear buffer
  }
	s = indexof(buf,'\t',e);
	e = indexof(buf,'\n',s);
  memmove(tempBuf, buf+s+1, e-s);
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
  	//write(1, buf, n);
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
  struct stat st;
  //printf(1,"parent: %s\n",path);

  if((fd = open(path, 0)) < 0){
    printf(2, "lsof: cannot open %s\n", path);
    return;
  }
  
  if(fstat(fd, &st) < 0){
    printf(2, "lsof: cannot stat %s\n", path);
    close(fd);
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
    //printf(1,"\tpath: %s\n",buf);
    if(stat(buf, &st) < 0){
      printf(1, "lsof: cannot stat %s\n", buf);
      continue;
    }

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
