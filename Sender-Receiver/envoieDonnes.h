#include "packet_interface.h"

#include <sys/poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
/*
* Specification de la méthode
* @addr =
* @sfd =
* @file =
*/
int envoieDonnes(sockaddr_in6 addr, int sfd, int file);


/*
* Decode le buffer et voit ce qu'il doit en faire
* Sender reçoit normalement des acquitments
* checkReceive va decoder la structure pour voir ce qui a ete reçu.
* Si TR= 1 doit envoyer un NACK
* Si c'est un acquitment; il decale sa window,
* Si c'est un data, il est ignore.
* SI c'est un Nack, le paquet a été tronqué donc on le renvoie. SI il ne rentre pas
* Dans la fenêtre des numeros de sequences, on doit ignorer le packet.
*
* retourne
*/
int checkReceive(char* buf);


/*
* @payload donnée reçue à envoyer.
* @tosend buffer qui contiendra le packet à envoyer.
* 
*/
int prepareToSend(char* payload, int taillePayload, char* toSend);
