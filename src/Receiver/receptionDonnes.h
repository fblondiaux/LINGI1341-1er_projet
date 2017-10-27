#include "../FormatSegments/packet_interface.h"
#include <stdio.h>
#include <netinet/in.h> /* * sockaddr_in6 */
#include <sys/types.h>
/* Taille maximale de Window */
#define MAX_WINDOW_SIZE 31

typedef enum {
  INGNORE = 0,     /* A ete ignore */
  S_ACK,           /* Necessite l'envoye d'un ack*/
  S_NACK,            /* Necessite l'envoye d'un nack */

} selectiveRepeat_status_code;


struct buffer{
  int seqnum;
  pkt_t* data;
  struct buffer* next;
};
/*
* Prends un pointeur vers une structure en argument et
* insère la structure pkt au bon endroit
*
*/

void insertStruct(struct buffer* str);
/*
* TraitementRecu recupere le buffer du socket, le decode, et réagit en conscéquence.
* @buf: char* en provenance du socket
* @taille : taille de buf
* Post ACK contient, si il y en à un, le nack ou ack à renvoyer.
* pré SizeACK = contient la taille de ACK.
* Retourne si le segment est ignoré ou si il faut renvoyer un ack/nack
*/
selectiveRepeat_status_code traitementRecu(char* buf, int taille, char* ACK,
  size_t* SizeACK, const int file);

void receptionDonnes( int sfd, int file);
