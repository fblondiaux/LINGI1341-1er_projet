#include "packet_interface.h"
#include "envoieDonnes.h"

#include <sys/poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <zlib.h> /* crc32 */

// = read_write_loop_sender


// A enlever
typedef enum {
  IGNORE = 0,     /* A ete ignore */
  S_DATA,        /* Necessite l'envoie d'un packet data*/
  S_ACK,           /* Necessite l'envoye d'un ack*/
  S_NACK,            /* Necessite l'envoye d'un nack */ 
} selectiveRepeat_status_code;

static int min = 0;
static int max = 31;
static int seqnum = 0;
static int window_dest = 1;
static int window = 32;

struct node
{
  pkt_t *pkt; 
  struct node *next;
}

/* rajoute un elem a la fin de la liste
*
* @ return : 0 succes, -1 erreur
*/
int add(pkt_t *pkt, struct node *liste)
{
  struct node *ptr = liste;
  while( ptr->next != NULL)
  {
    ptr = ptr->next;
  }
  struct node *elem = (struct node elem*)malloc(sizeof(struct node));
  if(elem == NULL)
    return -1;

  elem->pkt = pkt;
  elem->next = NULL;
  ptr->next = elem;
  return 0;
}

/*
* supprime  de la liste chainee le node dont le seqnum du pkt est le meme que le seqnum du 
*   pkt donne en argument.
*
* @return : 0 succes, -1 erreur
*/
int delete(pkt_t *pkt, struct node *liste)
{
  uint8_t seqnum = pkt_get_seqnum(pkt);

  struct node *ptr = liste;

  /* A TERMINER  (ne pas oublier de free le node !) */

  while( pkt_get_seqnum(ptr->pkt) != seqnum && ptr->next != NULL)
  {
    ptr = ptr->next;
  }
  // dernier elem ou elem dont les seqnums correspondent
  if(pkt_get_seqnum(ptr->pkt) == seqnum)
  {
    //dernier elem
    if( ptr->next == NULL)
    {
      pkt_del(ptr->pkt);

    }
  }
}

/*
*  Cherche dans la liste le node dont le seqnum du pkt == seqnum du pkt donne en arg
* 
* @return pkt_t* si trouve ou NULL si pas trouve
*/
pkt_t* search(pkt_t pkt, struct node *liste)
{
  uint8_t seqnum =  pkt_get_seqnum(pkt);

  struct node *ptr = liste;
  while( pkt_get_seqnum(ptr->pkt) != seqnum && ptr->next != NULL)
  {
    ptr = ptr->next;
  }
  // les seqnum correspondent ou dernier elem
  if(pkt_get_seqnum(ptr->pkt) == seqnum)
  {
    return ptr->pkt;
  }
  return NULL;
}

int envoieDonnes(struct sockaddr_in6 *addr, int sfd, int file){
  // Preparation
  if(file == -1){
    file = STDIN_FILENO;
  }
  char buf[528]; // 512 + 16
  char payload[512];
  int end = 0;
  struct pollfd ufds[2];
  ufds[0].fd = file;
  ufds[0].events = POLLIN; // check for normal data
  ufds[1].fd = file;
  ufds[1].events = POLLIN; // check for normal data

  struct node liste = NULL;

  while(end == 0 ){
    // attend evenement pendant 10 sec
    int rv = poll(ufds, 2, 10000);

    // timeout expire
    if(rv==0){
      fprintf(stderr, "Time out\n");
      return;
    }
    if (rv == -1) {
      fprintf(stderr, "Error lors de l'utilisation de poll\n");
      return ;
    }
    else {
      memset((void*)buf, 0, 528); // make sure the struct is empty
      
      // traite la lecture
      if (ufds[0].revents & POLLIN) {
        // receiver a recu un acquittement
        int recu = read(sfd, buf, sizeof buf); 
        checkReceive(buf, sizeof(buf), &liste);
      }

      // traite l'ecriture
      if(ufds[1].revents & POLLIN)  {
        // receiver 
        memset((void*)payload, 0, 512); // make sure the struct is empty

        // lu : nombre de bytes qui ont été lues dans file ?
        int lu = read(file,payload, 512);
        int nombre = prepareToSend(payload, lu, buf);
        int sended = write(sfd,buf,nombre);
        if(sended != nombre){
          fprintf(stderr, "Erreur lors de l'envoi\n");
        }
        seqnum = ((seqnum+1)%256);
      }
    }
    end = feof(file);
  }
}

