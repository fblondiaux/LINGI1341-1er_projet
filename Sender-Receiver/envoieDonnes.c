#include <sys/poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

int envoieDonnes(sockaddr_in6 addr, int sfd, int file){
  // Preparation
  if(file == -1){
    file = STDIN_FILENO;
  }
  char buf[528];
  char payload[512];
  int end = 0;
  struct pollfd ufds[2];
  ufds[0].fd = file;
  ufds[0].events = POLLIN; // check for normal data
  ufds[1].fd = file;
  ufds[1].events = POLLIN; // check for normal data

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
        //Eventuellement verifier le malloc
        int recu = read(sfd, buf, sizeof buf); // receive normal data
        checkReceive(buf);
      }
      // traite l'ecriture
      if(ufds[1].revents & POLLIN)  {

        memset((void*)payload, 0, 512); // make sure the struct is empty
        int lu = read(file,payload, 512);
        prepareToSend(payload, lu, buf);
        int sended = write(sfd,buf,lu);
        if(sended != lu + 16){
          fprintf(stderr, "Erreur lors de l'envoi\n");
        }
      }
    }
    end = feof(file);
  }
}


int checkReceive(char* buf);


int prepareToSend(char* payload, int taillePayload, char* toSend);
