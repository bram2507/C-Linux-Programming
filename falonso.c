#ifndef ALONSO_H
#define ALONOSO_H
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>

    #include <sys/types.h>
    #include <sys/ipc.h>
    #include <sys/sem.h>
    #include <sys/shm.h>

    #include <sys/msg.h>
    #include <signal.h>
    #include <sys/wait.h> 

    #include <string.h>
    #include <ctype.h> 
    #include "falonso.h"
#endif

#define SIGINT_SIG 10
#define SEM_ARRAY  10
#define SHM_SEM    1
#define SPEED 60
#define STAR_RACE 2
#define SHM_LIMIT 300

#define SEM_V 3
#define SEM_H 4
#define INTERCEPT_V 5
#define INTERCEPT_H 6


typedef struct shmemory{
            int count;
            int semid; 
            int msqid;
            int shmid;
            int semH;
            int semV;
            char* buf;
}shMemory;


typedef struct type_message{
                 long int type;    
                 int pos;
                 ushort msg_lspid;		/* pid último msgsnd */
		         ushort msg_lrpid;		/* pid último msgrcv */
}type_message;

int gotSIGINT = 0;

void ipcrm          (int shmid, int semid, int msqid, char* buf);
int  semaphoreLight (shMemory shm,int,int);
int  semop_PV       (int semid, int index, int numop );
void sigintHandler  (int sn);
int inicio          ( char **argumentos);

