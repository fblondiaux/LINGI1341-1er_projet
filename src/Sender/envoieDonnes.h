#include "../FormatSegments/packet_interface.h"

#include <sys/poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h> /* * sockaddr_in6 */
#include <sys/types.h>
#define MAX_WINDOW_SIZE 31

// = read_write_loop_sender

/*
* Structure est la tete d'une liste chainee de "node" representant le buffer d'envoi du sender.
*/
struct head {
  struct node* liste;
};

/*
* Structure pour faire une liste chainee. 
* La liste chainee represente le buffer dans lequel le sender place les packets qu'il a envoyes
* en attendant qu'il reçoive leur accuse de reception.
* Chaque node contient un pointeur vers un packet (pkt_t) et un pointeur vers le node suivant.
*/
struct node {
  pkt_t *pkt;
  struct node* next;
};


/* 
* Rajoute une structure node qui contient un pointeur vers pkt a la fin 
* de la liste chainee dont le pointeur vers la tete de cette liste chainee est buf. 
*
* @pkt : pointeur vers un packet
* @buf : pointeur vers une structure head qui represente la tete de la liste chainee
* @return : 0 succes, -1 erreur
*/
int add(pkt_t *pkt, struct head *buf);

/*
* Supprime  de la liste chainee (dont la tete est buf) tous les nodes qui ont un paquet dont le numero
* de sequence plus petit ou egal au numero de sequence du paquet pkt donne en argument.
*  
* @pkt : pointeur vers un paquet 
* @buf : pointeur vers une structure head qui est la tete d'une la liste chainee de node.
* @return : 0 succes, -1 erreur
*/
int del(pkt_t *pkt, struct head *buf);

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
int checkReceive(const char* buf, const size_t len, struct head *reception);


/*
* prepareToSend convertit le payload qui represente les donnees a envoyer en une structure pkt_t, 
* calcule crc1 ainsi que crc2 si payload != NULL, ajoute le paquet dans le buffer d'envoi du sender 
* convertit ce paquet en un buffer pret a etre envoye au receiver. 
*
* @payload : donnees  a envoyer au sender
* @taillePayload : taille en bytes du payload
* @toSend : buffer dans lequel on stoque les donnees pretes a etre envoyees
* @reception : pointeur vers la tete de la liste chainee de node qui represente le buffer d'envoi de sender
* @return : 0 en cas d'erreur, ou la taille en bytes ecrits dans toSend
*/
int prepareToSend(char* payload, int taillePayload, char* toSend, struct head *buf);


/*
* Specification de la méthode
* @addr =
* @sfd =
* @file =
*/
int envoieDonnes(int sfd, FILE* file);
