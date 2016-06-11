#include "serve.h"

char* makeFullPath(ServedFileState* s) {
  char* str =(char*) malloc(s->server->mountpath_length+s->fpath_length+1);
  memcpy(str,s->server->mountpath,s->server->mountpath_length);
  str[s->server->mountpath_length]='/';
  memcpy(str+s->server->mountpath_length+1,s->fpath,s->fpath_length);
  return str;
}

int commitQueue(ServedFileState* s){
  WriteQueue* q = s->queue;
  char* fpath = makeFullPath(s);
  FILE* f = fopen(fpath, "r+");
  if(f==NULL) {
    f = fopen(fpath,"wb");
    if(f==NULL) {
      printf("failed to create file.");
      return -1;
    }
  }
  while(q->next!=NULL) {
    fseek(f,q->packet->offset,SEEK_SET);
    fwrite(q->packet->data,1, q->packet->sz,f);
    q = q->next;
  }
  fclose(f);
  cleanUpWriteQueue(s->queue);
  free(fpath);
  return 0;
}

int handleOpenFile(void* packet, ServedFileState* s) {
  OpenFilePacket* p = (OpenFilePacket*) packet;
  OpenSuccessPacket out;
  out.transactionid = p->transactionid;
  
  if(s->open) {
    out.success = 0;
  }
  else {
    s->open = 1;
    memcpy(s->fpath, p->fpath, p->fpath_length);
    s->transactionid = p->transactionid;
    s->fd = rand();
    out.fd = s->fd;
    out.success = 1;
  }
  free(p->fpath);
  free(packet);
  return sendPacket(PTYPE_OPENSUCCESS,(void*)&out,s->server->network);
}

//returns 0 if new edit, 1 if overwriting a previous edit
int handleWriteFile(void* packet, ServedFileState* s) {
  WriteFilePacket* p = (WriteFilePacket*) packet;
  int exists = 0;
  WriteQueue* q = s->queue;
  if(s->open) {
    while(q->next) {
      if(q->packet->blockid == p->blockid){
	exists = 1;
	if(p->counter > q->packet->counter) {
	  free(q->packet);
	  q->packet = p;
	}
	break;
      }
      q = q->next;
    }
    if(!exists){
      q->next = (WriteQueue*) malloc(sizeof(WriteQueue));
      q->next->next = NULL;
      q->next->packet = p;
    }
  }
  return exists;
}

int handleCommit(void* packet, ServedFileState* s) {
  CommitPacket* p = (CommitPacket*) packet;
  CommitSuccessPacket out;
  out.transactionid = p->transactionid;
  
  out.fd = s->fd;
  int err = -1;
  if(s->open) err = commitQueue(s);
  if(!err) out.success = 1;
  else out.success = 0;
  err = sendPacket(PTYPE_COMMITSUCCESS,(void*)&out,s->server->network);
  return err;
}

int handleAbort(void* packet, ServedFileState* s) {
  AbortPacket* p = (AbortPacket*) packet;
  if(p->transactionid==s->transactionid) cleanUpWriteQueue(s->queue);
  return 0;
}

int handleClose(void* packet, ServedFileState* s) {
  ClosePacket* p = (ClosePacket*) packet;
  CloseSuccessPacket out;
  out.transactionid = p->transactionid;
  out.fd = p->fd;
  if(s->open){
    commitQueue(s);
    s->open = 0;
    out.success = 1;
  }
  else {
    out.success = 0;
  }
  return sendPacket(PTYPE_CLOSESUCCESS,(void*)&out,s->server->network);
}



int cleanUpServerState(ServerState* s){
  free(s->mountpath);
  free(s);
  return 0;
}

int cleanUpServedFileState(ServedFileState* s) {
  cleanUpWriteQueue(s->queue);
  free(s->fpath);
  free(s);
  return 0;
}

int cleanUpWriteQueue(WriteQueue* q){
  while(q->next!=NULL) cleanUpWriteQueue(q->next);
  free(q->packet);
  free(q);
  return 0;
}

NetworkState* connectServer(int groupId, unsigned short port, float droprate) {
  NetworkState* n = (NetworkState*) malloc(sizeof(NetworkState));
  n->droprate = droprate;
  int err = startNetwork(groupId,port,NTYPE_SERVER,n);
  if(!err) n->status = 1;
  else printf("Error: Server failed to connect to network.");
  return n;
}

int mainLoop(ServedFileState* f, unsigned short port,float droprate) {
  NetworkState* network = connectServer(GROUP,port,droprate);
  f->server->network = network;
  PTYPE p_t;
  void* packet;
  while(1) {
    packet = receivePacket(&p_t,network);
    if(p_t == PTYPE_OPENFILE) handleOpenFile(packet, f);
    else if(p_t == PTYPE_WRITEFILE) handleWriteFile(packet,f);
    else if(p_t == PTYPE_COMMIT) handleCommit(packet,f);
    else if(p_t == PTYPE_ABORT) handleAbort(packet,f);
    else if(p_t == PTYPE_CLOSE) handleClose(packet,f);
  }
  
  return 0;
}
