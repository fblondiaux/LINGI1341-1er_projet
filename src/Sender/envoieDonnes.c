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
static int max = 31;
static int seqnum = 0;
static int window_dest = 1;
static int window = 31;


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
  //printf("add : 3\n");
  ptr->next = elem;
  //printf("add : 4\n");
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
  reception->liste = ptr->next;
  pkt_del(ptr->pkt);
  free(ptr);
  return 0;

/*
  //premier node a supprimer?
  if( pkt_get_seqnum(ptr->pkt) == seq)
  {
    reception->liste = ptr->next;
    pkt_del(ptr->pkt);
    free(ptr);
    return 0;
  }
  else
  {
    if( ptr->next == NULL)
    {
      return -1;
    };
    // jusqu'a max l'avant dernier
    while( pkt_get_seqnum((ptr->next)->pkt) != seq  && (ptr->next)->next != NULL)
    {
      ptr = ptr->next;
    }

    if(pkt_get_seqnum((ptr->next)->pkt) == seq)
    {
      struct node *node_del = ptr->next;
      ptr->next = (ptr->next)->next;
      pkt_del(node_del->pkt);
      free(node_del);
      return 0;
    }
  }
  // ne l'a pas trouve
  return -1;
  */



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
  //printf("Sender a décodé les données -> packet\n");


  // verifier type  et tr du packet recu
  ptypes_t type = pkt_get_type(pkt);
  uint8_t trFlag = pkt_get_tr(pkt);

  if( type == PTYPE_DATA || trFlag != 0)
  {
    //fprintf(stderr, "type de packet incohérent ou packet tronqué");
    return -1;; // ignore
  }

  // verifier qu'on est bien censé recevoir ce packet : min <= seqnum <= max, et timestamp correct
  if( min < max)
  {
    if(pkt_get_seqnum(pkt) < min || pkt_get_seqnum(pkt) > max)
    {
      //printf("checkReceive : hors séq1\n");
      return 0;
    }
  }
  else
  {
    if(max < pkt_get_seqnum(pkt) && pkt_get_seqnum(pkt)< min)
    {
      //printf("checkReceive : hors séq2\n");
      return 0;
    }
  }

  if ((uint32_t)time(NULL) > (pkt_get_timestamp(pkt)+5))
  {
    printf("checkReceive : timestamp trop vieux\n");
    return 0;
  }
  //printf("checkReceive : 1\n");

  // packet d'acquittement
  if(type == PTYPE_ACK)
  {
    //printf("Sender : il s'agit d'un acquittement\n");
    //printf("checkReceive : 2\n");

    //mettre à jour la valeur de window_dest, window, min et max :

    window_dest = pkt_get_window(pkt);
    /*
    int n = seqnum - min;
    window = window +n; */

    if(pkt_get_seqnum(pkt) > min){
      window = window +(pkt_get_seqnum(pkt)-min);
      printf("window = %d\n", window);
    }
    else{
      window = 32 - min + pkt_get_seqnum(pkt);
      printf("window = %d\n", window);
    }
    min = (pkt_get_seqnum(pkt)%256);
    max = ((min+31)%256);

    pkt_set_seqnum(pkt, pkt_get_seqnum(pkt)-1);

    int ret = del(pkt, reception);
    //printf("checkReceive : 8\n");
    if (ret!= 0)
    {
      fprintf(stderr, "Impossible de supprimer ce packet du buffer de réception de Sender\n");
      fprintf(stderr, "on voulait supprimer le packet avec seqnum = %d\n", pkt_get_seqnum(pkt));
      return -1;
    }
    //printf("sender : le ACK a été traité !\n");
    return 0;
  }

  // packet de non acquittement.
  // on choisit d'ignorer le packet. Le time-out s'occupera de renvoyer le packet qui a été reçu tronqué.
  if(type == PTYPE_NACK)
  {
    //printf("Il s'agit d'un packet de non-acquittement\n");
    // renvoyer packet
    return 0;
  }
  return -1;
}


