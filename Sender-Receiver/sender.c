#include "real_address.h"
#include "create_socket.h"
#include "wait_for_client.h"
#include "packet_interface.h"
#include "envoieDonnes.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[]) {
  // Vérfication du nombre d'arguments. Doit être présent au miniimum :
  // Nom programme + hostname + port
  if(argc < 3){
    fprintf(stderr, "Pas assez d'arguments, donnez au minimum hostname et port.\n");
    return EXIT_FAILURE;
  }
  int opt, file = -1;
  // Recuperation des arguments
  while ((opt = getopt(argc, argv, "f:")) != -1) {
    switch (opt) {
      case 'f':
      File* temp = fopen(optarg, "w");
      if (temp == NULL){
        fprintf(stderr, " Echec lors de l'utilisation de fopen : %s\n", errno );
        return EXIT_FAILURE;
      }
      file = fileno(temp);  // Ouvre le fichier, si il n'existe pas on essaye de le creer.
      if(file == -1){
        fprintf(stderr, " Echec lors de l'utilisation de fileno : %s\n", errno );
        return EXIT_FAILURE;
      }
      break;
      default:
      fprintf(stderr, "L'option -f nécessite un nom de fichier.\n" );
      break;
    }
  }

  //QF: Les arguments arriveront tjs dans l'ordre hostname et puis port je suppose ?
  // Récuperation des arguments non optionnels.

  char* host = argv[optind];
  int port = atoi(argv[optind + 1]);

  // Transformation de l'adresse en une adresse utilisable par le programme
  struct sockaddr_in6 addr;
  const char *err = real_address(host, &addr);
  if (err) {
    fprintf(stderr, "Could not resolve hostname %s: %s\n", host, err);
    if(file != -1){
      fclose(file);
    }
    return EXIT_FAILURE;
  }

  // Création du socket.
  int sfd = create_socket(NULL, -1, &addr, port);
  if(sfd = -1){
    fprintf(stderr, "Echec lors de la création du socket\n");
    return EXIT_FAILURE;
  }


  int err = envoieDonnes(addr, sfd, file);

  if(file != -1){
    fclose(file);
  }

  close(sfd);

  return err;
}
