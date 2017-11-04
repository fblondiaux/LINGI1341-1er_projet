#include "envoieDonnes.h"

#include <sys/poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <zlib.h> /* crc32 */
#include <time.h>
#include <netinet/in.h> /* * sockaddr_in6 */
#include <sys/types.h>

// = read_write_loop_sender


static int min = 0;
static int max = MAX_WINDOW_SIZE;
static int seqnum = 0;
static int window_dest = 1;
static int window = MAX_WINDOW_SIZE;


/* rajoute un elem a la fin de la liste
*
* @ return : 0 succes, -1 erreur
*/
int add(pkt_t *pkt, struct head *buf)
{
  struct node* elem = (struct node*)malloc(sizeof(struct node));
  if(elem == NULL)
    return -1;
  elem->pkt = pkt;
  elem->next = NULL;


  if (buf->liste == NULL){
    buf->liste = elem;
    return 0;
  }

  struct node *ptr = buf->liste;
  while( ptr->next != NULL)
  {
    ptr = ptr->next;
  }
  ptr->next = elem;
  return 0;
}

/*
* supprime  de la liste chainee le node dont le seqnum du pkt est le meme que le seqnum du
*   pkt donne en argument.
*
* @return : 0 succes, -1 erreur
*/
int del(pkt_t *pkt, struct head *reception)
{
  uint8_t seq = pkt_get_seqnum(pkt);

  struct node *ptr = reception->liste;

  fprintf(stderr, "del 1\n");

  if(ptr == NULL)
  {
    fprintf(stderr, "le buffer est vide !! :(\n");
    return -1;
  }
  fprintf(stderr, "del 2\n");

  fprintf(stderr, "pkt_get_seqnum(ptr->pkt) = %d\n",pkt_get_seqnum(ptr->pkt) );
  fprintf(stderr, "seq = %d\n", seq);
  // a déjà été supprimé
  if( pkt_get_seqnum(ptr->pkt) > seq)
  {
    fprintf(stderr, "on rentre dans la boucle\n");
	  return -1;
  }

  fprintf(stderr, "del 3\n");

  while( pkt_get_seqnum(ptr->pkt) != seq)
  {
    reception->liste = ptr->next;
    pkt_del(ptr->pkt);
    ptr = ptr->next;
    if(ptr== NULL)
    {
      return -1; // ne l'a pas trouvé et a supprimé toute la liste chainée...
    }
  }

  fprintf(stderr, "del 4\n");
  reception->liste = ptr->next;
  pkt_del(ptr->pkt);
  return 0;
}

/*
* Receiver a recu a recu des donnees (normalment d'acquittement ou de non-acquittement)
* checkReceive gere ces donnees.
*/
int checkReceive(const char* buf, const size_t len, struct head *reception)
{
  pkt_t *pkt = pkt_new();

  // convertir buf en pkt
  pkt_status_code err = pkt_decode(buf, len, pkt);
  if(err != PKT_OK)
  {
    fprintf(stderr, "erreur dans décodage\n");
    return -1;
  }

  // verifier type  et tr du packet recu
  ptypes_t type = pkt_get_type(pkt);
  uint8_t trFlag = pkt_get_tr(pkt);

  if( type == PTYPE_DATA || trFlag != 0)
  {
    fprintf(stderr, "type de packet incohérent ou packet tronqué");
    return -1;; // ignore
  }

  // verifier qu'on est bien censé recevoir ce packet : min <= seqnum <= max, et timestamp correct
  if( min < max) {
    if(pkt_get_seqnum(pkt) < min || pkt_get_seqnum(pkt) > max) {
      /* DEBUG */ fprintf(stderr, "Sender : seqnum hos séquence (1)\n");
      return 0;
    }
  }
  else {
    if(max < pkt_get_seqnum(pkt) && pkt_get_seqnum(pkt)< min) {
    /* DEBUG */ fprintf(stderr, "Sender : seqnum hos séquence (2)\n");
      return 0;
    }
  }

  if ((uint32_t)time(NULL) > (pkt_get_timestamp(pkt)+5)) {
    fprintf(stderr, "checkReceive : timestamp trop vieux\n");
    return 0;
  }

  if(type == PTYPE_ACK)
  {
    window_dest = pkt_get_window(pkt);

    pkt_set_seqnum(pkt, pkt_get_seqnum(pkt)-1);
    fprintf(stderr, "Sender va suprimer le packet de seqnum : %d\n", pkt_get_seqnum(pkt));
    int ret = del(pkt, reception);
    if (ret!= 0)
    {
      fprintf(stderr, "Impossible de supprimer ce packet du buffer de réception de Sender\n");
      //fprintf(stderr, "on voulait supprimer le packet avec seqnum = %d\n", pkt_get_seqnum(pkt));

      

      // ----------------------------------------
      pkt_del(pkt);
      return 0;
    }
    pkt_set_seqnum(pkt, pkt_get_seqnum(pkt)+1);

    if(pkt_get_seqnum(pkt) > min){
      window = window +(pkt_get_seqnum(pkt)-min);
      fprintf(stderr, "(1) window = %d\n", window);
    }
    else{
      window = window + 256 - min + pkt_get_seqnum(pkt);
      fprintf(stderr, "(2) window= %d\n", window);
    }
    min = (pkt_get_seqnum(pkt)%256);
    max = ((min+MAX_WINDOW_SIZE)%256);
    fprintf(stderr, "Sender a reçu ack, on change min = %d\n", min);
    fprintf(stderr, "Sender a reçu ack, on change max = %d\n", max);

    pkt_del(pkt);
    return 0;
  }

  // packet de non acquittement.
  // on choisit d'ignorer le packet. Le time-out s'occupera de renvoyer le packet qui a été reçu tronqué.
  if(type == PTYPE_NACK)
  {
    //printf("Sender : reçu un NACK\n");
    // renvoyer packet
    pkt_del(pkt);
    return 0;
  }
  return 0;
}


