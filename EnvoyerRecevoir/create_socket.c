#include <netinet/in.h> /* * sockaddr_in6 */
#include <sys/types.h> /* sockaddr_in6 */
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include "create_socket.h"
/* Creates a socket and initialize it
* @source_addr: if !NULL, the source address that should be bound to this socket
* @src_port: if >0, the port on which the socket is listening
* @dest_addr: if !NULL, the destination address to which the socket should send data
* @dst_port: if >0, the destination port to which the socket should be connected
* @return: a file descriptor number representing the socket,
*         or -1 in case of error (explanation will be printed on stderr)
*/

int create_socket(struct sockaddr_in6 *source_addr,int src_port,struct sockaddr_in6 *dest_addr,int dst_port){

  int soc = socket(AF_INET6,SOCK_DGRAM, IPPROTO_UDP);
  if ( soc == -1){
    fprintf(stderr, "Erreur lors de l'utilisation de socket\n");
    return -1;
  }
  if(source_addr != NULL){
    source_addr->sin6_port = htons(src_port);
    int err = bind(soc,(struct sockaddr *) source_addr, sizeof(struct sockaddr_in6));
    if(err == -1){
      fprintf(stderr, "Erreur lors de l'utilisation de bind\n");
      return -1;
    }
  }
  if(dest_addr != NULL){
    dest_addr->sin6_port = htons(dst_port);
    if(connect(soc,(struct sockaddr *)dest_addr, sizeof(struct sockaddr_in6)) == -1){
      fprintf(stderr, "Erreur lors de l'utilisation de connect\n");
      return -1;
    }
  }


  return soc;
}