/*
* Receiver a recu a recu des donnees (normalment d'acquittement ou de non-acquittement)
* checkReceive gere ces donnees. 
*/
int checkReceive(const char* buf, const size_t len, struct node *liste)
{
  struct pkt_t *pkt = pkt_new();

  // convertir buf en pkt
  pkt_status_code = pkt_decode(buf, len, pkt);
  if(pkt_status_code != PKT_OK)
  {
    fprintf(stderr, "erreur dans décodage\n");
    return -1;
  }

  // verifier type  et tr du packet recu 
  ptypes_t type = pkt_get_type(pkt);
  uint8_t trFlag = pkt_get_tr(pkt);

  if( type == PTYPE_DATA || trFlag != 0)
  {
    return -1; // ignore
  }

  // packet d'acquittement
  if(type == PTYPE_ACK)
  {
    // verifier qu'on est dans la fenetre de sequence + timestamp? 
    if(pkt_get_seqnum(pkt) > min && pkt_get_seqnum(pkt) < max) // a verifier (+ verifier timestamp?)
    {
      return IGNORE;
    }
    int err = delete(pkt, liste);
    if (err!= 0)
      return; // a completer

    // met a jour la fenetre de reception
    min = pkt_get_seqnum(pkt);
    max = min+31;
    return IGNORE;
  }

  if(type == PTYPE_NACK)
  {
    // verifier qu'on est dans la fenetre de sequence + timestamp? 
    if(pkt_get_seqnum(pkt) > min && pkt_get_seqnum(pkt) < max) // a verifier (+ verifier timestamp?)
    {
      return IGNORE;
    }
     pkt_t *renvoyer = search(pkt, liste);
     if( renvoyer == NULL)
      return -1;
    // changer timestamp
    // pkt_decode
    // renvoyer

  }

  // gerer le cas ou ack/nack n'arrive jamais -> timeout
}


int prepareToSend(char* payload, int taillePayload, char* toSend, struct node *liste)
{
  pkt_status_code err;
  pkt_t *pkt = pkt_new();


  // on insere les donnees dans le pkt
  err = pkt_set_type(pkt, PTYPE_DATA);
  if( err != PKT_OK)
    return 0;
  err = pkt_set_tr(pkt, 0);
  if( err != PKT_OK)
    return 0;
  err = pkt_set_window = window;
  if( err != PKT_OK)
    return 0;
  err = pkt_set_seqnum = seqnum;
  if( err != PKT_OK)
    return 0;
  err = pkt_set_length = taillePayload;
  if( err != PKT_OK)
    return 0;
  err = pkt_set_timestamp = (uint32_t)time(NULL);
  if( err != PKT_OK)
    return 0;
  uLong crc = crc32(0L, Z_NULL, 0);
  char *data;
  memcpy(data, pkt, 8);
  err = pkt_set_crc1(crc32(crc,(Bytef*) data, 8));
  if( err != PKT_OK)
    return 0;

  if( payload!=NULL && taillePayload!= 0)
  {
    err = pkt_set_payload(pkt, payload, taillePayload);
    if( err != PKT_OK)
      return 0;
    err = pkt_set_crc2(crc32(crc, (Bytef*) payload, taillePayload));
    if( err != PKT_OK)
      return 0;
  }

  //rajoute pkt dans le buffer de reception
  int err = add(pkt, liste);
  if(err != 0)
    return 0;
  window--;



  //taille dispo dans toSend
  int length = 528;
  
  // length-POST : nombre d'octets ecrits dans toSend
  err = pkt_encode(pkt, toSend, &length);

  return length;
}