int main(int argc, char* argv[]){

    int ret = 1; 
    int speed = 1; 

            ret   = atoi(argv[1]);
            speed = atoi(argv[2]);      

    // if (argc == 3)
    // {
    //     if( !inicio(argv) ){
    //     }else return (EXIT_FAILURE);
    // } else{
    //     printf("Error argumentos !!!");
    //     exit(EXIT_FAILURE);
    // }

    //Tratamiento de señales SIGINT y Correspondeintes
	sigset_t sigintonly, sigall;
	struct sigaction actionSIGINT;
    
	sigfillset(&sigall);
	sigfillset(&sigintonly);

    sigdelset (&sigintonly,SIGCHLD);
	sigdelset (&sigintonly,SIGINT);

	actionSIGINT.sa_handler = sigintHandler;
	actionSIGINT.sa_mask    = sigintonly;
	actionSIGINT.sa_flags   = 0;

    sigaction (SIGINT, &actionSIGINT, NULL );
	if ( sigprocmask ( SIG_BLOCK , &sigintonly , NULL ) == -1 )
    {
		perror ("main:sigprocmask:SIG_SETMASK");
		exit(EXIT_FAILURE);
	}     
    
    shMemory shm; 
    shm.count = 0;  
    
    //Reserva e Inicializacion Memoria Compartida
    int shmid = shmget(ftok ("bin/cat",999), sizeof(char)*301, 0777 | IPC_CREAT ); 
    shm.buf   = (char *) shmat (shmid, NULL, 0);
    
    //Reserva de Cola de Mensajes
    shm.msqid = msgget(ftok ("bin/ls",0600), IPC_CREAT | 0777);
    if ( shm.msqid == -1 ){
        perror("Error de creacion cola de mensajes");
        exit(EXIT_FAILURE);
    }      
    
    //Reserva de array de semaforos
    shm.semid = semget(ftok("bin/cat", 888),SEM_ARRAY,0600 | IPC_CREAT);
    
    for (int i = 0; i < SEM_ARRAY; i++)  semctl( shm.semid, i,  SETVAL, 1); // declare 

    ret = 100; //borrar por que este valor se le pasa por terminal, el uno es de la memoria
    semctl( shm.semid, STAR_RACE, SETVAL, ret);
    semctl( shm.semid, SEM_V, SETVAL, 1);
    semctl( shm.semid, SEM_H, SETVAL, 1);
    semctl( shm.semid, INTERCEPT_V, SETVAL, 0);
    semctl( shm.semid, INTERCEPT_H, SETVAL, 0);
    
   
    if (gotSIGINT)  ipcrm(shm.semid,shm.shmid,shm.msqid,shm.buf);

    //inicilizar cola de mensaje para el carril derecho  
    type_message message;
    for ( int i = 1; i <=135; i++)
    {
      message.type= i;
      message.pos = i;
      msgsnd (shm.msqid, (struct msgbuf *)&message, sizeof(message.pos), IPC_NOWAIT);
    }

    //Inicializar biblioteca
    inicio_falonso(speed, shm.semid, shm.buf);

    //Se revisa si se ha registrado la señal SIGINT
    if (gotSIGINT)
    {
        ipcrm(shm.semid,shm.shmid,shm.msqid,shm.buf);
        fprintf(stderr,"El programa ha finalizado correctamente\n\n");
        exit(EXIT_SUCCESS);
    }
    
    int color = -1;   
    int pid   = 0; 
    int desp  = 0;

    int typeMsg = 0;
    int auxLib  = 0;
    int carril  = CARRIL_DERECHO;

    for (int i = 0; i < ret; i++)
    {        
        color++;
        desp+=1; 

        if( color == 7) color = 0;
        switch (pid = fork()){
            case -1:
                    ipcrm(shm.semid,shm.shmid,shm.msqid,shm.buf);
                    fprintf(stderr, "Error al realizar fork() \n");
                    break;
            case 0: 
                //Child Process
                
                sigaction (SIGINT, &actionSIGINT, NULL );
                if (gotSIGINT)
                {
                    shmdt((char*)shm.buf);
                    exit(0);
                }
                
                //Recibo mensaje de mi posicion - desp
                msgrcv( shm.msqid, (void*)&message, sizeof(type_message)-sizeof(long), (int)desp, IPC_NOWAIT);    
                
                inicio_coche(&carril,&desp,color+16);
                semop_PV(shm.semid,STAR_RACE,-1);
                semop_PV(shm.semid,STAR_RACE,0);

                semop_PV(shm.semid,SHM_SEM,-1);
                  auxLib = desp;
                semop_PV(shm.semid,SHM_SEM,1);

                //Ciclo de Avance Carril Derecho
                while (1){

                    if (gotSIGINT)
                    {
                        shmdt((char*)shm.buf);
                        exit(0);
                    }  

                    velocidad(SPEED, carril, desp);

                    semop_PV(shm.semid,SHM_SEM,-1);
                        if ( auxLib == 136) 
                        {
                          avance_coche(&carril, &desp, color+16);
                          message.type= 1;
                          message.pos = 1;
                          msgsnd (shm.msqid, (struct msgbuf *)&message, sizeof(message.pos), IPC_NOWAIT);  
                          auxLib = 0;
                        }
                    semop_PV(shm.semid,SHM_SEM,1);
                                     
                    
                    if (gotSIGINT)
                    {
                      shmdt((char*)shm.buf);
                      exit(0);
                    }

                 
                    if( auxLib == 105 ) {
                         
                        if( shm.buf[274] == ROJO || shm.buf[274] == AMARILLO){
                           
                            semop_PV(shm.semid, INTERCEPT_H,1);
                            semop_PV(shm.semid, INTERCEPT_H,0);
                            //semop_PV(shm.semid, SEM_H,1);
                        }
                     }

                     if ( auxLib == 20){
                           
                          if ( shm.buf[275] == ROJO || shm.buf[275] == AMARILLO){
                              semop_PV(shm.semid, INTERCEPT_V,1);
                              semop_PV(shm.semid, INTERCEPT_V,0);
                              //semop_PV(shm.semid, SEM_V,1);
                          }
                     }
                           
                    if (auxLib+1 == 21){
                       semop_PV(shm.semid, SEM_V,-1);
                    }else if (auxLib-4 == 21) semop_PV(shm.semid, SEM_V,1);
                    
                    if (auxLib+1 == 106){
                       semop_PV(shm.semid, SEM_H,-1);
                    }else if (auxLib-4== 106) semop_PV(shm.semid, SEM_H,1);

                    typeMsg = auxLib+1;
                    if ( msgrcv( shm.msqid, (void*)&message, sizeof(type_message)-sizeof(long), (int)typeMsg, 0) != -1 )
                    {
                        if (gotSIGINT)
                        {
                          shmdt((char*)shm.buf);
                          exit(0);
                        }

                        if (message.pos == typeMsg)
                        { 
                          avance_coche(&carril, &desp, color+16);

                          message.type= auxLib;
                          message.pos = auxLib;
                          msgsnd (shm.msqid, (struct msgbuf *)&message, 
                                                   sizeof(message.pos), 
                                                           IPC_NOWAIT);

                          if (gotSIGINT)
                          {
                            shmdt((char*)shm.buf);
                            exit(0);
                          }

                          if (auxLib == 1)
                          {
                            message.type= 136;
                            message.pos = 136;
                            msgsnd (shm.msqid, (struct msgbuf *)&message, sizeof(message.pos), IPC_NOWAIT);
                          }                                         

                          auxLib++;
                          if (gotSIGINT){
                              shmdt((char*)shm.buf);
                              exit(0);
                          }

                        }
                    }
                }
                
                if (gotSIGINT)
                {     
                    shmdt((char*)shm.buf);
                    exit(0);
                }
                    break;
            default: 
                    break;
        }
    }
     int    semH = VERDE;
     int    semV = ROJO;
    while(1)
    {
        if (gotSIGINT) break;
        semaphoreLight(shm, semH, semV);
    }
   
     //for (int i = 0; i < ret; i++) pid = wait(NULL);
   
    shm.count = 0; // quitar para cada vez que pase por meta  
    fin_falonso(&shm.count);
  
    if (gotSIGINT) ipcrm(shm.semid,shm.shmid,shm.msqid,shm.buf);
    else ipcrm(shm.semid,shm.shmid,shm.msqid,shm.buf);
      
    fprintf(stderr,"El programa ha finalizado correctamente\n\n");

    return EXIT_SUCCESS;
}

