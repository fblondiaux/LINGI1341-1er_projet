#include "receptionDonnes.h"
#include <sys/poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

/*
* Code réalisé par :
* Noemie verstraete - 25021500
* Florence Blondiaux - 06521500
* Version du 05.11.17
*/

int seqnumMax = MAX_WINDOW_SIZE;
int seqnumMin = 0;
int window = MAX_WINDOW_SIZE;
uint32_t lasttimestamp= 0; //Necessaire pour l'envoie des Acks
int end = 0;

//Buffer pour stocker les données
struct buffer* startBuffer = NULL;

/*
*@pre:
*
*@post: Imprime le contenu de la structure.
*/
void printStruct(){
  struct buffer* parcours = startBuffer;
  while(parcours != NULL){
    fprintf(stderr, "%d->", parcours->seqnum );
    parcours = parcours->next;
  }
  fprintf(stderr, "fin\n");
}

/*
*@pre: Prends seqnum en argument.
*
*@post: retourne 0 si l'element n'est pas dans la structure, 1 si il y est déjà.
*/
int alreadyInStruct(int seqnum){
  struct buffer* parcours = startBuffer;
  while(parcours != NULL){
    if (parcours->seqnum == seqnum){
      return 1;
    }
    parcours = parcours->next;
  }
  return 0;
}

/*
*@pre str != NULL;
*
*@post l'element est inseré au bon endroit dans la structure. Si il y était déjà,
* on ne change rien.
*/

void insertStruct(struct buffer* str){

  struct buffer* parcours = startBuffer;
  int seqnum = pkt_get_seqnum(str->data);

  // si packet déjà présent
  if(alreadyInStruct(seqnum) == 1){
    return;
  }
  window--;

    /*DEBUG*/ fprintf(stderr,"Receiver stocke le seqnum %d dans son buf\n",seqnum);

  // si rien dans le buffer
  if (parcours == NULL) {
    startBuffer = str;
    startBuffer->next = NULL;
    return ;
  }


  if (seqnumMin < seqnumMax){
    //Element a inserer avant ce qui y etait deja.
    if(seqnum < parcours->seqnum){
      struct buffer *temp = parcours;
      startBuffer = str;
      str->next = temp;
      return;
    }

    while(parcours->next != NULL && parcours->next->seqnum < seqnum){
      parcours=parcours->next;
    }
    str->next = parcours->next;
    parcours->next = str;
    return;
  }
  else{
    if(seqnum < 50){ // Seqnum est dans le début des numéros de sequences.
      while(parcours->next != NULL && parcours->next->seqnum > 50){ // On passe tous les seqnums de 2XX pour arriver avec next
        parcours=parcours->next;
      }
      //ON est a la fin
      if(parcours->next == NULL){
        parcours->next = str;
        str->next = NULL;
        //fprintf(stderr, "->%d\n",seqnum );
        return;
      }
      //il reste des numeros de sequences et ils sont entre 0 et 50.
      else{
        while(parcours->next != NULL && parcours->next->seqnum < seqnum){
          parcours=parcours->next;
        }
        str->next = parcours->next;
        parcours->next = str;
        return;
      }
    }

    else{ // seqnum > 50
      // L'element a inserer est plus petit que le reste de la structure.
      if((seqnum < parcours->seqnum && parcours->seqnum > 50 ) || parcours->seqnum < 50){
        str->next = parcours;
        startBuffer = str;
        return;
      }
      while( parcours->next!= NULL && parcours->next->seqnum > 50 &&  seqnum > parcours->next->seqnum)
      {
        parcours = parcours->next;
      }
      str->next = parcours->next;
      parcours->next = str;
      return;
    }

  }
}

