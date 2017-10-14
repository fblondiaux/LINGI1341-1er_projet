
/* Loop reading a socket and printing to stdout,
 * while reading stdin and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as stdin signals EOF
 */
void read_write_loop(const int sfd){
  // COde issus du cours de Systeme informatique
  // En vrai il faut que j'approfondisse la fonction poll peut-etre pas besoin de processus :)
  

// COmment signaler si la fonction foire ?

  int status;
  pid_t pid;

  pid=fork();

  // if (pid==-1) {
  //   // erreur à l'exécution de fork
  //   perror("fork");
  //   exit(EXIT_FAILURE);
  // }
  // pas d'erreur
  if (pid==0) {
    //Processus fils
    // Lis les données
    int err = recv(sfd, un moyen d'envoyer sur la sortie standantd )
  }
  else {
    // processus père
    int fils=waitpid(pid,&status,0);
    if(fils==-1) {
      perror("wait");
      exit(EXIT_FAILURE);
    }
    if(WIFEXITED(status)) {
      printf("Le fils %d s'est terminé correctement et a retourné la valeur %d\n",fils,WEXITSTATUS(status));
      return(EXIT_SUCCESS);
    }
    else {
      if( WIFSIGNALED(status)) {
	printf("Le fils %d a été tué par le signal %d\n",fils,WTERMSIG(status));
      }
      return(EXIT_FAILURE);
    }
  }
}