int inicio( char **argumentos)
{
    if( (atoi(argumentos[1]) < 2 || atoi(argumentos[1]) > 135)){
        fprintf(stderr,"Primer argumento entre 2 y 135\n");
        return 1;
    } 

    if( argumentos[2][0] != 'N' && argumentos[2][0] != 'R'){
        fprintf(stderr,"Segundo argumento R o N\n");
         return 1;
    }
return 0;
}

void ipcrm(int semid, int shmid, int msqid,char* buf){
    shmdt((char*)buf);
    shmctl(shmid,IPC_RMID,NULL);
    msgctl(msqid,IPC_RMID,NULL);    
    semctl(semid,IPC_RMID,0);
}

int semaphoreLight(shMemory shm, int semH, int semV){   
    
    luz_semAforo(HORIZONTAL, semH);
    luz_semAforo(VERTICAL,   semV);
 
        if( shm.buf[274] == VERDE){
           
            // semop_PV(shm.semid, SEM_H, 1);
            semctl( shm.semid, SEM_H, SETVAL, 1);
            semctl( shm.semid, INTERCEPT_H, SETVAL, 0);
           
        }
   
        if( shm.buf[275] == VERDE){
          
            semctl( shm.semid, SEM_V, SETVAL, 1);
            semctl( shm.semid, INTERCEPT_V, SETVAL, 0);
           
        }
   
    sleep(3);

    semH++;
    semV++;
    if( semH ==4) semH = 1;
 
    if( semV ==4) semV = 1;

    if (gotSIGINT) return SIGINT_SIG;
        else semaphoreLight(shm, semH, semV);
   
    return EXIT_SUCCESS;
}


int semop_PV (int semid, int index , int numop ){            
      struct sembuf op_p;  

      op_p.sem_num = index;    /** indice en el array de semafros **/
      op_p.sem_op  = numop;   /** un wait (resto 1)  signal sumo 1**/
      op_p.sem_flg =  0;       /** flag **/
      return semop( semid, &op_p, 1 );    
}

void sigintHandler  (int sn){ gotSIGINT = 1; }