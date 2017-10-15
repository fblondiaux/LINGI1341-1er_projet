#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h> // pour memset et memcpy

#include <netinet/in.h> // définit strut sockaddr_in qui peut être casté en sockaddr 

// systeme de chat utilisant des packets UDP

/* contrairement au protocole du projet, ce systeme de chat de donne aucune garantie 
			concernant la bonne réception des messages envoyés*/



/* Resolve the resource name to an usable IPv6 address
 * @address: The name to resolve
 * @rval: Where the resulting IPv6 address descriptor should be stored
 * @return: NULL if it succeeded, or a pointer towards
 *          a string describing the error if any.
 *          (const char* means the caller cannot modify or free the return value,
 *           so do not use malloc!)
 */
const char * real_address(const char *address, struct sockaddr_in6 *rval)
{
	struct addrinfo hints; // spécifie critères pour sélectionner les sockets address structures retournées dans
							// la liste pointée par res.
	struct addrinfo *result, *rp;
	int status;


	memset(&hints, 0, sizeof(struct addrinfo)); // copie caractère 0 sur les sizeof(struct addrinfo) 
												// premiers caractères de la chaine pointée par &hints
												// cad qu'on met tous les champs de la struct addrinfo à 0. 

	hints.ai_family = AF_INET6; // les adresses retournées doivent être de la famille IPv6
	hints.ai_socktype = SOCK_DGRAM;	/* On choisit le type d'adresse de socket. 
										 	les datagram sockets utilisent le "User datagram protocol (UDP)" 
											et non le TCP  --> noté dans consigne Inginious qu'il faut UDP*/
	hints.ai_protocol = IPPROTO_UDP; /* je crois qu'il ne faut pas le mettre car il existe normalement un seul type de protocol 
	pour un type de socket et une famille de protocole donnés. On peut alors mettre zéro 
	(source: http://man7.org/linux/man-pages/man2/socket.2.html ) */


	// hints.ai_flags = AI_PASSIVE; // à enlever? 

	status = getaddrinfo(address, NULL, &hints, &result); // place dans result une ou plusieurs addrinfo structures (liste chainée)
	if (status != 0)
		return gai_strerror(status); // gai_strerror prend en argument un code d'erreur et le transforme en string

	struct sockaddr_in6 *inter;
	memcpy((void *)inter, (void *) result->ai_addr, sizeof(struct sockaddr_in6));
	rval = (struct sockaddr_in6 *) inter;
	freeaddrinfo(result);
	return NULL;


	// question : comment ça se fait que result sera un pointeur vers UNE struct addrinfo et pas une liste chainéee de struct addrinfo??

}