/*
* @pre= buf : buffer recu dans le socket, taille: taille de buf, ACK le ack ou le nack à renvoyer
* SizeACK: la taille de ce qu'il faut renvoyer, file: le file descriptor sur lequel on ecrit ce qui a été lu.
*
* @post retourne INGNORE si pas de ack ou de nack à envoyer, sinon retourne S_NACK OU S_ACK.
*/
selectiveRepeat_status_code traitementRecu(char* buf, int taille, char* ACK, size_t* SizeACK, const int file){
  pkt_t* reception = pkt_new();
  pkt_status_code err = pkt_decode(buf, taille, reception);

  //C'est le signal de fin, on quitte le programme.
  if(pkt_get_length(reception) == 0 && err != E_TR){
    /*DEBUG*/fprintf(stderr, "Receiver envoie le ack de fin\n");
    pkt_t* ack = pkt_new();
    pkt_set_type(ack, PTYPE_ACK);
    pkt_set_window(ack, window);
    pkt_set_seqnum(ack,seqnumMin);
    pkt_set_length(ack,0);
    pkt_set_timestamp(ack, pkt_get_timestamp(reception));
    pkt_del(reception);
    if(pkt_encode(ack,ACK, (size_t*) SizeACK) != PKT_OK){
      pkt_del(ack);
      return INGNORE;
    }
    end = 1;
    return S_ACK;
  }
  //Reception d'une donnée tronquée, on envoie un nack.
  if (err == E_TR && pkt_get_type(reception) == 1){
    // On va devoir envoyer un NACK
    /*DEBUG*/fprintf(stderr, "Receiver doit renvoyer un nack\n"); // Preparation du NACK à envoyer.
    pkt_t* ack = pkt_new(); // Préparation de la structure à encoder
    pkt_set_type(ack, 3);
    pkt_set_window(ack, window);
    pkt_set_seqnum(ack,pkt_get_seqnum(reception));
    pkt_set_length(ack,0);
    pkt_set_timestamp(ack,pkt_get_timestamp(reception));

    if(pkt_encode(ack,ACK, (size_t*) SizeACK) != PKT_OK){
      fprintf(stderr, "Erreur lors de l'encodage du NACK, le packet est ignoré\n");
      pkt_del(reception);
      pkt_del(ack);
      return INGNORE;
    }
    pkt_del(reception);
    pkt_del(ack);
    return S_NACK;
  }
  else if(err == PKT_OK){
    // On doit envoyer un ACK et écrire ce qu'il y avait dans le payload.
    int seqnum = pkt_get_seqnum(reception);
    if(seqnumMin < seqnumMax){
      if (seqnum < seqnumMin || seqnum > seqnumMax){ // En dehors de ce que l'on peux recevoir.
        lasttimestamp = pkt_get_timestamp(reception);

        /*DEBUG*/ fprintf(stderr, "Receiver a déja reçu le sequnum %d, il l'ignore mais envoie quand meme un ack\n", seqnum );
        //Preparation de l'ack à envoyer
        pkt_t* ack = pkt_new();
        pkt_set_type(ack, 2);
        pkt_set_window(ack, window);
        pkt_set_seqnum(ack,seqnumMin);
        pkt_set_length(ack,0);
        pkt_set_timestamp(ack, pkt_get_timestamp(reception));
        pkt_del(reception);
        if(pkt_encode(ack,ACK, (size_t*) SizeACK) != PKT_OK){
          pkt_del(ack);
          return INGNORE;
        }
        return S_ACK;
      }
    }
    if(seqnumMax < seqnumMin){
      if (seqnum > seqnumMax && seqnum < seqnumMin){ // En dehors de ce que l'on peux recevoir.
        //ON prepare egalement en ack.
        lasttimestamp = pkt_get_timestamp(reception);
        pkt_del(reception);
        fprintf(stderr, "Receiver a déja reçu le sequnum %d, il l'ignore mais envoie quand meme un ack\n", seqnum );
        pkt_t* ack = pkt_new();
        pkt_set_type(ack, 2);
        pkt_set_window(ack, window);
        pkt_set_seqnum(ack,seqnumMin);
        pkt_set_length(ack,0);
        pkt_set_timestamp(ack,lasttimestamp);
        if(pkt_encode(ack,ACK, (size_t*) SizeACK) != PKT_OK){
          pkt_del(ack);
          return INGNORE;
        }
        return S_ACK;
      }
    }

    // Creation et placement de la structure dans le buffer.
    struct buffer* new = malloc(sizeof(struct buffer));
    new->seqnum = seqnum;
    new->data = reception;
    /*DEBUG*/ fprintf(stderr, "Resultat apres insertion\n");
    insertStruct(new);
    /*DEBUG*/ printStruct();
    /*DEBUG*/fprintf(stderr, "window avant while : %d\n", window);
    struct buffer* current = startBuffer;
    char payloadTemp[512];
    int size = 0;
    while(startBuffer != NULL && startBuffer->seqnum == seqnumMin){
      // Decalage de la fenêtre
      seqnumMin = (seqnumMin +1) % 256;
      seqnumMax = (seqnumMax +1) % 256;
      lasttimestamp = pkt_get_timestamp(startBuffer->data); // ON en a besoin pour la suite
      //Ecriture du contenu du payload dans le fichier ou sur la sortie standard
      size = pkt_get_length(current->data);
      memcpy( payloadTemp,pkt_get_payload(current->data), size);
      int ecrit = write(file, payloadTemp, size);
      if(ecrit!=size){
        fprintf(stderr, "Le segment à été reçu mais un problème à été rencontré lors de l'écriture de son payload\n");
      }
      startBuffer = startBuffer->next;
      pkt_del(current->data);
      free(current);
      window ++; // ON a libere une place dans le buffer
      /*DEBUG*/fprintf(stderr, "window = %d\n", window);
      current = startBuffer;
    }

    //Preparation du ack a renvoyer.
    pkt_t* ack = pkt_new();
    pkt_set_type(ack, 2);
    pkt_set_window(ack, window);
    pkt_set_seqnum(ack,seqnumMin);
    pkt_set_length(ack,0);
    pkt_set_timestamp(ack,lasttimestamp);
    if(pkt_encode(ack,ACK, (size_t*) SizeACK) != PKT_OK){
      pkt_del(ack);
      return INGNORE;
    }
    pkt_del(ack);
    return S_ACK;

  }
  else{
    //Ce qu'on a recu n'est pas coherent, on renvoie un ack du prochain seqnum attendu
    pkt_del(reception);
    pkt_t* ack = pkt_new();
    pkt_set_type(ack, 2);
    pkt_set_window(ack, window);
    pkt_set_seqnum(ack,seqnumMin);
    pkt_set_length(ack,0);
    pkt_set_timestamp(ack,lasttimestamp);
    if(pkt_encode(ack,ACK, (size_t*) SizeACK) != PKT_OK){
      pkt_del(ack);
      return INGNORE;
    }

    return S_ACK;
  }
}

