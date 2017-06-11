#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

int main(int argc, char *argv[]) {
  open("proc", O_RDWR);

  printf(3, "blalala\n");
  exit();
}
