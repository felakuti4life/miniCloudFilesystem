#include <stdio.h>
#include "protocol.h"

typedef struct _serverState {
  NetworkState* network;
  size_t mountpath_length;
  char* mountpath;
} ServerState;

typedef struct _writeQueue {
  WriteFilePacket* packet;
  _writeQueue* next;
} WriteQueue;

typedef struct _servedFileState {
  ServerState* server;

  unsigned int open;
  int fd;
  size_t fpath_length;
  char* fpath;
  unsigned int nBlocks;
  unsigned int transactionid;
  //write queue;
  WriteQueue* queue;
} ServedFileState;

char* makeFullPath(ServedFileState* s);
int commitQueue(ServedFileState* s);

int handleOpenFile(void* packet, ServedFileState* s);
int handleWriteFile(void* packet, ServedFileState* s);
int handleCommit(void* packet, ServedFileState* s);
int handleAbort(void* packet, ServedFileState* s);
int handleClose(void* packet, ServedFileState* s);

//clean up things
int cleanUpServerState(ServerState* s);
int cleanUpServedFileState(ServedFileState* s);
int cleanUpWriteQueue(WriteQueue* q);

int mainLoop(ServedFileState* f,unsigned short port, float droprate);
