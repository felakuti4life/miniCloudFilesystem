#include "protocol.h"

#define PTYPE_SZ sizeof(int)

char* serializePacket(PTYPE p_t, void* packet, size_t* str_sz) {
  void* str;
  size_t sz = 0;
  if(p_t == PTYPE_OPENFILE) {
    sz = sizeof(OpenFilePacket) + PTYPE_SZ+MAX_FPATH_LENGTH;
    str = malloc(sz);
    memcpy(str, &p_t, PTYPE_SZ);
    memcpy(str+PTYPE_SZ, packet,sizeof(OpenFilePacket));
    memcpy(str+PTYPE_SZ+sizeof(OpenFilePacket),((OpenFilePacket*)packet)->fpath, ((OpenFilePacket*)packet)->fpath_length);
  }
  else if(p_t == PTYPE_OPENSUCCESS) {
    sz = sizeof(OpenSuccessPacket) + PTYPE_SZ;
    str = malloc(sz);
    memcpy(str, &p_t, PTYPE_SZ);
    memcpy(str+PTYPE_SZ, packet,sizeof(OpenSuccessPacket));
  }
  else if(p_t == PTYPE_WRITEFILE) {
    sz = sizeof(WriteFilePacket) + PTYPE_SZ + ((WriteFilePacket*)packet)->sz;
    str = malloc(sz);
    memcpy(str, &p_t, PTYPE_SZ);
    memcpy(str+PTYPE_SZ, packet,sizeof(WriteFilePacket));
    memcpy(str+PTYPE_SZ+sizeof(WriteFilePacket),
	   ((WriteFilePacket*)packet)->data, 
	   ((WriteFilePacket*)packet)->sz);
  }
  else if(p_t == PTYPE_COMMIT) {
    sz = sizeof(CommitPacket) + PTYPE_SZ;
    str = malloc(sz);
    memcpy(str, &p_t, PTYPE_SZ);
    memcpy(str+PTYPE_SZ, packet,sizeof(CommitPacket));
  }
  else if(p_t == PTYPE_COMMITSUCCESS) {
    sz = sizeof(CommitSuccessPacket) + PTYPE_SZ;
    str = malloc(sz);
    memcpy(str, &p_t, PTYPE_SZ);
    memcpy(str+PTYPE_SZ, packet,sizeof(CommitSuccessPacket));
  }
  else if(p_t == PTYPE_ABORT) {
    sz = sizeof(AbortPacket) + PTYPE_SZ;
    str = malloc(sz);
    memcpy(str, &p_t, PTYPE_SZ);
    memcpy(str+PTYPE_SZ, packet,sizeof(AbortPacket));
  }
  else if(p_t == PTYPE_CLOSE) {
    sz = sizeof(ClosePacket) + PTYPE_SZ + ((ClosePacket*)packet)->fpath_length;
    str = malloc(sz);
    memcpy(str, &p_t, PTYPE_SZ);
    memcpy(str+PTYPE_SZ, packet,sizeof(ClosePacket));
    memcpy(str+PTYPE_SZ+sizeof(ClosePacket),((ClosePacket*)packet)->fpath,((ClosePacket*)packet)->fpath_length);
  }
  else if(p_t == PTYPE_CLOSESUCCESS) {
    sz = sizeof(CloseSuccessPacket) + PTYPE_SZ;
    str = malloc(sz);
    memcpy(str, &p_t, PTYPE_SZ);
    memcpy(str+PTYPE_SZ, packet,sizeof(CloseSuccessPacket));
  }
  *str_sz = sz;
  return (char*) str;
};

