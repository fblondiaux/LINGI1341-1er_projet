#include <sys/poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h> /* * sockaddr_in6 */
#include <sys/types.h>
#include <getopt.h>
#include "receiver.h"

/*
* Code réalisé par :
* Noemie verstraete - 25021500
* Florence Blondiaux - 06521500
* Version du 05.11.17
*/

int main(int argc, char * argv[]) {
  if(argc < 3){
    fprintf(stderr, "Pas assez d'arguments, donnez au minimum hostname et port.\n");
    return EXIT_FAILURE;
  }
  //int opt, file = -1;
  int opt = -1;
  FILE* fileF = NULL;

  // Recuperation des arguments
  while ((opt = getopt(argc, argv, "f:")) != -1) {
    switch (opt) {
        case 'f':
          fileF = fopen(optarg, "w+");
          if(fileF == NULL){
            fprintf(stderr, " Echec lors de l'utilisation de fopen\n");
            return EXIT_FAILURE;
          }
          break;
      default:
        fprintf(stderr, "L'option -f nécessite un nom de fichier.\n");
        break;
    }
  }

  // Récuperation des arguments non optionnels.

  char* host = argv[optind];
  int port = atoi(argv[optind + 1]);
  int file =0;

  if(fileF == NULL){
    file = STDIN_FILENO;
  }
  else{
    file = fileno(fileF);
  }
  // Transformation de l'adresse en une adresse utilisable par le programme
  struct sockaddr_in6 addr;
  const char *err = real_address(host, &addr);
  if (err) {
    fprintf(stderr, "Could not resolve hostname %s: %s\n", host, err);
    if(file != STDIN_FILENO){
      close(file);
    }
    return EXIT_FAILURE;
  }

  // Création du socket.
  int sfd = create_socket(&addr, port, NULL, -1);
  if(sfd == -1){
    fprintf(stderr, "Echec lors de la création du socket\n");
    return EXIT_FAILURE;
  }
  if (wait_for_client(sfd) < 0){ /* Connected */
    fprintf(stderr, "Could not connect the socket after the first message.\n");
      if(file != STDIN_FILENO){
        close(file);
      }
      close(sfd);
      return EXIT_FAILURE;
  }

    receptionDonnes(sfd, file);

    //Le transfert est terminé, fermeture.
    if(file != STDIN_FILENO){
      close(file);
    }

    close(sfd);

    return EXIT_SUCCESS;

  }
