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

#define SIGINT_SIG 30
#define SEM_ARRAY  10
#define SHM_SEM    1
#define SPEED 90
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
            char* buf;
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

    int ret = 1; 
    int speed = 1; 

            ret   = atoi(argv[1]);
            speed = atoi(argv[2]);      

    //Tratamiento de señales SIGINT y Correspondeintes
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
    shm.count = 0;  
   
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

    int semid = semget(ftok("bin/cat", 888),SEM_ARRAY,0600 | IPC_CREAT); //Reserva de array de semaforos, cantidad SEM_ARRAY
    shm.semid = semid;
    for (int i = 0; i < SEM_ARRAY; i++)  semctl( semid, i,  SETVAL, 1); // declare 

    ret = 2; //Parámetro pasado por la terminal manejar de esa forma y borrar este
    semctl( semid, STAR_RACE, SETVAL, ret); // Inicializacion de los semáforos - START_RACE espera que todos los coches esten creados  
    semctl( semid, SEM_V, SETVAL, 1);       // Control de cruce del semáforo Vertical 
    semctl( semid, SEM_H, SETVAL, 1);       // Control de cruce del semáforo Horizontal
    semctl( semid, INTERCEPT_V, SETVAL, 0); // Permite avanzar los coches cuando luz del semáforo vertical está en verde
    semctl( semid, INTERCEPT_H, SETVAL, 0); // Permite avanzar los coches cuando luz del semáforo horizontal está en verde

    //inicilizar cola de mensaje para el carril derecho  
    type_message message;
    for ( int i = 1; i <=135; i++)
    {
      message.type= i;
      message.pos = i;
      msgsnd (shm.msqid, (struct msgbuf *)&message, sizeof(message.pos), IPC_NOWAIT);
    }
    
    inicio_falonso(speed, semid, shm.buf);  //Inicializar biblioteca

    if (gotSIGINT) //Se revisa si se ha registrado la señal SIGINT
    {
        ipcrm(semid,shmid,msqid);
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
                    ipcrm(semid,shmid,msqid);
                    fprintf(stderr, "Error al realizar fork() \n");
                    break;
            case 0:  // Proceso Hijo  
                actionSIGINT.sa_handler = sigintHandler;
	            actionSIGINT.sa_mask    = sigintonly;
	            actionSIGINT.sa_flags   = 0;

                sigaction (SIGINT, &actionSIGINT, NULL );
                if (gotSIGINT) break;
                
                //Recibo mensaje de mi posicion - desp
                msgrcv( shm.msqid, (void*)&message, sizeof(type_message)-sizeof(long), (int)desp, IPC_NOWAIT);    
                
                //Dibujamos un coche en pantalla en posicion desp y esperamos a dibujar los ret-1 restantes
                inicio_coche(&carril,&desp,color+16);
                semop_PV(shm.semid,STAR_RACE,-1);
                semop_PV(shm.semid,STAR_RACE,0);

                auxLib = desp; //Ciclo de Avance Carril Derecho , asignamos posicion a coche en el carril
                while (1){

                    if (gotSIGINT) break;  //Revisamos si se ha registrado la señal SIGINT
                    velocidad(SPEED, carril, desp);

                    if ( auxLib == 136) 
                    {
                      avance_coche(&carril, &desp, color+16);
                      message.type= 1;
                      message.pos = 1;
                      msgsnd (shm.msqid, (struct msgbuf *)&message, sizeof(message.pos), IPC_NOWAIT);  
                      auxLib = 0;
                    }
                                      
                    if (gotSIGINT) break; //Revisamos si se ha registrado la señal SIGINT

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
                           
                     if (auxLib+1 == 21){ // Revisamos que la siguiente posicion de auxLib sea el cruce 
                        semop_PV(shm.semid, SEM_V,-1); // De ser asi no quedamos bloquedos
                     }else 
                         if (auxLib-4 == 21)  //Dejamos libre el cruce para que pasen otros coches
                            semop_PV(shm.semid, SEM_V,1);
                     
                     if (auxLib+1 == 106){ // Revisamos que la siguiente posicion de auxLib sea el cruce
                        semop_PV(shm.semid, SEM_H,-1); // De ser asi no quedamos bloquedos
                     }else 
                         if (auxLib-4== 106) //Dejamos libre el cruce para que pasen otros coches
                           semop_PV(shm.semid, SEM_H,1);
                    
                     semop_PV(shm.semid,SHM_SEM,-1);
                       if (auxLib == 133)   // Incrementamos la cuanta de paso por meta
                           shm.count++;     // para que cincida con la biblioteca
                     semop_PV(shm.semid,SHM_SEM,1);

                    if (gotSIGINT) break;   //Revisamos si se ha registrado la señal SIGINT

                     typeMsg = auxLib+1;   //Intentamos avanzar reciviendo un mensaje de la posicion a la que queremos ir
                     if ( msgrcv( shm.msqid, (struct msgbuf *)&message, 
                                     sizeof(type_message)-sizeof(long), 
                                                (int)typeMsg, 0) != -1)
                     {
                         if (message.pos == typeMsg)
                         { 
                           avance_coche(&carril, &desp, color+16);
                           message.type= auxLib;
                           message.pos = auxLib;
                           msgsnd (shm.msqid, (struct msgbuf *)&message, 
                                                    sizeof(message.pos), 
                                                            IPC_NOWAIT);
 
                           if (auxLib == 1) //Si estamos auxLib es 1 debemos enviar mensaje 
                           {                //para dejar libre la posicion trasera  
                             message.type= 136;
                             message.pos = 136; // Hay que tener en cuanta que posicion 0 no puede
                             msgsnd (shm.msqid, (struct msgbuf *)&message, //implementarse con mensajes
                                                      sizeof(message.pos), //ya que al intentar recivir el mensaje tipo 0
                                                              IPC_NOWAIT); //se recivira cualquier mensaje en su lugar 
                           }                                               //este es el comportamiento prederminado de la funcion
                                                                           //por esa razon cogemos el 1 en vez de 0 como posicion anterior
                           auxLib++;                                       //ademas de por ser el ultimo deplazamiento del carrill derecho.
                         }//Una vez he podido avanzar incrementamos la posicion de auxLib.
                     }

                     if (gotSIGINT) break; //Revisamos si se ha registrado la señal SIGINT
                }
                
                if (gotSIGINT) //Si se ha registrado la señal SIGINT desvinculamos la memoria compartida 
                {              //con la de nuestro programa para poder eliminarla y cada porceso terminará.
                    shmdt((char*)shm.buf);
                    exit(0);
                }
                break;
            default: 
                    break;
        }
    }
    
    int    semH = VERDE; //Inicializacion de luces del ciclo semafórico 
    int    semV = ROJO;  //Inicializacion de luces del ciclo semafórico
    while(1)
    {
        if (gotSIGINT) break; //Si se registra la señal SIGINT salir del ciclo semafórico
        semaphoreLight(shm, semH, semV);
    }

    fin_falonso(&shm.count); //Enviamos la cuenta a la biblioteca 
    
    shmdt((char*)buf);
    if (gotSIGINT) 
        ipcrm(semid,shmid,msqid); //Eliminamos los recursos tanto si se ha detectado una señal SIGINT como si no
    else 
        ipcrm(semid,shmid,msqid);
      
    fprintf(stderr,"El programa ha finalizado correctamente\n\n");

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
 
    if( shm.buf[274] == VERDE)
    {
        semctl( shm.semid, SEM_H, SETVAL, 1);
        semctl( shm.semid, INTERCEPT_H, SETVAL, 0); 
    }

    if( shm.buf[275] == VERDE)
    { 
        semctl( shm.semid, SEM_V, SETVAL, 1);
        semctl( shm.semid, INTERCEPT_V, SETVAL, 0);  
    }
   
    sleep(3); // Los semaforos etardaran en cambiar de luz  3 segundos 
    semH++;
    semV++;

    if( semH == 4) semH = 1;
    if( semV == 4) semV = 1;

    if (gotSIGINT) 
        return SIGINT_SIG;
    else 
        semaphoreLight(shm, semH, semV);
   
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