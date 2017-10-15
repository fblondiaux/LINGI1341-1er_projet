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
  char* buf = malloc(MAXBUFLEN);
  if (buf == NULL) {
    return -1;
  }
  struct sockaddr_in6* theiraddress = malloc(sizeof( struct sockaddr_in6));
  if(theiraddress == NULL){
    free(buf);
    return -1;
  }
  socklen_t theirlength = 0;
  if(recvfrom(sfd, buf, MAXBUFLEN , 0, (struct sockaddr*) theiraddress, &theirlength) == -1){
    free(buf);
    free(theiraddress);
    return -1;
  }

  if(connect(sfd, (struct sockaddr*) theiraddress,(int) theirlength)== -1){
    free(buf);
    free(theiraddress);
    return -1;
  }
  free(buf);
  free(theiraddress);
  return 0;
}
