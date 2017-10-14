#include <netinet/in.h> /* * sockaddr_in6 */
#include <sys/types.h> /* sockaddr_in6 */
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

// Ce code est inspire de la resource https://beej.us/guide/bgnet/output/html/singlepage/bgnet.html

/* Resolve the resource name to an usable IPv6 address
* @address: The name to resolve
* @rval: Where the resulting IPv6 address descriptor should be stored
* @return: NULL if it succeeded, or a pointer towards
*          a string describing the error if any.
*          (const char* means the caller cannot modify or free the return value,
*           so do not use malloc!)
*/
const char * real_address(const char *address, struct sockaddr_in6 *rval){

  int status;
  struct addrinfo hints;
  struct addrinfo *servinfo;  // will point to the results

  memset(&hints, 0, sizeof hints); // make sure the struct is empty
  hints.ai_family = AF_INET6;
  hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
  hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

  if ((status = getaddrinfo(address, NULL , &hints, &servinfo)) != 0) {
    return gai_strerror(status);
  }

  rval = (struct sockaddr_in6 *) servinfo->ai_addr;
    fprintf(stderr,"realvalue %d", rval->sin6_family);  // Present juste pour que ça compile, il faut trouver une meilleure solution

  // freeaddrinfo(servinfo); // JE NE SAIS PAS SI ÇA DOIT ETRE LA OU PAS
  // SI ON FREE SERVINFO ALORS ON FREE AUSSI RVAL ? Faut-il utiliser meme copy ?
  return NULL;
}
