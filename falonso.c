///////////////////////////////////////////////////////////////
// Practica de SSOOII - Universidad de Salamanca             //
//                      Escuala Politecnica de Zamora        //
//                      Grado en Ingenieria                  //
//                                                           //   
// Titulo             - Un día en las carreras (de coches)   //
//                                                           //
// Autor  - Abraham Alvarez Gomez                            //
// DNI    - 79257294F                                        //
// Curso  - 2018/2019                       ///////////////////
////////////////////////////////////////////

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
    #include <math.h>
    #include <time.h>
    #include "falonso.h"
#endif

#define SIGINT_SIG 30
#define SEM_ARRAY  7
#define SHM_SEM    1
#define SPEED 100
#define STAR_RACE 2
#define SHM_LIMIT 300


#define SEM_V 3
#define INTERCEPT_V 4 
#define INTERCEPT_H 5 
#define LAP_COUNT   6   
#define SEMLIMIT    32767

typedef struct shmemory{
            int count;
            int semid; 
            int msqid;
            int shmid;
            char* buf;
            int cambioD;
            int cambioI;
}shMemory;


typedef struct type_message{
                 long int type;    
                 int pos;
}type_message;

int gotSIGINT = 0;

void ipcrm          (int shmid, int semid, int msqid);
int  semaphoreLight (shMemory shm,int,int);
int  semop_PV       (int semid, int index, int numop );
void sigintHandler  (int sn);
int inicio          ( char **argumentos);

