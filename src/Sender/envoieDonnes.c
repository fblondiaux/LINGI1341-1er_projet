/*
* Code réalisé par :
* Noemie verstraete - 25021500
* Florence Blondiaux - 06521500
* Version du 05.11.17
*/

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

static int min = 0;
static int max = MAX_WINDOW_SIZE;
static int seqnum = 0;
static int window_dest = 1;
static int window = MAX_WINDOW_SIZE;

// verifier accent ! 
// verifier paquet et non packet ! 


/* 
* Rajoute une structure node qui contient un pointeur vers pkt a la fin 
* de la liste chainee dont le pointeur vers la tete de cette liste chainee est buf. 
*
* @pkt : pointeur vers un packet
* @buf : pointeur vers une structure head qui represente la tete de la liste chainee
* @return : 0 succes, -1 erreur
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
  while(ptr->next != NULL)
  {
    ptr = ptr->next;
  }
  ptr->next = elem;
  return 0;
}

/*
* Supprime  de la liste chainee (dont la tete est buf) le node qui a un paquet dont le numero de sequence est egal
* au numero de sequence du paquet donne en argument, ainsi que tous les nodes dont le paquet a ete envoye precedemment. 
* 
* Si la liste chainee ne contient pas de node dont le numero de sequence du paquet est egal au numero de sequence 
* du paquet donne en argument, (c-a-d : si sender a deja recu un ack avec ce numero de sequence), retourne -1
* Sinon, retourne 0.
*  
* @pkt : pointeur vers un paquet 
* @buf : pointeur vers une structure head qui est la tete d'une la liste chainee de node.
* @return : 0 ou -1 
*/
int del(pkt_t *pkt, struct head *reception)
{
  uint8_t seq = pkt_get_seqnum(pkt);
  struct node *ptr = reception->liste;

  if(ptr == NULL) {
    fprintf(stderr, "Le buffer d'envoi de sender est vide\n");
    return -1;
  }

  // sender a-t-il deja recu un ack concernant ce packet? si oui -> return -1
  if(min < max )
  {
    if( pkt_get_seqnum(ptr->pkt) > seq)
    {
      return -1;
    }
  }
  if( max < min)
  {
    if(pkt_get_seqnum(ptr->pkt) > seq && (pkt_get_seqnum(ptr->pkt)-seq)< 32 )
    {
      return -1;
    }
  }

  while(pkt_get_seqnum(ptr->pkt) != seq)
  {
    reception->liste = ptr->next;
    pkt_del(ptr->pkt);
    ptr = ptr->next;
    if(ptr== NULL)
    {
      return -1; // la paquet n'a pas ete trouve et le buffer est vide
    }
  }
  reception->liste = ptr->next;
  pkt_del(ptr->pkt);
  return 0;
}

