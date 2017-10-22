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

int main(int argc, char * argv[]) {
  if(argc < 3){
    fprintf(stderr, "Pas assez d'arguments, donnez au minimum hostname et port.\n");
    return EXIT_FAILURE;
  }
  //int opt, file = -1;
  int opt = -1;
  FILE* file = NULL;

  // Recuperation des arguments
  while ((opt = getopt(argc, argv, "f:")) != -1) {
    switch (opt) {
        case 'f':
          //FILE* temp = fopen(optarg, "w");
          //if (temp == NULL){
          file = fopen(optarg, "w");
          if(file == NULL){
            fprintf(stderr, " Echec lors de l'utilisation de fopen\n");
            return EXIT_FAILURE;
          }
          //file = fileno(temp);  // Ouvre le fichier, si il n'existe pas on essaye de le creer.
          // if(file == -1){
          //   fprintf(stderr, " Echec lors de l'utilisation de fileno\n");
          //   return EXIT_FAILURE;
          // }
          if(file == NULL){
            fprintf(stderr, " Echec lors de l'utilisation de fopen.\n");
            return EXIT_FAILURE;
          }
          break;
      default:
        fprintf(stderr, "L'option -f nécessite un nom de fichier.\n");
        break;
    }
  }

  //QF: Les arguments arriveront tjs dans l'ordre hostname et puis port je suppose ?
  // N : oui je crois
  // Récuperation des arguments non optionnels.

  char* host = argv[optind];
  int port = atoi(argv[optind + 1]);
  printf("Arguments bien recus\n");

  // Transformation de l'adresse en une adresse utilisable par le programme
  struct sockaddr_in6 addr;
  const char *err = real_address(host, &addr);
  if (err) {
    fprintf(stderr, "Could not resolve hostname %s: %s\n", host, err);
    if(file != NULL){
      fclose(file);
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
    fprintf(stderr,
      "Could not connect the socket after the first message.\n");
      if(file != NULL){
        fclose(file);
      }
      close(sfd);
      return EXIT_FAILURE;
  }

    receptionDonnes(sfd, file);

    if(file != NULL){
      fclose(file);
    }

    close(sfd);

    return EXIT_SUCCESS;

  }
