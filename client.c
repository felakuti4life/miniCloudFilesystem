/****************/
/* Ethan Geller	*/
/* 5/21/16	*/
/* CS 244B	*/
/* Spring 2016	*/
/****************/

#define DEBUG

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <client.h>
#ifndef CLIENTSTATE
#define CLIENTSTATE 1
typedef struct _clientState {
  unsigned int currentTransactionId;
  unsigned int currentRevisionCount;
  unsigned int nBlocks;
} ClientState;

ClientState* cState;
NetworkState* clientNetwork;
#endif


int
InitReplFs( unsigned short portNum, int packetLoss, int numServers ) {
#ifdef DEBUG
  printf( "InitReplFs: Port number %d, packet loss %d percent, %d servers\n", 
	  portNum, packetLoss, numServers );
#endif
  clientNetwork =(NetworkState*) malloc(sizeof(NetworkState));
  cState = (ClientState*) malloc(sizeof(ClientState));
  clientNetwork->my_ntype = NTYPE_CLIENT;
  clientNetwork->droprate = ((float) packetLoss)/100.0f;
  int err = startNetwork(GROUP,PORT,NTYPE_CLIENT,clientNetwork);
  return( err );  
}


int OpenFile( char * fileName ) {
  int fd;
  ASSERT( fileName );
#ifdef DEBUG
  printf( "OpenFile: Opening File '%s'\n", fileName );
#endif
  OpenFilePacket out;
  out.ntype = NTYPE_CLIENT;
  out.transactionid = rand();
  cState->currentTransactionId = out.transactionid;
  cState->nBlocks = 128;
#ifdef DEBUG
  if ( fd < 0 )
    perror( "OpenFile" );
#endif
  sendPacket(PTYPE_OPENFILE,(void*)&out,clientNetwork);å
  //wait a couple of attempts for a success packet
  unsigned int numAttempts = ATTEMPT_COUNT;
  PTYPE p_t;
  void* packet;
  while(numAttempts) {
    packet = receivePacket(&p_t,clientNetwork);
    if(p_t==PTYPE_OPENSUCCESS) {
      OpenSuccessPacket* p = (OpenSuccessPacket*) packet;
      if(p->transactionid == cState->currentTransactionId) {
	if(p->success)fd = p->fd;
	else fd=-1;
	cState->currentRevisionCount = 0;
	break;
      }
    }
    numAttempts--;
  }
  free(packet);
  return( fd );
}


int WriteBlock( int fd, char * buffer, int byteOffset, int blockSize ) {
  int bytesWritten;
  ASSERT( fd >= 0 );
  ASSERT( byteOffset >= 0 );
  ASSERT( buffer );
  ASSERT( blockSize >= 0 && blockSize < MaxBlockLength );
#ifdef DEBUG
  printf( "WriteBlock: Writing FD=%d, Offset=%d, Length=%d\n",
	fd, byteOffset, blockSize );
#endif
  cState->currentRevisionCount++;
  WriteFilePacket out;
  out.data = buffer;
  out.sz = blockSize;
  out.offset = byteOffset;
  out.counter = cState->currentRevisionCount;
  out.blockid = byteOffset/cState->nBlocks;
  sendPacket(PTYPE_WRITEFILE,(void*)&out, clientNetwork);
  return( bytesWritten );

}

/* ------------------------------------------------------------------ */

int Commit( int fd ) {
  ASSERT( fd >= 0 );

#ifdef DEBUG
  printf( "Commit: FD=%d\n", fd );
#endif
  int err =0;
  CommitPacket out;
  out.ntype = NTYPE_CLIENT;
  out.transactionid = cState->currentTransactionId;
  err = sendPacket(PTYPE_COMMIT,(void*)&out,clientNetwork);

  //wait a couple of attempts for a success packet
  unsigned int numAttempts = ATTEMPT_COUNT;
  PTYPE p_t;
  void* packet;
  while(numAttempts) {
    packet = receivePacket(&p_t,clientNetwork);
    if(p_t==PTYPE_COMMITSUCCESS) {
      CommitSuccessPacket* p = (CommitSuccessPacket*) packet;
      if(p->transactionid == cState->currentTransactionId) {
	if(p->success){
	  cState->currentRevisionCount = 0;
	}
	else err=-1;
	
	break;
      }
    }
    numAttempts--;
  }
  free(packet);

  return( err );

}

int Abort( int fd )
{
  ASSERT( fd >= 0 );
#ifdef DEBUG
  printf( "Abort: FD=%d\n", fd );
#endif
  int err = 0;
  AbortPacket out;
  out.ntype = NTYPE_CLIENT;
  out.transactionid = cState->currentTransactionId;
  
  err = sendPacket(PTYPE_ABORT,(void*)&out, clientNetwork);
  return(err);
}

int CloseFile( int fd ) {
  ASSERT( fd >= 0 );
#ifdef DEBUG
  printf( "Close: FD=%d\n", fd );
#endif
  int err = 0;
  ClosePacket out;
  out.ntype = NTYPE_CLIENT;
  out.transactionid = cState->currentTransactionId;
  out.fd = fd;
  err = sendPacket(PTYPE_CLOSE,(void*)&out,clientNetwork);
  //wait a couple of attempts for a success packet
  unsigned int numAttempts = ATTEMPT_COUNT;
  PTYPE p_t;
  void* packet;
  while(numAttempts) {
    packet = receivePacket(&p_t,clientNetwork);
    if(p_t==PTYPE_CLOSESUCCESS) {
      CloseSuccessPacket* p = (CloseSuccessPacket*) packet;
      if(p->transactionid == cState->currentTransactionId) {
	if(p->success){
	  cState->currentRevisionCount = 0;
	  cState->currentTransactionId = 0;
	}
	else err=-1;
	break;
      }
    }
    numAttempts--;
  }
  free(packet);

  return(err);
}




