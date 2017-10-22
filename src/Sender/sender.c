// #include "../EnvoyerRecevoir/real_address.h"
// #include "../EnvoyerRecevoir/create_socket.h"
// #include "../EnvoyerRecevoir/wait_for_client.h"
// #include "../FormatSegments/packet_interface.h"
// #include "envoieDonnes.h"
#include "sender.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>



int main(int argc, char *argv[]) {
  // Vérfication du nombre d'arguments. Doit être présent au miniimum :
  // Nom programme + hostname + port
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

  printf("sender : 1\n");
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

  printf("sender : 2\n");
  // Création du socket.
  int sfd = create_socket(NULL, -1, &addr, port);
  if(sfd == -1){
    fprintf(stderr, "Echec lors de la création du socket\n");
    return EXIT_FAILURE;
  }

  printf("sender : 3\n");
  envoieDonnes(sfd, file);

  printf("sender : 4\n");
  if(file != NULL){
    fclose(file);
  }

  close(sfd);

  return EXIT_SUCCESS;
}
