#include "packet_interface.h"

#include <sys/poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

// = read_write_loop_sender

typedef enum {
  INGNORE = 0,     /* A ete ignore */
  S_ACK,           /* Necessite l'envoye d'un ack*/
  S_NACK,            /* Necessite l'envoye d'un nack */
  
} selectiveRepeat_status_code;

/*
* Specification de la méthode
* @addr =
* @sfd =
* @file =
*/
int envoieDonnes(struct sockaddr_in6 *addr, int sfd, int file);


/*
* Decode le buffer et voit ce qu'il doit en faire
* Sender reçoit normalement des acquitments (N : ou non acquittement?)
* checkReceive va decoder la structure pour voir ce qui a ete reçu.
* Si TR= 1 doit envoyer un NACK
* Si c'est un acquitment; il decale sa window,
* Si c'est un data, il est ignore.
* SI c'est un Nack, le paquet a été tronqué donc on le renvoie. SI il ne rentre pas
* Dans la fenêtre des numeros de sequences, on doit ignorer le packet.
*
* @buf : donnees recues a gerer
* @len : taille en bytes des donnees recues
* @liste : pointeur vers une liste chainee qui constitue le buffer de reception du sender
*
* @return : 
*
*/
int checkReceive(const char* buf, const size_t len, struct node *liste);


/*
* @payload: données reçues à envoyer.
* @tosend: buffer qui contiendra le packet à envoyer.
* 
* @return : taille de toSend
*/
int prepareToSend(char* payload, int taillePayload, char* toSend, struct node *liste);
