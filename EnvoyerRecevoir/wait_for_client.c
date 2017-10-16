#include "wait_for_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/* Block the caller until a message is received on sfd,
* and connect the socket to the source addresse of the received message.
* @sfd: a file descriptor to a bound socket but not yet connected
* @return: 0 in case of success, -1 otherwise
* @POST: This call is idempotent, it does not 'consume' the data of the message,
* and could be repeated several times blocking only at the first call.
*/

#define MAXBUFLEN 1024
int wait_for_client(int sfd){
  char buf[1024];
  struct sockaddr_in6 theiraddress;
  socklen_t theirlength = sizeof(struct sockaddr_in6);
  if(recvfrom(sfd, buf, MAXBUFLEN ,MSG_PEEK, (struct sockaddr*) &theiraddress, &theirlength) == -1){

    printf("Ici? WFC\n");
    return -1;
  }

  if(connect(sfd, (struct sockaddr*)& theiraddress,(int) theirlength)== -1){
    return -1;
  }
  return 0;
}
