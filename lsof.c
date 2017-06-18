#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

char* fmtname(char *path){
  static char buf[DIRSIZ+1];
  char *p;
  
  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--);
  p++;
  
  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

int isdir(int inum) {
	if (inum == 201 || inum == 202)
		return 0; //blockstat and inodestat are files
	return (inum < 200 || inum % 100 == 0 || inum % 100 == 1);
}

void lsof(char *path){
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;
  
  if((fd = open(path, 0)) < 0){
    printf(2, "lsof: cannot open %s\n", path);
    return;
  }
  
  if(fstat(fd, &st) < 0){
    printf(2, "lsof: cannot stat %s\n", path);
    close(fd);
    return;
  }
  if (isdir(st.ino)){
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf(1, "ls: path too long\n");
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf(1, "ls: cannot stat %s\n", buf);
        continue;
      }
      printf(1, "%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
    }
	} else {
	    printf(1, "%s %d %d %d\n", fmtname(path), st.type, st.ino, st.size);
		}
	}
}

int main(int argc, char *argv[]){
  lsof("proc");
  exit();
}