int prepareToSend(char* payload, int taillePayload, char* toSend, struct head *reception)
{
  pkt_status_code err;
  pkt_t *pkt = pkt_new();

  //printf("prepareToSend : 1\n");
  // on insere les donnees dans le pkt
  err = pkt_set_type(pkt, PTYPE_DATA);
  if( err != PKT_OK)
  {
    //printf("prepareToSend : set type\n");
    return 0;
  }
  err = pkt_set_tr(pkt, 0);
  if( err != PKT_OK)
  {
    //printf("prepareToSend : set tr\n");
    return 0;
  }
  err = pkt_set_window(pkt, window);
  if( err != PKT_OK)
  {
    //printf("prepareToSend : set window\n");
    //printf("window = %d\n", window);
    return 0;
  }
  err = pkt_set_seqnum(pkt, seqnum);
  //printf("prepareToSend : on met seqnum = %d\n", seqnum);
  if( err != PKT_OK)
  {
   //printf("prepareToSend : set seqnum\n");
    return 0;
  }
  err = pkt_set_length(pkt, taillePayload);
  if( err != PKT_OK)
  {
    //printf("prepareToSend : set length\n");
    return 0;
  }
  err = pkt_set_timestamp(pkt, (uint32_t)time(NULL));
  if( err != PKT_OK)
  {
    //printf("prepareToSend : set timestamp\n");
    return 0;
  }

  //printf("prepareToSend : 2\n");
  uLong crc = crc32(0L, Z_NULL, 0);
  char *data = (char *)malloc(8);
  memcpy(data, pkt, 8);
  //printf("prepareToSend : 3\n");

  err = pkt_set_crc1(pkt, crc32(crc,(Bytef*) data, 8));
  if( err != PKT_OK)
  {
    //printf("prepareToSend : set crc1\n");
    return 0;
  }

  if( payload!=NULL && taillePayload!= 0)
  {
    err = pkt_set_payload(pkt, payload, taillePayload);
    if( err != PKT_OK)
    {
      //printf("prepareToSend : set payload  %d\n",err);
      return 0;
    }
    err = pkt_set_crc2(pkt, crc32(crc, (Bytef*) payload, taillePayload));
    if( err != PKT_OK)
    {
      //printf("prepareToSend : set crc2\n");
      return 0;
    }
  }

  //printf("prepareToSend : 4\n");

  //rajoute pkt dans le buffer de reception
  int nb = add(pkt, reception);
  //printf("prepareToSend : 5\n");
  if(nb != 0)
    return 0;
  //window--;



  //taille dispo dans toSend
  int length = 528;

  // length-POST : nombre d'octets ecrits dans toSend

  //printf("prepareToSend : 6\n");
  err = pkt_encode(pkt, toSend, (size_t*) &length);
  //printf("prepareToSend : length = %d\n", length);

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
          int sended = write(sfd,buf,0);
          if(sended != 0){
            fprintf(stderr, "Erreur lors de l'envoi\n");
          }
          end = 1;
        }
      }

      // y a-t-il des timeout?
      struct node *ptr = reception->liste;
      while(ptr!=NULL)
      {
        //timeout
        if( (uint32_t)time(NULL) > (pkt_get_timestamp(ptr->pkt)+5))
        {
          printf("timeout --> on réenvoie les données\n");
          //printf("sender retransmission d'un packer\n");
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
            }

          }
        }
        ptr = ptr->next;
      }

      // traite l'ecriture
      if(ufds[1].revents & POLLIN && window_dest > 0 && window > 0 && attendre == 0)  { //--------------------------------------
        //printf("envoieDonnes : il y a de quoi écrire !\n");
        // receiver
        memset((void*)payload, 0, 512); // make sure the struct is empty

        // lu : nombre de bytes qui ont été lues dans file ?
        int lu = read(file,payload, 512);
        if( lu == 0)
        {
          //printf("J'ai vu que lu == 0\n");
          attendre = 1;
          if(reception->liste == NULL){
            int sended = write(sfd,buf,0);
            if(sended != 0){
              fprintf(stderr, "Erreur lors de l'envoi\n");
            }
            end = 1;
          }
        }
        else
        {
          //printf("sender : appel prepareToSend\n");
          int nombre = prepareToSend(payload, lu, buf, reception);
          if( nombre != 0)
          {
            //printf("sender : va écrire\n");
            int sended = write(sfd,buf,nombre);
            //printf("sender : a écrit \n");
            if(sended != nombre){
              fprintf(stderr, "Erreur lors de l'envoi\n");
            }
            if( seqnum == 254)
            {
              printf("seqnum = 254\n");
              printf(" (seqnum+1)mod 256 = %d\n", (seqnum+1)%256);
            }
            seqnum = ((seqnum+1)%256);




            //printf("seqnum = %d\n", seqnum);
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
