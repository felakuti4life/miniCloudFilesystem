#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <poll.h>

//packet defs

//config
#define MAX_FPATH_LENGTH 256
#define MAX_WRITE_SIZE 512
#define GROUP 0xe0010101
#define PORT 44017

//packet types
#define PTYPE unsigned int
#define PTYPE_OPENFILE 0x1
#define PTYPE_OPENSUCCESS 0x2
#define PTYPE_WRITEFILE 0x3
#define PTYPE_COMMIT 0x4
#define PTYPE_COMMITSUCCESS 0x5
#define PTYPE_ABORT 0x6
#define PTYPE_CLOSE 0x7
#define PTYPE_CLOSESUCCESS 0x8

//nodes
#define NTYPE_SERVER 0x1
#define NTYPE_CLIENT 0x2

typedef struct _openFilePacket {
  unsigned int ntype;
  unsigned int transactionid;
  size_t fpath_length;
  char* fpath;
} OpenFilePacket;

typedef struct _openSuccessPacket {
  unsigned int ntype;
  int fd;
  unsigned int transactionid;
  int success;
} OpenSuccessPacket;

typedef struct _writeFilePacket  {
  unsigned int ntype;
  int fd;
  unsigned int transactionid;
  unsigned int counter;
  int blockid;
  int offset;
  size_t sz;
  char* data;
} WriteFilePacket;

typedef struct _commitPacket {
  unsigned int ntype;
  unsigned int transactionid;
  int fd;
  int success;
} CommitPacket;
  
typedef struct _commitSuccessPacket {
  unsigned int transactionid;
  int fd;
  unsigned int success;
} CommitSuccessPacket;

typedef struct _abortPacket {
  unsigned int ntype;
  unsigned int transactionid;
  int fd;
} AbortPacket;

typedef struct _closePacket {
  unsigned int ntype;
  unsigned int transactionid;
  int fd;
  size_t fpath_length;
  char* fpath;

} ClosePacket;

typedef struct _closeSuccessPacket {
  unsigned int transactionid;
  int fd;
  unsigned int success;
} CloseSuccessPacket;

//packet handling
//both of these functions return a null pointer on failiure
char* serializePacket(PTYPE p_t, void* packet, size_t* str_sz);
void* deserializePacket(PTYPE *p_t, char* str);

//networking
typedef struct _networkState {
  int groupId;
  unsigned short port;
  int socketId;
  struct sockaddr_in socketAddress;
  float droprate;
  unsigned int my_ntype;
  int status; //1 connected, 0 disconnected
} NetworkState;

int startNetwork(int groupId, unsigned short port, unsigned int my_ntype, NetworkState* network);
int sendPacket(PTYPE p_t, void* packet, NetworkState* network);
void* receivePacket(PTYPE *p_t, NetworkState* network);
