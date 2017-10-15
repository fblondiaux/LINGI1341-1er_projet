#include <sys/poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "read_write_loop.h"
/* Loop reading a socket and printing to stdout,
* while reading stdin and writing to the socket
* @sfd: The socket file descriptor. It is both bound and connected.
* @return: as soon as stdin signals EOF
*/
void read_write_loop(const int sfd){
  // COde issus du cours de Systeme informatique
  struct pollfd ufds[2];
  // Eventuellement checker si ça donne une erreur.
  ufds[0].fd = sfd;
  ufds[0].events = POLLIN; // check for normal or out-of-band

  ufds[1].fd = sfd;
  ufds[1].events = POLLOUT; // check for just normal data
  int rv = poll(ufds, 2, 3500); // Si apres 3.5 secondes pas de data il plante.

  if (rv == -1) {
    //perror("poll"); // error occurred in poll()
  }
  else if (rv == 0) {
    //printf("Timeout occurred!  No data after 3.5 seconds.\n");
  }
  else {
    // check for events on s1:
    if (ufds[0].revents & POLLIN) {
      char buf1[1024];
      //Eventuellement verifier le malloc
      int recu = recv(sfd, buf1, sizeof buf1, 0); // receive normal data
      int ecrit = fwrite(buf1, 1, recu, stdout);
      if(ecrit != recu){
        fprintf(stderr, "Mauvaise ecriture sur stdout\n");
      }
    }
    // check for events on s2:
    if (ufds[1].revents & POLLOUT) {
      char buf2[1024];
      int lu = fread(buf2,1,1024,stdin);
      int sended = send(sfd,buf2,lu,0);
      if(sended != lu){
        fprintf(stderr, "Erreur lors de l'envoie\n");
      }
    }
  }
}
