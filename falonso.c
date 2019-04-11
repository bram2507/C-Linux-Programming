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
#define SPEED 2
#define STAR_RACE 2
#define SHM_LIMIT 300

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
int  semaphoreLight (shMemory shm);
int  semop_PV       (int semid, int index, int numop );
void sigintHandler  (int sn);

int main(int argc, char* argv[]){

    int ret = 1; 
    int speed = 1; 
    
    if (argc == 3)
    {
       ret = atoi(argv[1]);
       speed = atoi(argv[2]);
    } else{
        printf("Error argumentos !!!");
        exit(EXIT_SUCCESS);
    }

    //Tratamiento de señales SIGINT y Correspondeintes
	sigset_t sigintonly, sigall;
	struct sigaction  actionSIGINT;
    
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
        fprintf(stderr,"\n msgIdget failed \n");
        exit(EXIT_FAILURE);
    }      
    
    //Reserva de array de semaforos
    shm.semid = semget(ftok("bin/cat", 888),SEM_ARRAY,0600 | IPC_CREAT);
    for (int i = 0; i < SEM_ARRAY; i++)  semctl( shm.semid, i,  SETVAL, 1); // declare 

    ret = 1; //borrar por que este valor se le pasa por terminal
    semctl( shm.semid, STAR_RACE,  SETVAL, ret);
    semctl( shm.semid, 3,  SETVAL, 0);
   
    if (gotSIGINT)  ipcrm(shm.semid,shm.shmid,shm.msqid,shm.buf);

    //inicilizar cola de mensaje para el carril derecho  
    type_message message;
    for ( int i = 1; i <=136; i++)
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
        fprintf(stderr,"El progrma ha finalizado correctamente\n\n");
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
    
   
    for (int i = 0; i < ret; i++) pid = wait(NULL);
   
    shm.count = 0; // quitar para cada vez que pase por meta  
    fin_falonso(&shm.count);
  
    if (gotSIGINT) ipcrm(shm.semid,shm.shmid,shm.msqid,shm.buf);
    else ipcrm(shm.semid,shm.shmid,shm.msqid,shm.buf);
      
    fprintf(stderr,"El progrma ha finalizado correctamente\n\n");

    return EXIT_SUCCESS;
}

void ipcrm(int semid, int shmid, int msqid,char* buf){
    shmdt((char*)buf);
    shmctl(shmid,IPC_RMID,NULL);
    msgctl(msqid,IPC_RMID,NULL);    
    semctl(semid,IPC_RMID,0);
}

int semaphoreLight(shMemory shm){   
    shm.semH = VERDE; shm.semV = ROJO;

    if (gotSIGINT) return SIGINT_SIG;
    luz_semAforo(HORIZONTAL, shm.semH);
    luz_semAforo(VERTICAL,   shm.semV);
    sleep(10);//pausa();

    if (gotSIGINT) return SIGINT_SIG;
    shm.semH = AMARILLO; shm.semV = VERDE;
    luz_semAforo(HORIZONTAL, shm.semH);
    luz_semAforo(VERTICAL,   shm.semV);
    sleep(10);

    if (gotSIGINT) return SIGINT_SIG;
    shm.semH = ROJO; shm.semV = AMARILLO;
    luz_semAforo(HORIZONTAL, shm.semH);
    luz_semAforo(VERTICAL,   shm.semV);  
    sleep(10);

    if (gotSIGINT) return SIGINT_SIG;
    shm.semH = VERDE; shm.semV = ROJO;
    luz_semAforo(HORIZONTAL, shm.semH);
    luz_semAforo(VERTICAL,   shm.semV);
    sleep(10);

    if (gotSIGINT) return SIGINT_SIG;
  
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