#include <netinet/in.h> /* * sockaddr_in6 */
#include <sys/types.h> /* sockaddr_in6 */
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
/* Cre
ates a socket and initialize it
* @source_addr: if !NULL, the source address that should be bound to this socket
* @src_port: if >0, the port on which the socket is listening
* @dest_addr: if !NULL, the destination address to which the socket should send data
* @dst_port: if >0, the destination port to which the socket should be connected
* @return: a file descriptor number representing the socket,
*         or -1 in case of error (explanation will be printed on stderr)
*/
int create_socket(struct sockaddr_in6 *source_addr,int src_port,
  struct sockaddr_in6 *dest_addr,int dst_port){
    // si dst port ou src port < 0  ?
    // Lie le port a l'adresse
    source_addr->sin6_port = src_port;
    dest_addr->sin6_port = dst_port;

    // Creation des sockets
    struct protoent * temp = getprotobyname("udp");

    int soc = socket(PF_INET6,SOCK_STREAM, temp->p_proto);
    if ( soc == -1){
      fprintf(stderr, "Erreur lors de l'utilisation de socket\n");
      return -1;
    }

    // COnnection

    if(source_addr != NULL){
      if(bind(soc,(struct sockaddr *)source_addr, sizeof(struct sockaddr_in6)) == -1){
        fprintf(stderr, "Erreur lors de l'utilisation de bind\n");
        return -1;
      }
    }
    if(dest_addr != NULL){
      if(connect(soc,(struct sockaddr *)dest_addr, sizeof(struct sockaddr_in6)) == -1){
        fprintf(stderr, "Erreur lors de l'utilisation de connect\n");
        return -1;
      }
    }

    return soc;
    //Faut-il faire qqch si le source addr ou dest adress == NULL, on peux simplement renvoyer une erreur ?
    // SI c'est null on cree socket mais on ne renvoye pas les socket ?
    //On renvoie le fd de qui ? C'est pas juste on ne peux en renvoyer qu'un

  }
