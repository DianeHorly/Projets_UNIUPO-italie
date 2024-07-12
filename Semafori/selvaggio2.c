#include<semaphore.h>
#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>

sem_t piena;  // indica pentola piena
sem_t mutex;  // mutex serve per accedere alla variabile 'PORZIONI' esclusivamente in mutua esclusione
sem_t vuota;  // indica pentola vuota
int shmid; 
int shmid2;   //per i semafori posix da condividere con i processi
int NGIRI;
int SELVAGGI;
int PORZIONI;
int CONT=0; // conta quante volte il cuoco avrà riempito la pentola al termine del programma 

int *buf; //un buffer che permette di condividere le variabili tra i processi


sem_t *buf2;      //un buffer per condividere i semafori posix tra i processi


//struct sigaction sa;


void cuoco(){
     int porzioniI=buf[0];
     int m; 
      
     m = 50000000 + 30000000 * 10;
     
     while(1){

        /*//***** a cosa serve la sem_wait??? 
        //*** =decrementa il valore associato al semaforo fino a quando il valore del semaforo
        diventa negativo e a questo punto se un processo arriva aspetta fino a quando 
        il valore del semaforo diventa maggiore a zero cioè quando si fa la sem_post******/

         sem_wait(&buf2[1]); // il cuoco viene svegliato da un selvaggio
         // la pentola è vuota, il cuoco la riempe
         
         //buf[2] contiene la variabile condivisa SELVAGGI
         if(buf[2]==0){ exit(0);}   // se tutti i selvaggi hanno finito il loro ciclo viene chiamata la funzione exit(0) per uscire dal thread 'cuoco'
         buf[1]++;  //buf[1] contiene la variabile condivisa CONT
         
         printf("\n\t\t\t...Il cuoco sta riempendo la pentola....\n\n"); 
                    
         //buf[0] contiene la variabile condivisa PORZIONI           
         while(buf[0]<porzioniI){ // riempo la pentola fino a M porzioni
               buf[0]++;
         }

         for(int i=0; i<m; i++); // tempo per cui il cuoco impiega a riempire la pentola
           
         printf("\n\t\t\t...la pentola è piena\n\n");         
          
         sem_post(&buf2[0]);
         //exit(0);
     }
}

void selvaggio(int cont){
      int m, n;
      int i=cont;
      // ogni selvaggio usa un tempo diverso per fare le sue azioni
      m = 50000000; // tempo per cui il selvaggio prende la porzione
      n = 500000000 ; // tempo per cui il selvaggio mangia
      for(int k=1; k<=NGIRI; k++){
          printf("Il selvaggio %d sta pensando per la  %d° volta\n", i, k);   
          // controllo se ci sono porzioni
          sem_wait(&buf2[2]);
          if(buf[0]==0){ // se non ci sono porzioni
              printf("Il selvaggio %d sveglia il cuoco e aspetta\n", i);  
              sem_post(&buf2[1]); //il selvaggio sveglia il cuoco bloccato in attesa che la pentola sia vuota
              sem_wait(&buf2[0]); //si ferma finchè la pentola non è piena
          }
          
          
          buf[0]--;     //decremento il numero di porzioni

          printf("Il selvaggio %d ha preso una porzione per la - %d° volta\t", i, k);
           
          printf("%d porzioni rimaste\n", buf[0]);
          
          for(int j=0; j<m; j++); // ogni selvaggio usa le risorse per un tempo diverso
          sem_post(&buf2[2]); 
          
          printf("Il selvaggio %d sta mangiando per la - %d° volta\n", i, k);
          
          for(int j=0; j<n; j++);// ogni selvaggio mangia per un tempo diverso
      }
    //*** se tutti i selvaggi hanno conpiuto il loro n° di giri e poi sveglia il cuoco
    //*** il cuoco al suo turno finisce l'esecuzione del suo programma e fa la exit() 
      if((--buf[2])==0) sem_post(&buf2[1]);
     
     //exit(0);
}


