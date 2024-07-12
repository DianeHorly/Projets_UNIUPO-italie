#include "smallsh.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>



char prompt[50]; // "Scrivere un comando>";

void procline(void) 	/* tratta una riga di input */
{

  char *arg[MAXARG+1];	/* array di puntatori per runcommand */
  int toktype;  	/* tipo del simbolo nel comando */
  int narg;		/* numero di argomenti considerati finora */
	int type;   // indica la modalità background o foreground
  narg=0;

  
  int exitstat;
  pid_t pid;
  

  do {

    /* mette un simbolo in arg[narg] 
       ed esegue un'azione a seconda del tipo di simbolo */
	
    switch (toktype = gettok(&arg[narg])) {
	
      case ARG:   

        /* se argomento: passa al prossimo simbolo */
		
        if (narg < MAXARG) narg++;
	break;
       

      case EOL:
      case SEMICOLON:

      /* caso per riconoscere il background e il foreground. */
      case AMPERSAND:
          if(toktype==AMPERSAND){
             type=BACKGROUND;
             
             /*stampa lo stato di terminazione di un processo eseguito in background */
             while((pid = waitpid(-1,&exitstat,WNOHANG)) > 0 ){
                printf("\n\til processo %d in background ",pid);
                
                if(WIFEXITED(exitstat))
                {
                   printf("è terminato normalmente\n");
          
                }
                if(WIFSIGNALED(exitstat))
                {
                   printf("è stato terminato dal segnale %d\n",WTERMSIG(exitstat));
            
                }
             } 
          }
          else
             type=FOREGROUND;
          
      
         /* se fine riga o ';' esegue il comando ora contenuto in arg,
	 mettendo NULL per indicare la fine degli argomenti: 
         serve a execvp */

        if (narg != 0) {
	  arg[narg] = NULL;
	  runcommand(arg,type);
        }
      

	/* se non fine riga (descrizione comando finisce con ';')
           bisogna ricominciare a riempire arg dall'indice 0 */

        if (toktype != EOL)
         
         narg = 0; 

        break;
     }
  }

  while (toktype != EOL);  /* fine riga, procline finita */
  
}




void runcommand(char **cline, int modo)	/* esegue un comando */
{
  pid_t pid;
  int exitstat,ret;
  struct sigaction act;  // structura sigaction per i segnali  
  
  pid = fork();
  if (pid == (pid_t) -1) {
     perror("smallsh: fork fallita");
     return;
  }

  if (pid == (pid_t) 0) { 	/* processo figlio */

    /* esegue il comando il cui nome e' il primo elemento di cline,
       passando cline come vettore di argomenti */

    execvp(*cline,cline);
    perror(*cline);
    exit(1);
  }

  /* non serve "else"... ma bisogna aver capito perche' :-) : 
  non serve else perché il processo figlio chiama la execvp () e exit()  */

 
  /* qui aspetta sempre e comunque - i comandi in background 
     richiederebbero un trattamento diverso */



  if(modo==FOREGROUND){
    
    /*permette all'interprete di ignorare il segnale di interruzione quando c'è un processo
    in esecuzione in foreground */
    act.sa_handler=SIG_IGN;
    sigemptyset(&act.sa_mask);     
    act.sa_flags=0;                 
    sigaction(SIGINT, &act, NULL);

    
    ret = waitpid(pid,&exitstat,0);

    if (ret == -1) perror("errore waitpid");

    /* stampa lo stato di terminazione di un processo in foreground */
    if(WIFSIGNALED(exitstat))
          printf("\n\til processo %d in foreground è terminato dal segnale %d\n\n",ret,WTERMSIG(exitstat) );
    else{
       if(WIFEXITED(exitstat))
          printf("\til processo %d in foreground è terminato normalmente \n\n",ret);   
    }

  } 
  
  else//BACKGROUND
      {

        printf("\t ...processo con pid %d va in background...\n\n",pid );
        
      }


//ripristino il comportamento di default di un segnale alla fine di ogni ciclo
 act.sa_handler=SIG_DFL;
 sigemptyset(&act.sa_mask);
 act.sa_flags=0;
 sigaction(SIGINT, &act, NULL);
}


int main()
{

  while(userin(prompt) != EOF)
    procline();
  return 0;
}
