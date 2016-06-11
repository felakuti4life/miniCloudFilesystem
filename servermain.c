#include "serve.h"
int main(int argc, char **argv)
{
  ServedFileState sf;
  ServerState s;
  sf.server = &s;
  if(argc!=7){
    printf("wrong arguments!\nproper usage: ./replFsServer -port 00000 -mountpath file/path droprate 10\n");
    exit(-1);
  }
  s.mountpath = argv[4];
  s.mountpath_length = strlen(argv[4]);
  int port=atoi(argv[2]);
  float droprate=((float)atoi(argv[6]))/100.0f;
  return mainLoop(&sf,port,droprate);
}
