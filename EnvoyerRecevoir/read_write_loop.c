#include <sys/poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "read_write_loop.h"
/* Loop reading a socket and printing to stdout,
* while reading stdin and writing to the socket
* @sfd: The socket file descriptor. It is both bound and connected.
* @return: as soon as stdin signals EOF
*/
void read_write_loop(const int sfd){
  // COde issus du cours de Systeme informatique

    char buf1[1024];
    char buf2[1024];
    int end = 0;
  struct pollfd ufds[2];
  // Eventuellement checker si ça donne une erreur.
  ufds[0].fd = sfd;
  ufds[0].events = POLLIN; // check for normal

  ufds[1].fd = STDIN_FILENO;
  ufds[1].events = POLLIN; // check for just normal data
  while(end == 0 ){
    memset((void*)buf1, 0, 1024); // make sure the struct is empty
    memset((void*)buf2, 0, 1024);
    int rv = poll(ufds, 2, 5000);
    if(rv==0){
      fprintf(stderr, "Time out\n");
      return;
    }
    if (rv == -1) {
      fprintf(stderr, "Error lors de l'utilisation de poll\n");
      //perror("poll"); // error occurred in poll()
      return ;
    }
    else {
      if (ufds[0].revents & POLLIN) {
        //Eventuellement verifier le malloc
        printf("serveur passe ici poll in\n");
        int recu = read(sfd, buf1, sizeof buf1); // receive normal data
        printf("Apres read avant write\n");
        int ecrit = write(STDOUT_FILENO, buf1, recu);
        if(ecrit != recu){
        }
      }
      // check for events on s2:
      if(ufds[1].revents & POLLIN)  {
        printf("Serveur passe ici pollout\n" );
        int lu = read(STDIN_FILENO,buf2,1024);
        printf("Apres read\n");
        int sended = write(sfd,buf2,lu);
        if(sended != lu){
          fprintf(stderr, "Erreur lors de l'envoie\n");
        }
      }
    }
    end = feof(stdin);
  }
}