int prepareToSend(char* payload, int taillePayload, char* toSend, struct head *reception)
{
  pkt_status_code err;
  pkt_t *pkt = pkt_new();

  err = pkt_set_type(pkt, PTYPE_DATA);
  if( err != PKT_OK)
  {
    fprintf(stderr, "prepareToSend : set type\n");
    return 0;
  }
  err = pkt_set_tr(pkt, 0);
  if( err != PKT_OK)
  {
    fprintf(stderr, "prepareToSend : set tr\n");
    return 0;
  }
  err = pkt_set_window(pkt, window);
  if( err != PKT_OK)
  {
    fprintf(stderr, "prepareToSend : set window\n");
    fprintf(stderr, "window = %d\n", window);
    return 0;
  }
  err = pkt_set_seqnum(pkt, seqnum);
  if( err != PKT_OK)
  {
   fprintf(stderr, "prepareToSend : set seqnum\n");
    return 0;
  }
  err = pkt_set_length(pkt, taillePayload);
  if( err != PKT_OK)
  {
    fprintf(stderr, "prepareToSend : set length\n");
    return 0;
  }
  err = pkt_set_timestamp(pkt, (uint32_t)time(NULL));
  if( err != PKT_OK)
  {
    fprintf(stderr, "prepareToSend : set timestamp\n");
    return 0;
  }

  uLong crc = crc32(0L, Z_NULL, 0);
  char *data = (char *)malloc(8);
  memcpy(data, pkt, 8);

  err = pkt_set_crc1(pkt, crc32(crc,(Bytef*) data, 8));
  if( err != PKT_OK)
  {
    fprintf(stderr, "prepareToSend : set crc1\n");
    return 0;
  }

  if( payload!=NULL && taillePayload!= 0)
  {
    err = pkt_set_payload(pkt, payload, taillePayload);
    if( err != PKT_OK)
    {
      fprintf(stderr, "prepareToSend : set payload  %d\n",err);
      return 0;
    }
    err = pkt_set_crc2(pkt, crc32(crc, (Bytef*) payload, taillePayload));
    if( err != PKT_OK)
    {
      fprintf(stderr, "prepareToSend : set crc2\n");
      return 0;
    }
  }


  //rajoute pkt dans le buffer de reception
  int nb = add(pkt, reception);
  //printf("prepareToSend : 5\n");
  if(nb != 0)
    return 0;


  //taille dispo dans toSend
  int length = 528;

  // length-POST : nombre d'octets ecrits dans toSend

  err = pkt_encode(pkt, toSend, (size_t*) &length);

  return length;
}