/*
* checkreceive gere les donnees que receiver a recues : 
* checkereceive ignore le packet si
* - le paquet est tronqué ou incohéerent
* - le numero de sequence du paquet est en dehors de la fenetre de numeros de sequence autorises.
* - le timestamp du paquet est trop vieux
* - le packet est de type NACK. Dans ce cas, ce packet sera reenvoye au receiver au bout d'un certain temps (grace au timer)
*
* Si le paquet n'est pas ignore pour les raisons precedantes et qu'il s'agit d'un paquet de type ACK,
* checkreceive met a jour window_dest, tente de supprimer le paquet de donnees correspondant au ack 
* dans le buffer d'envoi (liste chainee dont le pointeur vers la tete est reception), et
* met a jour window, min et max.
*
* @buf : buffer contenant les donnees a gerer
* @len : taille en bytes des donnees recues
* @reception : pointeur vers une structure head qui represente la tete de la liste chainee (buffer d'envoi)
* @return : 
*     - -2 si le paquet n'a pas pu etre supprime du buffer d'envoi et est donc ignore
*     - -1 si le paquet est incoherent ou si erreur dans decodage
*     -  0 si un/des paquets a/ont ete supprime ou si on ignore la paquet pour une autre raison
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
    fprintf(stderr, "type de packet incohérent ou packet tronqué\n");
    return -1;
  }

  // verifier que sender est bien censé recevoir ce packet : min <= seqnum <= max, et timestamp correct
  if( min < max) {
    if(pkt_get_seqnum(pkt) < min || pkt_get_seqnum(pkt) > max) {
      fprintf(stderr, "Sender : seqnum hos séquence (1)\n");
      return 0;
    }
  }
  else {
    if(max < pkt_get_seqnum(pkt) && pkt_get_seqnum(pkt)< min) {
      fprintf(stderr, "Sender : seqnum hos séquence (2)\n");
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
    fprintf(stderr, "window_dest = %d\n", window_dest);

    pkt_set_seqnum(pkt, pkt_get_seqnum(pkt)-1);
    fprintf(stderr, "Sender va suprimer le packet de seqnum : %d\n", pkt_get_seqnum(pkt));
    int ret = del(pkt, reception);
    if (ret!= 0)
    {
      /* DEBUG */ fprintf(stderr, "Impossible de supprimer ce packet du buffer de réception de Sender\n");
      pkt_del(pkt);
      return -2; 
    }
    pkt_set_seqnum(pkt, pkt_get_seqnum(pkt)+1);

    if(pkt_get_seqnum(pkt) > min){
      window = window +(pkt_get_seqnum(pkt)-min);
      /* DEBUG */ fprintf(stderr, "(1) window = %d\n", window);
    }
    else{
      window = window + 256 - min + pkt_get_seqnum(pkt);
      /* DEBUG */ fprintf(stderr, "(2) window= %d\n", window);
    }
    min = (pkt_get_seqnum(pkt)%256);
    max = ((min+MAX_WINDOW_SIZE)%256);
    /* DEBUG */ fprintf(stderr, "Sender a reçu ack, on change min = %d\n", min);
    /* DEBUG */ fprintf(stderr, "Sender a reçu ack, on change max = %d\n", max);

    pkt_del(pkt);
    return 0;
  }

  if(type == PTYPE_NACK)
  {
    pkt_del(pkt);
    return 0;
  }
  return 0;
}

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
int prepareToSend(char* payload, int taillePayload, char* toSend, struct head *reception)
{
  pkt_status_code err;
  pkt_t *pkt = pkt_new();

  err = pkt_set_type(pkt, PTYPE_DATA);
  if( err != PKT_OK)
  {
    fprintf(stderr, "echec prepareToSend : set type\n");
    return 0;
  }
  err = pkt_set_tr(pkt, 0);
  if( err != PKT_OK)
  {
    fprintf(stderr, "echec prepareToSend : set tr\n");
    return 0;
  }
  err = pkt_set_window(pkt, window);
  if( err != PKT_OK)
  {
    fprintf(stderr, "echec prepareToSend : set window\n");
    /* DEBUG*/ fprintf(stderr, "window = %d\n", window);
    return 0;
  }
  err = pkt_set_seqnum(pkt, seqnum);
  if( err != PKT_OK)
  {
   fprintf(stderr, "echec prepareToSend : set seqnum\n");
    return 0;
  }
  err = pkt_set_length(pkt, taillePayload);
  if( err != PKT_OK)
  {
    fprintf(stderr, "echec prepareToSend : set length\n");
    return 0;
  }
  err = pkt_set_timestamp(pkt, (uint32_t)time(NULL));
  if( err != PKT_OK)
  {
    fprintf(stderr, "echec prepareToSend : set timestamp\n");
    return 0;
  }

  uLong crc = crc32(0L, Z_NULL, 0);
  char *data = (char *)malloc(8);
  memcpy(data, pkt, 8);

  err = pkt_set_crc1(pkt, crc32(crc,(Bytef*) data, 8));
  if( err != PKT_OK)
  {
    fprintf(stderr, "echec prepareToSend : set crc1\n");
    return 0;
  }

  if( payload!=NULL && taillePayload!= 0)
  {
    err = pkt_set_payload(pkt, payload, taillePayload);
    if( err != PKT_OK)
    {
      fprintf(stderr, "echec prepareToSend : set payload  %d\n",err);
      return 0;
    }
    err = pkt_set_crc2(pkt, crc32(crc, (Bytef*) payload, taillePayload));
    if( err != PKT_OK)
    {
      fprintf(stderr, "echec prepareToSend : set crc2\n");
      return 0;
    }
  }

  //rajoute pkt dans le buffer d'envoi
  int nb = add(pkt, reception);
  if(nb != 0) {
    return 0;
  }

  //taille dispo dans toSend
  int length = 528;
  // length-POST : nombre d'octets ecrits dans toSend
  err = pkt_encode(pkt, toSend, (size_t*) &length);
  return length;
}


