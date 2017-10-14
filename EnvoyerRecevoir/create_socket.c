#include <netinet/in.h> /* * sockaddr_in6 */
#include <sys/types.h> /* sockaddr_in6 */

/* Creates a socket and initialize it
* @source_addr: if !NULL, the source address that should be bound to this socket
* @src_port: if >0, the port on which the socket is listening
* @dest_addr: if !NULL, the destination address to which the socket should send data
* @dst_port: if >0, the destination port to which the socket should be connected
* @return: a file descriptor number representing the socket,
*         or -1 in case of error (explanation will be printed on stderr)
*/
int create_socket(struct sockaddr_in6 *source_addr,int src_port,
  struct sockaddr_in6 *dest_addr,int dst_port){
    int source = 0, destination= 0 ;
    int source = socket(PF_INET6,SOCK_STREAM,getprotobyname("udp"));
    if (source == -1){
      fprintf(stderr, "Erreur lors de l'utilisation de socket\n");
      return -1;
    }
    int destination = socket(PF_INET6,SOCK_STREAM,getprotobyname("udp"));
    if (destination== -1){
      fprintf(stderr, "Erreur lors de l'utilisation de socket\n");
      return -1;
    }

    //Faut-il faire qqch si le source addr ou dest adress == NULL, on peux simplement renvoyer une erreur ?


  }
