#include "receptionDonnes.h"
#include <sys/poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>


int seqnumMax = MAX_WINDOW_SIZE;
int seqnumMin = 0;
int window = MAX_WINDOW_SIZE;
uint32_t lasttimestamp= 0;

struct buffer* startBuffer = NULL;

void insertStruct(struct buffer* str){
  struct buffer* parcours = startBuffer;
  int seqnum = pkt_get_seqnum(str->data);
  fprintf(stderr,"Receiver stocke le seqnum %d dans son buf\n",seqnum);
  if (parcours == NULL) {
    startBuffer = str;
    startBuffer->next = NULL;
    return ;
  }
  if(parcours->next == NULL || parcours->seqnum > seqnum){
    if(parcours->seqnum > seqnum){
      str->next = parcours;
      startBuffer = str;

      return;
    }
    else{
      parcours->next = str;
      str->next = NULL;
      return;
    }
  }
  else{
    while(parcours->next != NULL || parcours->next->seqnum < seqnum ){
      parcours = parcours->next;
    }
    str->next = parcours->next;
    parcours->next = str;
    return;
  }
}


selectiveRepeat_status_code traitementRecu(char* buf, int taille, char* ACK, size_t* SizeACK, const int file){
  pkt_t* reception = pkt_new();
  pkt_status_code err = pkt_decode(buf, taille, reception);

  if (err == E_TR && pkt_get_type(reception) == 1){
    // On va devoir envoyer un NACK
    fprintf(stderr, "Receiver doit renvoyer un nack\n");
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
    // N : comment le nack est-il envoyé?
  }
  else if(err == PKT_OK){
    // On doit envoyer un ACK et écrire ce qu'il y avait dans le payload.
    int seqnum = pkt_get_seqnum(reception);
    if(seqnumMin < seqnumMax){
      if (seqnum < seqnumMin || seqnum > seqnumMax){ // En dehors de ce que l'on peux recevoir.
        pkt_del(reception);
        fprintf(stderr, "Sender a déja reçu le sequnum %d, il l'ignore\n", seqnum );
        return INGNORE;
      }
    }
    if(seqnumMax < seqnumMin){
      if (seqnum > seqnumMax && seqnum < seqnumMin){ // En dehors de ce que l'on peux recevoir.
        pkt_del(reception);
        fprintf(stderr, "Sender a déja reçu le sequnum %d, il l'ignore\n", seqnum );
        return INGNORE;
      }
    }
    // Creation et placement de la structure dans le buffer.
    struct buffer* new = malloc(sizeof(struct buffer));
    new->seqnum = seqnum;
    new->data = reception;
    insertStruct(new);
    window--;
    //Envoie du ack.
    struct buffer* current = startBuffer;
    char payloadTemp[512];
    int size = 0;
    while(startBuffer != NULL && startBuffer->seqnum == seqnumMin){
      // Decalage de la fenêtre
      seqnumMin = (seqnumMin +1) % 256;
      seqnumMax = (seqnumMax +1)%256;

      //fprintf(stderr, "Mes numeros de sequnums, mis à jours sont min %d, max %d\n", seqnumMin , seqnumMax );

      lasttimestamp = pkt_get_timestamp(startBuffer->data); // ON en a besoin pour la suite

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
    }
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
    pkt_del(reception);
    return INGNORE;
  }
}


void receptionDonnes(int sfd, int file){
  // Preparation

  //fprintf(stderr, "AVANT DE COMMENCER min %d, max %d\n", seqnumMin , seqnumMax );
  char buf[528];
  char payload[512];
  size_t payloadSize = 512;
  selectiveRepeat_status_code err= 0;

  int end = 0;
  struct pollfd ufds[2];
  ufds[0].fd = sfd;
  ufds[0].events = POLLIN; // check for normal data
  ufds[1].fd = sfd;
  ufds[1].events = POLLOUT;
  while(end == 0 ){
    // attend evenement pendant 10 sec
    int rv = poll(ufds, 2, -1); // Faut-il changer cette valeur? avant : 10000
    // timeout expire
    if(rv==0){
      fprintf(stderr, "Time out\n");
      return;
    }
    else if (rv == -1) {
      fprintf(stderr, "Error lors de l'utilisation de poll\n");
      return ;
    }
    else {
      memset((void*)buf, 0, 528); // make sure the struct is empty
      err = INGNORE;
      // traite la lecture
      if (ufds[0].revents & POLLIN) {
        fprintf(stderr, "il y a de quoi lire\n");
        memset((void*)payload, 0, 512); // make sure the struct is empty
        payloadSize = 512;
        int recu = read(sfd, buf, sizeof buf); // receive normal data
        if(recu == 12){ //faire un truc + propre
          fprintf(stderr, "J'ai reçu le dernier packer, 0\n");
          end = 1;
        }
        else{
          err = traitementRecu(buf, recu, payload, &payloadSize, file);
          fprintf(stderr, "Mes numeros de sequnums après l'envoie du ack sont min %d, max %d\n", seqnumMin , seqnumMax );
        }
      }
      if(err != INGNORE && ufds[1].revents & POLLOUT){
        int sended = write(sfd,payload,payloadSize);
        if(sended != payloadSize){
          fprintf(stderr, "Erreur lors de l'envoi de l'acquitement\n");
        }
      }
    }

  }
}