int envoieDonnes( int sfd, FILE* f){
  // Preparation
  int file;
  if(f == NULL){
    file = STDIN_FILENO;
  }
  else
  {
    file = fileno(f);
  }
  char buf[528]; // 512 + 16
  char payload[512];
  int end = 0;
  struct pollfd ufds[2];
  ufds[0].fd = sfd;
  ufds[0].events = POLLIN; // check for normal data
  ufds[1].fd = file;
  ufds[1].events = POLLIN; // check for normal data

  int attendre = 0;

  // initialisation d'un buffer vide.
  struct head *reception = (struct head *)malloc(sizeof(struct head));
  if( reception==NULL)
    return -1; // a changer?
  reception->liste = NULL;

  struct node *ptr = reception->liste; /* changé */

  while(end == 0 ){
    // attend evenement pendant 10 sec
    int rv = poll(ufds, 2, 10000);

    // timeout expire
    if(rv==0){
      fprintf(stderr, "Time out\n");
      return -1;
    }
    if (rv == -1) {
      fprintf(stderr, "Error lors de l'utilisation de poll\n");
      return -1;
    }
    else {
      memset((void*)buf, 0, 528); // make sure the struct is empty

      // traite la lecture
      if (ufds[0].revents & POLLIN) {
        //printf("Sender : il y a de quoi lire !\n");
        // receiver a recu un acquittement
        int lu = read(sfd, buf, sizeof buf);

        checkReceive(buf, lu, reception);

        // tous les éléments du fichier ont été envoyés et tous les ack ont été reçus
        if(reception->liste == NULL && attendre ==1)
        {
          fprintf(stderr, "sender : tous les elems du fichiers ont été envoyés, tous les acks ont été reçus\n");
          fprintf(stderr, "Sender : Je vais envoyer un packet vide\n");
            int nombre = prepareToSend(NULL, 0, buf, reception);
            if(nombre == 0)
            {
              fprintf(stderr, "erreur de prepareToSend\n");
            }
            fprintf(stderr, "nombre = %d\n", nombre);
            int sended = write(sfd, buf, nombre);
            if(sended != nombre){
              fprintf(stderr, "Erreur lors de l'envoi\n");
            }
            end = 1;
            fprintf(stderr, "err = 1\n");
        }
      }

      // y a-t-il des timeout?
      ptr = reception->liste;  /* changé */
      //fprintf(stderr, "ici\n");
      while(ptr!=NULL)
      {
        //fprintf(stderr, "timeout?\n");
        if( (uint32_t)time(NULL) > (pkt_get_timestamp(ptr->pkt)+5))
        {
          fprintf(stderr, "timeout du packet de seq : %d --> on réenvoie les données\n", pkt_get_seqnum(ptr->pkt));
          pkt_status_code err = pkt_set_timestamp(ptr->pkt, (uint32_t)time(NULL));
          if( err == PKT_OK)
          {
            int t = 528;
            err = pkt_encode(ptr->pkt, buf, (size_t *) &t);
            if (err == PKT_OK)
            {
              int sended = write(sfd,buf,t);
              if(sended != t){
                fprintf(stderr, "Erreur lors de l'envoi\n");
              }
              fprintf(stderr, "sender a renvoyé le packet dont seqnum = %d\n", pkt_get_seqnum(ptr->pkt));
            }
          }
        }
        ptr = ptr->next;
      }

      // traite l'ecriture
      if(ufds[1].revents & POLLIN && window_dest > 0 && window > 0 && attendre == 0)  {
        memset((void*)payload, 0, 512); // make sure the struct is empty

        // lu : nombre de bytes qui ont été lues dans file
        int lu = read(file,payload, 512);
        if(lu == 0)
        {
          fprintf(stderr, "J'ai vu que lu == 0\n");
          attendre = 1;

          // on ne passe jamais dans cette boucle??
          if(reception->liste == NULL){
            fprintf(stderr, "Sender : Je vais envoyer un packet vide\n");
            int nombre = prepareToSend(NULL, 0, buf, reception);
            if(nombre == 0)
            {
              fprintf(stderr, "erreur de prepareToSend\n");
            }
            fprintf(stderr, "nombre = %d\n", nombre);
            int sended = write(sfd, buf, nombre);
            if(sended != nombre){
              fprintf(stderr, "Erreur lors de l'envoi\n");
            }
            end = 1;
            fprintf(stderr, "err = 1\n");
          }



        }
        else
        {
          int nombre = prepareToSend(payload, lu, buf, reception);
          if(nombre == 0)
          {
            fprintf(stderr, "echec prepareToSend\n");
          }
          if( nombre != 0)
          {
            int sended = write(sfd,buf,nombre);
            if(sended != nombre){
              fprintf(stderr, "Erreur lors de l'envoi\n");
            }
            seqnum = ((seqnum+1)%256);
            window_dest--;
            window--;
          }
        }
      }
    }
    // if(f == NULL){
    //   end = feof(stdin);
    // }
    // else{

    //   end = feof(f);
    // }
  }
  free(reception);
  return 0;
}