/*
* Lis dans un fichier les donnees a envoyer, les envoie au receiver et 
* recoit et gere les acks/nacks envoyes par le receiver
*
* @sfd : un file descriptor 
* @f : pointeur FILE
* @return : 0 succes, -1 erreur
*/
int envoieDonnes( int sfd, FILE* f){
  int filetemp;
  if(f == NULL){
    filetemp = STDIN_FILENO;
  }
  else {
    filetemp = fileno(f);
    /* DEBUG */ fprintf(stderr, "file desc a comme valeur au tout debut %d\n", filetemp);
  }
  const int file = filetemp;
  char buf[528]; // 512 + 16
  char payload[512];
  int end = 0;
  struct pollfd ufds[2];
  ufds[0].fd = sfd;
  ufds[0].events = POLLIN; 
  ufds[1].fd = file;
  ufds[1].events = POLLIN; 
  int attendre = 0;

  // initialisation d'un buffer vide.
  struct head *reception = (struct head *)malloc(sizeof(struct head));
  if(reception==NULL)
    return -1; 
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
        int lu = read(sfd, buf, sizeof buf);
        int err2 =  checkReceive(buf, lu, reception);
        // un ack concernant ce paquet a deja ete recu --> renvoi du paquet qui a ete perdu
        if( err2 == -2)
        {
          struct node *temp = reception->liste;
          pkt_set_timestamp(temp->pkt, (uint32_t)time(NULL)); 
          char buf2[528];
          int longueur = 528;
          // longueur-post : nombre d'octets ecrits dans buf2
          err2 = pkt_encode(temp->pkt, buf2, (size_t*) &longueur);
          if(longueur == 0)
          {
            fprintf(stderr, "echec renvoi données encodage\n");
          }
          else
          {
            int sended = write(sfd, buf2, longueur);
            if(sended != longueur){
              fprintf(stderr, "Erreur lors de l'envoi\n");
            }
          }
        }

        // tous les éléments du fichier ont été envoyés et des ack concernant tous les paquets ont ete recus
        if(reception->liste == NULL && attendre ==1)
        {
          /* DEBUG */ fprintf(stderr, "sender : tous les elems du fichiers ont été envoyés, tous les acks ont été reçus\n");
            int nombre = prepareToSend(NULL, 0, buf, reception);
            if(nombre == 0) {
              fprintf(stderr, "erreur de prepareToSend\n");
            }
            uint32_t time_end = (uint32_t)time(NULL);    
            int sended = write(sfd, buf, nombre);
            if(sended != nombre){
              fprintf(stderr, "Erreur lors de l'envoi\n");
            }
            int end2 = 0;
            int count = 0;
            while( end2 == 0 && count < 5)
            {
              // a recu le ack
              if (ufds[0].revents & POLLIN)
              {
                if(checkReceive(buf,lu,reception) != 0){
                  memset((void*)buf, 0, 528);
                  /* DEBUG */ fprintf(stderr, "Ce n'est pas ce que j'attendais pour me fermer, j'attends un autre ack.\n");
                  count ++;
                }
                else{
                  end2 = 1;
                  /* DEBUG */ fprintf(stderr, "end2=1\n");
                }
              }
              else if((uint32_t)time(NULL) > time_end+5)
              {
                time_end = (uint32_t)time(NULL);
                sended = write(sfd, buf, nombre);
                if(sended != nombre){
                  fprintf(stderr, "Erreur lors de l'envoi\n");
                }
                count++;
              }
            }
            end = 1;
            /* DEBUG */ fprintf(stderr, "end = 1\n");
        }
      }

      // renvoyer les paquets lorsque time-out
      ptr = reception->liste;  
      while(ptr!=NULL)
      {
        if( (uint32_t)time(NULL) > (pkt_get_timestamp(ptr->pkt)+5)) 
        {
          /* DEBUG */ fprintf(stderr, "timeout du packet de seq : %d --> on réenvoie les données\n", pkt_get_seqnum(ptr->pkt));
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

      if(ufds[1].revents & POLLIN && window_dest > 0 && window > 0 && attendre == 0)  {
        memset((void*)payload, 0, 512); // make sure the struct is empty

        fprintf(stderr, "file = %d\n", file);
        int lu = read(file,payload, 512);
        if(lu == 0)
        {
          /* DEBUG */ fprintf(stderr, "J'ai vu que lu == 0\n");
          attendre = 1;

          if(reception->liste == NULL){
            fprintf(stderr, "Sender : Je vais envoyer un packet vide\n");
            int nombre = prepareToSend(NULL, 0, buf, reception);
            if(nombre == 0)
            {
              fprintf(stderr, "erreur de prepareToSend\n");
            }
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
  }
  free(reception);
  return 0;
}