/*
* @pre sfd, file pour orienter la lecture et l'ecriture des donnees au bon endroit.
*
* @post Toutes les donnees ont ete recues et un ack de fin a ete envoye.
*
*/
void receptionDonnes(int sfd, int file){
  // Preparation

  char buf[528];
  char payload[512];
  size_t payloadSize = 512;
  selectiveRepeat_status_code err= 0;


  struct pollfd ufds[2];
  ufds[0].fd = sfd;
  ufds[0].events = POLLIN; // check for normal data
  ufds[1].fd = sfd;
  ufds[1].events = POLLOUT;
  while(end == 0 ){

    int rv = poll(ufds, 2, -1);
    if (rv == -1) {
      /*DEBUG*/ fprintf(stderr, "Error lors de l'utilisation de poll\n");
      return ;
    }
    else {
      memset((void*)buf, 0, 528); // make sure the struct is empty
      err = INGNORE;
      // traite la lecture
      if (ufds[0].revents & POLLIN) {
        memset((void*)payload, 0, 512); // make sure the struct is empty
        payloadSize = 512;
        int recu = read(sfd, buf, sizeof buf); // receive normal data
        /*DEBUG*/if(recu == 12){
        /* DEBUG*/   fprintf(stderr, "C'EST LA FIN \n");
      /*debug*/ }
        err = traitementRecu(buf, recu, payload, &payloadSize, file);
      }
      if(err != INGNORE && ufds[1].revents & POLLOUT){
        int sended = write(sfd,payload,payloadSize);
        if(sended != payloadSize){
          fprintf(stderr, "Erreur lors de l'envoi de l'ACK, on continue\n");
        }
      }
    }

  }
  /*DEBUG*/ fprintf(stderr, "Moi j'ai fini bye, receiver\n" );
}
