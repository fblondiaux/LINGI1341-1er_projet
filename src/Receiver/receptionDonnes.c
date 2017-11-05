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
  int end = 0;

struct buffer* startBuffer = NULL;

void printStruct(){
  struct buffer* parcours = startBuffer;
  while(parcours != NULL){
    fprintf(stderr, "%d->", parcours->seqnum );
    parcours = parcours->next;
  }
  fprintf(stderr, "fin\n");
}

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

void insertStruct(struct buffer* str){

  //fprintf(stderr, "seqnumMin = %d\n", seqnumMin);
  //fprintf(stderr, "seqnumMax = %d\n", seqnumMax);
  struct buffer* parcours = startBuffer;
  int seqnum = pkt_get_seqnum(str->data);

  // si packet déjà présent
  if(alreadyInStruct(seqnum) == 1){
    return;
  }
  window--; // N : chgmt

    /*DEBUG*/ fprintf(stderr,"Receiver stocke le seqnum %d dans son buf\n",seqnum);

  // si rien dans le buffer
  if (parcours == NULL) {
    //fprintf(stderr, "->%d\n",seqnum );
    startBuffer = str;
    startBuffer->next = NULL;
    return ;
  }


  if (seqnumMin < seqnumMax){
    if(seqnum < parcours->seqnum){

      struct buffer *temp = parcours;
      startBuffer = str;
      str->next = temp;


      //str->next = parcours;
      //startBuffer = str;
      return;
    }
    while(parcours->next != NULL && parcours->next->seqnum < seqnum){
    /*DEBUG*/  //fprintf(stderr, "->%d",parcours->seqnum );

      parcours=parcours->next;
    }
    str->next = parcours->next;
    parcours->next = str;
    /*DEBUG*/   //fprintf(stderr, "->%d\n",seqnum );
    return;
  }
  else{
    if(seqnum < 50){ // Seqnum est dans le début des numéros de sequence.
      while(parcours->next != NULL && parcours->next->seqnum > 50){ // On passe tous les seqnums de 200 pour arriver avecc next

          /*DEBUG*/   //fprintf(stderr, "->%d",parcours->seqnum );
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
        //A verif
        while(parcours->next != NULL && parcours->next->seqnum < seqnum){

            //fprintf(stderr, "->%d",parcours->seqnum );
          parcours=parcours->next;
        }
        str->next = parcours->next;
        parcours->next = str;

        //fprintf(stderr, "->%d\n",seqnum );
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


      // N : chgmt
      while( parcours->next!= NULL && parcours->next->seqnum > 50 &&  seqnum > parcours->next->seqnum)
      {
        parcours = parcours->next;
      }
      str->next = parcours->next;
      parcours->next = str;

      /*
      while(parcours->next != NULL && parcours->next->seqnum > seqnum && parcours->next->seqnum > 50){

          fprintf(stderr, "->%d",parcours->seqnum );
        parcours=parcours->next;
      }
      str->next = parcours->next;
      parcours->next = str; */
      return;
    }

  }
}


selectiveRepeat_status_code traitementRecu(char* buf, int taille, char* ACK, size_t* SizeACK, const int file){
  pkt_t* reception = pkt_new();
  pkt_status_code err = pkt_decode(buf, taille, reception);

  if(pkt_get_length(reception) == 0 && err != E_TR){
    fprintf(stderr, "Receiver envoie le ack de fin\n");
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
  if (err == E_TR && pkt_get_type(reception) == 1){
    // On va devoir envoyer un NACK
    fprintf(stderr, "Receiver doit renvoyer un nack\n"); // Preparation du NACK à envoyer.
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
        lasttimestamp = pkt_get_timestamp(reception);

        fprintf(stderr, "Receiver a déja reçu le sequnum %d, il l'ignore mais envoie quand meme un ack\n", seqnum );
        pkt_t* ack = pkt_new();
        pkt_set_type(ack, 2);
        pkt_set_window(ack, window);
        pkt_set_seqnum(ack,seqnumMin);
        pkt_set_length(ack,0);
        pkt_set_timestamp(ack, pkt_get_timestamp(reception)); // N : à la place de lasttimestamp
        pkt_del(reception);                           // N : mis ici
        if(pkt_encode(ack,ACK, (size_t*) SizeACK) != PKT_OK){
          pkt_del(ack);
          return INGNORE;
        }
        return S_ACK;
      }
    }
    if(seqnumMax < seqnumMin){
      if (seqnum > seqnumMax && seqnum < seqnumMin){ // En dehors de ce que l'on peux recevoir.
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
    //printStruct();
    fprintf(stderr, "Resultat apres insertion\n");
    insertStruct(new);
    printStruct();
    //fprintf(stderr, "debug1\n");
    //window--;
    fprintf(stderr, "window avant while : %d\n", window);
    //Envoie du ack.
    struct buffer* current = startBuffer;
    char payloadTemp[512];
    int size = 0;
    //fprintf(stderr, "debug2\n");
    //while(current != NULL && current->seqnum == seqnumMin){
    while(startBuffer != NULL && startBuffer->seqnum == seqnumMin){
      // Decalage de la fenêtre
      seqnumMin = (seqnumMin +1) % 256;
      seqnumMax = (seqnumMax +1) % 256;
      //fprintf(stderr, "Mes numeros de sequnums, mis à jours sont min %d, max %d\n", seqnumMin , seqnumMax );

      //lasttimestamp = pkt_get_timestamp(current->data);
      lasttimestamp = pkt_get_timestamp(startBuffer->data); // ON en a besoin pour la suite
      //fprintf(stderr, "debug2-1\n");
      size = pkt_get_length(current->data);
      memcpy( payloadTemp,pkt_get_payload(current->data), size);
      int ecrit = write(file, payloadTemp, size);
      if(ecrit!=size){
        fprintf(stderr, "Le segment à été reçu mais un problème à été rencontré lors de l'écriture de son payload\n");
      }
      //fprintf(stderr, "debug2-2\n");
      //current = current->next;
      startBuffer = startBuffer->next;
      //current = startBuffer;
      //fprintf(stderr, "debug2-3\n");
      pkt_del(current->data);           // free qui pose problème
      //fprintf(stderr, "debug2-4\n");
      free(current);
      //fprintf(stderr, "debug2-5\n");
      window ++; // ON a libere une place dans le buffer
      fprintf(stderr, "window = %d\n", window);
      current = startBuffer;
    }
    //fprintf(stderr, "debug3\n");
    pkt_t* ack = pkt_new();
    pkt_set_type(ack, 2);
    pkt_set_window(ack, window);
    pkt_set_seqnum(ack,seqnumMin);
    pkt_set_length(ack,0);
    pkt_set_timestamp(ack,lasttimestamp);
    //fprintf(stderr, "debug4\n");
    if(pkt_encode(ack,ACK, (size_t*) SizeACK) != PKT_OK){
      pkt_del(ack);
      return INGNORE;
    }
    ///fprintf(stderr, "debug5\n");

    pkt_del(ack);
    //fprintf(stderr, "debug6\n");
    return S_ACK;

  }
  else{
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

    //fprintf(stderr,"Poll attends un event\n");
    int rv = poll(ufds, 2, -1);
    //fprintf(stderr,"UN evenement a eu lieu\n");
    if (rv == -1) {
      /*DEBUG*/ fprintf(stderr, "Error lors de l'utilisation de poll\n");
      return ;
    }
    else {
      memset((void*)buf, 0, 528); // make sure the struct is empty
      err = INGNORE;
      // traite la lecture
      if (ufds[0].revents & POLLIN) {
        /*DEBUG*/ //fprintf(stderr, "il y a de quoi lire\n");
        memset((void*)payload, 0, 512); // make sure the struct is empty
        payloadSize = 512;
        int recu = read(sfd, buf, sizeof buf); // receive normal data
        if(recu == 12){
          fprintf(stderr, "C'EST LA FIN \n");
        }
        err = traitementRecu(buf, recu, payload, &payloadSize, file);
      }
      if(err != INGNORE && ufds[1].revents & POLLOUT){
        /*DEBUG*/ //fprintf(stderr, "Envoie d'un ack\n" );
        int sended = write(sfd,payload,payloadSize);
        if(sended != payloadSize){
          fprintf(stderr, "Erreur lors de l'envoi de l'acquitement, on continue\n");
        }
      }
    }

  }
  fprintf(stderr, "Moi j'ai fini bye, receiver\n" );
}
