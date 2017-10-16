#include <netinet/in.h> /* * sockaddr_in6 */
#include <sys/types.h> /* sockaddr_in6 */
#include <sys/socket.h> // pour la fonction socket()
#include <errno.h>
#include <stdio.h>
#include <string.h> // pour strerror()

/* Creates a socket and initialize it
 * @source_addr: if !NULL, the source address that should be bound to this socket
 * @src_port: if >0, the port on which the socket is listening
 * @dest_addr: if !NULL, the destination address to which the socket should send data
 * @dst_port: if >0, the destination port to which the socket should be connected
 * @return: a file descriptor number representing the socket,
 *         or -1 in case of error (explanation will be printed on stderr)
 */
int create_socket(struct sockaddr_in6 *source_addr, int src_port, 
  struct sockaddr_in6 *dest_addr, int dst_port) {


  int err = 0;
  int sock = socket(AF_INET6, SOCK_DGRAM, 0); /* j'ai mis protocol = 0 car 
  normalement il n'existe qu'un protocol par type de socket et maille de protocole donnés. 
  (source :  http://man7.org/linux/man-pages/man2/socket.2.html) ... 
  mais c'est sans doute pas obligatoire */

  if (sock == -1)
  {
    fprintf(stderr, "%s\n", strerror(errno)); // peut-être plus précis d'utiliser errno ? (comme il est quand même mis à jour par socket())
    return -1;
  }



  /* question : est-ce possible qu'il faille spécifier 
  adresse et port pour envoyer données (source) ET 
  adresse et port sur lequel envoyer données (destination)? */
  

  if(source_addr != NULL)
  {
    source_addr->sin6_port = htons(src_port); // question (peut-être un peu bête): comment on sait qu'il faut convertir de host à networck byte order?
    err = bind(sock, (const struct sockaddr *) source_addr, sizeof(struct sockaddr));
    if(err !=0)
    {
      fprintf(stderr, "%s\n", strerror(errno));
      return -1;
    }
  }
  if(dest_addr != NULL)
  {
    dest_addr->sin6_port = htons(dst_port);
    err = connect(sock, (const struct sockaddr *) dest_addr, sizeof(struct sockaddr));
    if (err!= 0)
    {
      fprintf(stderr, "%s\n", strerror(errno));
      return -1;
    }
  }
  return sock;

}