int main(int argc, char* argv[]){

    srand(time(NULL));
    int ret = 1; 
    int speed = 1; 

            ret   = atoi(argv[1]);
            speed = atoi(argv[2]);  
    
    ret = 20;
	sigset_t sigintonly, sigall;
	struct   sigaction actionSIGINT;
    
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
    
    shMemory shm;  //Estructura general con identificadores de los IPCs y variables útiles
    shm.count = shm.cambioD = shm.cambioI = 0;  
   
    int shmid = shmget(IPC_PRIVATE, sizeof(char)*SHM_LIMIT, IPC_CREAT | 0600 );  //Reserva e Inicializacion Memoria Compartida
    shm.buf   = (char *) shmat (shmid, NULL, 0);  // Se vincula la memoria compartida en espacio del SO con la memoria de nuestro programa 
    char *buf = shm.buf;
    
    shm.shmid = shmid;

    int msqid = msgget(ftok ("bin/ls",0600), IPC_CREAT | 0600); //Reserva de Cola de Mensajes 
    if ( msqid == -1 )
    {
        perror("Error de creacion cola de mensajes"); //Borrar los recurso reservados hasta el
        exit(EXIT_FAILURE);                           //momento , no esta implementado
    }      
    shm.msqid = msqid;

    int semid = semget(ftok("bin/cat", 888),SEM_ARRAY,0600 | IPC_CREAT); 
    shm.semid = semid;
    for (int i = 0; i < SEM_ARRAY; i++)  semctl( semid, i,  SETVAL, 1); 

    semctl( semid, STAR_RACE, SETVAL, ret); 
    semctl( semid, SEM_V, SETVAL, 1);             
    semctl( semid, INTERCEPT_V, SETVAL, 0); 
    semctl( semid, INTERCEPT_H, SETVAL, 0); 
    semctl( semid, LAP_COUNT, SETVAL, 0);
   
    type_message message;           //recibo 1 -> 136 recibe el primer mensaje que te encuentre
    for ( int i = 1; i <=135; i++)  //0 - 136 [ si auxlibx == 1 -> 136 Y 136 -> 1]
    {
      message.type= i;
      message.pos = i;
      msgsnd (shm.msqid, (struct msgbuf *)&message, sizeof(message.pos), IPC_NOWAIT);
    }

    inicio_falonso(speed, semid, shm.buf);  //Inicializar biblioteca
    if (gotSIGINT) 
    {
        ipcrm(semid,shmid,msqid);
        fprintf(stderr,"El programa ha finalizado correctamente\n\n");
        exit(EXIT_SUCCESS);
    }
    
    int color = -1;   
    int pid   =  0; 
    int desp  =  0;
    int vel   =  vel = rand()%101;
    int count = 0;

    int typeMsg = 0;
    int auxLib  = 0;
    int carril  = CARRIL_DERECHO;

    for (int i = 0; i < ret; i++)
    {        
        color++;
        desp+=1;
        speed = rand()%vel;
        if( color == 7) color = 0;
        switch (pid = fork()){
            case -1:
                    ipcrm(semid,shmid,msqid);
                    fprintf(stderr, "Error al realizar fork() \n");
                    break;
            case 0:  // Proceso Hijo  
                    actionSIGINT.sa_handler = sigintHandler;
	                actionSIGINT.sa_mask    = sigintonly;
	                actionSIGINT.sa_flags   = 0;

                    sigaction (SIGINT, &actionSIGINT, NULL );
                    msgrcv( shm.msqid, (void*)&message, sizeof(type_message)-sizeof(long), (int)desp, IPC_NOWAIT);   
                    if (gotSIGINT) break;
                
                    inicio_coche(&carril,&desp,color+16);    
                    semop_PV(semid,STAR_RACE,-1);
                    semop_PV(semid,STAR_RACE,0);
                    
                    if (gotSIGINT) break;
                    
                    auxLib = desp;
                    while (1){

                        if (gotSIGINT) break;  //Revisamos si se ha registrado la señal SIGINT                       
                        velocidad(speed, carril, desp);

                        if ( auxLib == 136 ) 
                        {   
                            avance_coche(&carril, &desp, color+16);
                            message.type= 1;
                            message.pos = 1;
                            msgsnd (shm.msqid, (struct msgbuf *)&message, sizeof(message.pos), IPC_NOWAIT);  
                            auxLib = 0;
                        }
     
                        if ((desp == 133 && carril == CARRIL_DERECHO) || 
                            (desp == 131 && carril == CARRIL_IZQUIERDO)) 
                        {
                            semop_PV(shm.semid,LAP_COUNT,1);
                            
                            semop_PV(semid, SHM_SEM, -1);
                            count=semctl( semid, LAP_COUNT, GETVAL);
                            if (count == SEMLIMIT ){
                                shm.count+=count;
                                semctl( semid, LAP_COUNT, SETVAL,0);
                            }
                            semop_PV(semid, SHM_SEM, 1);
                            
                        }
                       
                        if (gotSIGINT) break; //Revisamos si se ha registrado la señal SIGINT
                        
                        if ((desp == 105 && carril == CARRIL_DERECHO)|| 
                            (desp ==  98 && carril == CARRIL_IZQUIERDO)) 
                        {
                            if( shm.buf[274] == ROJO || shm.buf[274] == AMARILLO)
                            {
                                semop_PV(shm.semid, INTERCEPT_H,-1);
                            }
                        }
                        
                        if ((desp == 20 && carril == CARRIL_DERECHO) || 
                            (desp == 22 && carril == CARRIL_IZQUIERDO))
                        {
                            if ( shm.buf[275] == ROJO || shm.buf[275] == AMARILLO)
                            {
                                semop_PV(shm.semid, INTERCEPT_V,-1);
                            }
                        }
                    
                        if (gotSIGINT) break;   
                        
                        if (desp == 20 || desp == 105)
                        {
                             semop_PV(semid, SEM_V, -1);   
                        }
                       
                        if (desp == 22 || desp == 109) 
                        {
                            semop_PV(semid, SEM_V, 1);  
                        }

                        if (gotSIGINT) break;
                       
                        typeMsg = auxLib+1;   
                        if ( msgrcv( shm.msqid, (struct msgbuf *)&message, sizeof(type_message)-sizeof(long), (int)typeMsg, 0) != -1)
                        {
                            if (message.pos == typeMsg)
                            { 
                                avance_coche(&carril, &desp, color+16);
                                if (auxLib == 1) 
                                {                 
                                    message.type = 136;
                                    message.pos  = 136; 
                                    msgsnd (msqid, (struct msgbuf *)&message, sizeof(message) - sizeof(long), IPC_NOWAIT); 
                                }
                                else{
                                     message.type= auxLib;
                                     message.pos = auxLib;
                                     msgsnd (shm.msqid, (struct msgbuf *)&message, sizeof(message) - sizeof(long), IPC_NOWAIT);
                                }                                                                           
                                auxLib++;                                       
                            }
                        }
                        if (gotSIGINT) break; 
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

    for (int i = 0; i < ret; i++) wait(NULL);

    shm.count=shm.count+semctl( semid, LAP_COUNT, GETVAL);
    fin_falonso(&shm.count); 
    
    shmdt((char*)buf);
    if (gotSIGINT) ipcrm(semid,shmid,msqid);
     
    else ipcrm(semid,shmid,msqid);
      
    fprintf(stderr," - El programa ha finalizado correctamente\n\n");
    return EXIT_SUCCESS;
}

void ipcrm(int semid, int shmid, int msqid){
    shmctl(shmid,IPC_RMID,NULL);
    msgctl(msqid,IPC_RMID,NULL);    
    semctl(semid,IPC_RMID,0);
}

int semaphoreLight(shMemory shm, int semH, int semV){   
    luz_semAforo(HORIZONTAL, semH);
    luz_semAforo(VERTICAL,   semV);
 
    semop_PV(shm.semid, SHM_SEM,-1);
    if( shm.buf[274] == VERDE)
    {
        semop_PV( shm.semid, INTERCEPT_H, 1); 
    }
    semop_PV(shm.semid, SHM_SEM,1);
    
    semop_PV(shm.semid, SHM_SEM,-1);
    if( shm.buf[275] == VERDE)
    { 
         semop_PV( shm.semid, INTERCEPT_V, 1);  
    }
    semctl( shm.semid, INTERCEPT_V, SETVAL, 0);
    semctl( shm.semid, INTERCEPT_H,SETVAL, 0);
    semop_PV(shm.semid, SHM_SEM,1);
   
    sleep(3); semH++; semV++;

    if( semH == 4) semH = 1;
    if( semV == 4) semV = 1;

    if (gotSIGINT) return SIGINT_SIG;
    else  semaphoreLight(shm, semH, semV);

    return EXIT_SUCCESS;
}

int semop_PV (int semid, int index , int numop ){   
      struct sembuf op_p;  
      op_p.sem_num = index;    /** indice en el array de semafros **/
      op_p.sem_op  = numop;    /** un wait (resto 1)  signal sumo 1**/
      op_p.sem_flg =  0;       /** flag **/
      return semop( semid, &op_p, 1 );    
}

void sigintHandler  (int sn){ gotSIGINT = 1; }