void clear(int s)	/* rimuove strutture IPC */
{

if (sem_destroy(&piena) == -1) perror("sem_destroy");
if (sem_destroy(&mutex) == -1) perror("sem_destroy");
if (sem_destroy(&vuota) == -1) perror("sem_destroy");


if (shmctl(shmid,IPC_RMID,0) == -1) perror("shmctl");
if (shmctl(shmid2,IPC_RMID,0) == -1) perror("shmctl");
printf("ho cancellato i semafori e la memoria condivisa\n");

exit(s);
}



int main(int argc, char *argv[]){
      if(argc!=4){
          printf("numero di argomenti insufficiente: usare %s <arg1=n° di selvaggi> <arg2=n° porzioni> <arg3=n° NGIRI>\n",argv[0]);
          exit(1);
      }
     
     pid_t n;
     pid_t x;
     struct sigaction sa;

      /* sto passando il numero di selvaggi,porzioni e giri alla linea di comando */
      SELVAGGI=atoi(argv[1]);// la variabile 'SELVAGGI' indica il numero dei selvaggi
      PORZIONI=atoi(argv[2]);// la variabile 'PORZIONI' indica le porzioni massime che può contenere la pentola
      NGIRI=atoi(argv[3]);// la variabile 'NGIRI' indica per quante volte il selvaggio mangia 
      int selvaggiI=SELVAGGI;
           
     
 //inizializzo i semafori    
      
sem_init(&piena, 1, 0);
sem_init(&vuota, 1, 0);
sem_init(&mutex, 1, 1);
     

//inizializzo la memoria condivisa per le variabili
if ((shmid = shmget(IPC_PRIVATE,sizeof(int)*3,0600))==-1)
     perror("shmget");

//condivido il buffer e inserisco le variabili che devono essere condivise
buf= (int *) shmat(shmid,0,0);
buf[0]= PORZIONI;
buf[1]=CONT;      //*** n° di volta che le cuoco riempie la pentola
buf[2]=SELVAGGI;  //***definito per specificare che tutti i selvaggi hanno conpiuto il loro n° di giri  



//inizializzo la memoria condivisa2 per i semafori
if ((shmid2 = shmget(IPC_PRIVATE,sizeof(sem_t)*3,0600))==-1)
     perror("shmget");
 
      
//condivido il buffer e inserisco le variabili che devono essere condivise
buf2= (sem_t *) shmat(shmid2,0,0);
buf2[0]=piena;
buf2[1]=vuota;  
buf2[2]=mutex; 
      
//inizializzo i semafori    
      
sem_init(&buf2[0], 1, 0);
sem_init(&buf2[1], 1, 0);
sem_init(&buf2[2], 1, 1);  

 
 
   if (buf == (void *)-1) 
   	   perror("shmat");

     
    n=fork();
     
    //genero il cuoco 
    if(n==0) 
    {
    cuoco();
    exit(0);
    
    }
    
     
      
      //genero i selvaggi
    for(int i=1; i<=selvaggiI; i++){
         //if(fork()==0){
         
        x=fork();
        if(x==0)
        {
         
         selvaggio(i);
         
         exit(0);
        }
    }
      
      
// le devo definire dopo i processi perché se no ogni processi tenta di cancellare dei semafori già cancellati
sa.sa_handler = clear;
sigaction(SIGINT,&sa,NULL);   
      
      
      
      //aspetto il cuoco
      waitpid(n,NULL,0);
        printf("\nTerminato il cuoco %d\n",n);
      
      //aspetto i selvaggi
      for(int j=0;j<SELVAGGI;j++){
        x=wait(NULL); 
        printf("Terminato il selvaggio %d\n",x);
      }
      
      
      printf("\nIl cuoco ha riempito la pentola %d volte\n\n", buf[1]);
      
      clear(0);
}