void* deserializePacket(PTYPE *p_t, char* str) {
  memcpy(p_t, str, PTYPE_SZ);
  //dereference for performance?
  PTYPE p = *p_t;

  void* packet;
  if(p == PTYPE_OPENFILE) {
    size_t sz = sizeof(OpenFilePacket);
    packet = malloc(sz);
    memcpy(packet,str+PTYPE_SZ,sz);
    OpenFilePacket* p = (OpenFilePacket*) packet;
    p->fpath = (char*) malloc(p->fpath_length);
    memcpy(p->fpath, str+PTYPE_SZ+sz,p->fpath_length);
  }
  else if(p == PTYPE_OPENSUCCESS) {
    size_t sz = sizeof(OpenSuccessPacket);
    packet = malloc(sz);
    memcpy(packet,str+PTYPE_SZ,sz);
  }
  else if(p == PTYPE_WRITEFILE) {
    size_t sz = sizeof(WriteFilePacket);
    packet = malloc(sz);
    memcpy(packet,str+PTYPE_SZ,sz);
    size_t dataSz = ((WriteFilePacket*)packet)->sz;
    ((WriteFilePacket*)packet)->data = (char*) malloc(dataSz);
    memcpy(((WriteFilePacket*)packet)->data,str+PTYPE_SZ+sizeof(WriteFilePacket),dataSz);
  }
  else if(p == PTYPE_COMMIT) {
    size_t sz = sizeof(CommitPacket);
    packet = malloc(sz);
    memcpy(packet, str+PTYPE_SZ,sz);
  }
  else if(p == PTYPE_COMMITSUCCESS) {
    size_t sz = sizeof(CommitSuccessPacket);
    packet = malloc(sz);
    memcpy(packet, str+PTYPE_SZ,sz);
  }
  else if(p == PTYPE_ABORT) {
    size_t sz = sizeof(AbortPacket);
    packet = malloc(sz);
    memcpy(packet, str+PTYPE_SZ,sz);
  }
  else if(p == PTYPE_CLOSE) {
    size_t sz = sizeof(ClosePacket);
    packet = malloc(sz);
    memcpy(packet, str+PTYPE_SZ,sz);
    ClosePacket* p = (ClosePacket*)packet;
    p->fpath = (char*) malloc(p->fpath_length);
    memcpy(p->fpath,str+PTYPE_SZ+sz,p->fpath_length);
  }
  else if(p == PTYPE_CLOSESUCCESS) {
    size_t sz = sizeof(CloseSuccessPacket);
    packet = malloc(sz);
    memcpy(packet, str+PTYPE_SZ,sz);
  }
  return packet;
}

int startNetwork(int groupId, unsigned short port, unsigned int my_ntype, NetworkState* network) {
  network->port = port;
  network->my_ntype = my_ntype;
  int err = 0;
  struct sockaddr_in socketAddress;
  //initialize socket
  network->socketId = socket(AF_INET, SOCK_DGRAM,0);
  err = network->socketId;
  if(err < 0) printf("Error getting socket.\n");
  int uses = 1;
  err= setsockopt(network->socketId, SOL_SOCKET, SO_REUSEADDR,&uses,sizeof(int));
  if(err<0) printf("error setting options for socket.\n");
  socketAddress.sin_port = htons(port);
  socketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  socketAddress.sin_family = AF_INET;
  err = bind(network->socketId, (sockaddr*)&socketAddress,sizeof(socketAddress));
  if(err<0) printf("error binding socket\n");
  
  //MREQ stuff
  struct ip_mreq mreq;
  mreq.imr_multiaddr.s_addr = htonl(groupId);
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  err = setsockopt(network->socketId, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &mreq, sizeof(mreq));
  if(err<0) printf("Error setting mreq options for socket.");
  
  //TTL stuff
  u_char ttlCode = 1; //subnet
  err = setsockopt(network->socketId, IPPROTO_IP, IP_MULTICAST_TTL, &ttlCode, sizeof(ttlCode));
  if(err<0) printf("Error setting TTL options for socket.\n");

  socketAddress.sin_addr.s_addr = htonl(groupId);
  //We're done!
  memcpy(&(network->socketAddress),&socketAddress, sizeof(socketAddress));
  return err;
}

int sendPacket(PTYPE p_t, void* packet, NetworkState* network) {
  int err=0;
  size_t sz;
  char* msg = serializePacket(p_t, packet,&sz);
  err = sendto(network->socketId,msg,sz,0,(struct sockaddr*)&(network->socketAddress),
	       sizeof(struct sockaddr_in));
  if(err<0) printf("Send failed!\n");
  free(msg);
  return err;
}

void* receivePacket(PTYPE *p_t, NetworkState* network) {
  void* packet = NULL;
  
  //set up polling for specific packet type:
  pollfd msgPoll;
  msgPoll.fd = network->socketId;
  msgPoll.events = POLLIN;
  int err = poll(&msgPoll,1,3500);
  if(!err) {
    printf("Poll timeout\n");
    return NULL;
  }
  else if (err == -1) {
    printf("Error with the poll!");
    return NULL;
  }
  else {
    //we got something
    if(msgPoll.revents&POLLIN) {
      char* msg;
      msg = (char*)malloc(1024);
      sockaddr socketAddress;
      socklen_t socket_sz = sizeof(socketAddress);
      err = recvfrom(network->socketId,msg,1024,0,&socketAddress, &socket_sz);
      if(err<0) {
	printf("Error getting packet from socket!");
      }
      packet = deserializePacket(p_t,msg);
    }
  }
  return packet;
}

