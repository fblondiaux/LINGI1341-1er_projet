#include <sys/poll.h>
#include <stdlib.h>
#include <stdio.h>
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
  struct pollfd ufds[2];
  // Eventuellement checker si ça donne une erreur.
  ufds[0].fd = sfd;
  ufds[0].events = POLLIN; // check for normal or out-of-band

  ufds[1].fd = STDIN_FILENO;
  ufds[1].events = POLLOUT; // check for just normal data
  int rv = poll(ufds, 2, -1); // Si apres 5 secondes pas de data il plante.
  if (rv == -1) {
    fprintf(stderr, "Error lors de l'utilisation de poll\n");
    //perror("poll"); // error occurred in poll()
  }
  else {
    if (ufds[0].revents & POLLIN) {
      char buf1[1024];
      //Eventuellement verifier le malloc
      int recu = read(sfd, buf1, sizeof buf1); // receive normal data
      int ecrit = write(STDOUT_FILENO, buf1, recu);
      if(ecrit != recu){
      }
    }
    // check for events on s2:
    if (ufds[1].revents & POLLOUT) {
      char buf2[1024];
      int lu = read(STDIN_FILENO,buf2,1024);
      int sended = write(sfd,buf2,lu);
      if(sended != lu){
        fprintf(stderr, "Erreur lors de l'envoie\n");
      }
    }
  }
}
