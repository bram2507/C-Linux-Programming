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
#define SEM_ARRAY  10
#define SHM_SEM    1
#define SPEED 90
#define STAR_RACE 2
#define SHM_LIMIT 301


#define SEM_V 3
#define SEM_H 4
#define INTERCEPT_V 5
#define INTERCEPT_H 6
#define TEST_SEM 7
#define LAP_COUNT 8
#define LIMIT 20


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
int  inicio          ( char **argumentos);

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
    shm.count = shm.cambioD = 0;  
   
    int shmid = shmget(IPC_PRIVATE, sizeof(char)*SHM_LIMIT, IPC_CREAT | 0600 );  //Reserva e Inicializacion Memoria Compartida
    shm.buf   = (char *) shmat (shmid, NULL, 0);  // Se vincula la memoria compartida en espacio del SO con la memoria de nuestro programa 
    char *buf = shm.buf;
    shm.shmid = shmid;
    shm.buf[SHM_LIMIT] = '0';

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

    ret = 5; //Parámetro pasado por la terminal manejar de esa forma y borrar este
    semctl( semid, STAR_RACE, SETVAL, ret); // Inicializacion de los semáforos - START_RACE espera que todos los coches esten creados  
    semctl( semid, SEM_V, SETVAL, 1);       // Control de cruce del semáforo Vertical 
    semctl( semid, SEM_H, SETVAL, 1);       // Control de cruce del semáforo Horizontal
    semctl( semid, INTERCEPT_V, SETVAL, 0); // Permite avanzar los coches cuando luz del semáforo vertical está en verde
    semctl( semid, INTERCEPT_H, SETVAL, 0); // Permite avanzar los coches cuando luz del semáforo horizontal está en verde
    semctl( semid, TEST_SEM, SETVAL, 0);
    semctl( semid, LAP_COUNT, SETVAL, 0);
   
    type_message message; //inicilizar cola de mensaje para el carril derecho  
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
    
    int color = 1;   
    int pid   = 0; 
    int desp  = 0;
    int indice = 0;

    int typeMsg = 0;
    int auxLib  = 0;
    int carril  = CARRIL_DERECHO;

    int cambioCarrilDi[]  = {0,13,14,28,29,60,61,63,62,65,66,67,68,69,129,130,131,134,135,136};
    int cambioCarrilDii[] = {0,13,15,29,29,60,60,61,61,63,63,64,64,64,124,127,129,132,134,135};

    int cambioCarrilId[]  = {0,15,16,28,29,58,59,60,61,62,63,64,65,125,126,127,128,129,133,136};
    int cambioCarrilIdd[] = {0,15,15,27,29,58,60,61,63,64,67,68,70,130,130,130,131,131,135,136};
    
    srand(time(NULL));
    int n = 0;
    for (int i = 0; i < ret; i++)
    {   
        color++;
        desp+=1; 
        n = rand() % 91;
        speed = rand()%n;
        if( color == 7) color = 0;
        switch (pid = fork()){
            case -1:
                    ipcrm(semid,shmid,msqid);
                    fprintf(stderr, "Error al realizar fork() \n");
                    break;
            case 0:  // Proceso Hijo  
                srand(time(NULL));
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

                        if ( auxLib == 136 ) 
                        {
                            avance_coche(&carril, &desp, color+16);
                            message.type= 1;
                            message.pos = 1;
                            msgsnd (shm.msqid, (struct msgbuf *)&message, sizeof(message.pos), IPC_NOWAIT);  
                            auxLib = 0;
                        }

                        semop_PV(shm.semid, SHM_SEM,-1);
                        if ((desp == 133 && carril == CARRIL_DERECHO) || 
                            (desp == 131 && carril == CARRIL_IZQUIERDO)) 
                            semop_PV(shm.semid,LAP_COUNT,1);
                        semop_PV(shm.semid, SHM_SEM,1);

                        if (gotSIGINT) break; //Revisamos si se ha registrado la señal SIGINT
                        
                    
                        if ((desp == 105 && carril == CARRIL_DERECHO)|| 
                            (desp ==  98 && carril == CARRIL_IZQUIERDO)) {
                            if( shm.buf[274] == ROJO || shm.buf[274] == AMARILLO){
                                semop_PV(shm.semid, INTERCEPT_H,-1);
                            }
                        }
                        
                        if ((desp == 20 && carril == CARRIL_DERECHO) || 
                            (desp == 21 && carril == CARRIL_IZQUIERDO)){
                            if ( shm.buf[275] == ROJO || shm.buf[275] == AMARILLO){
                                semop_PV(shm.semid, INTERCEPT_V,-1);
                            }
                        }
                  
                        if (gotSIGINT) break;   //Revisamos si se ha registrado la señal SIGINT
                        
                       
                        
                       if (carril == CARRIL_DERECHO){
                         
                            do{
                                indice = rand() % 20;
                            }while(indice>=20);
                            if (desp == cambioCarrilDi[indice]){
                                typeMsg = cambioCarrilDii[indice];
                                if ( msgrcv( shm.msqid, (struct msgbuf *)&message, 
                                                sizeof(type_message)-sizeof(long), 
                                                        (int)typeMsg, 0) != -1)
                                {
                                    if (message.pos == typeMsg)
                                    {
                                        cambio_carril(&carril,&desp,color+16);
                                        message.type= cambioCarrilDi[indice];
                                        message.pos = cambioCarrilDi[indice]; 
                                        msgsnd (shm.msqid, (struct msgbuf *)&message, 
                                                                sizeof(message.pos), 
                                                                        IPC_NOWAIT);

                                        if (desp == 1) //Si estamos auxLib es 1 debemos enviar mensaje 
                                        {                //para dejar libre la posicion trasera  
                                            message.type= 136;
                                            message.pos = 136; // Hay que tener en cuanta que posicion 0 no puede
                                            msgsnd (shm.msqid, (struct msgbuf *)&message, //implementarse con mensajes
                                                                    sizeof(message.pos), //ya que al intentar recivir el mensaje tipo 0
                                                                            IPC_NOWAIT); //se recivira cualquier mensaje en su lugar 
                                        }    
                                    
                                        auxLib = desp;
                                        
                                    }
                                }
                        }
                         else{
                                     typeMsg = desp+1;   //Intentamos avanzar reciviendo un mensaje de la posicion a la que queremos ir
                                        if ( msgrcv( shm.msqid, (struct msgbuf *)&message, 
                                                        sizeof(type_message)-sizeof(long), 
                                                                    (int)typeMsg, 0) != -1)
                                        {
                                            if (message.pos == typeMsg)
                                            { 
                                            message.type= desp;
                                            message.pos = desp;
                                            avance_coche(&carril, &desp, color+16);
                                           
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
                            }
                            

                        }else if ( carril == CARRIL_IZQUIERDO){
                             
                            do{
                                indice = rand() % 20;
                            }while(indice>=20);
                            if (desp == cambioCarrilId[indice]){
                                typeMsg = cambioCarrilIdd[indice];
                                if ( msgrcv( shm.msqid, (struct msgbuf *)&message, 
                                                sizeof(type_message)-sizeof(long), 
                                                        (int)typeMsg, 0) != -1)
                                {
                                    if (message.pos == typeMsg)
                                    {
                                        cambio_carril(&carril,&desp,color+16);
                                        message.type= cambioCarrilId[indice];
                                        message.pos = cambioCarrilId[indice]; 
                                        msgsnd (shm.msqid, (struct msgbuf *)&message, 
                                                                sizeof(message.pos), 
                                                                        IPC_NOWAIT);

                                        if (desp == 1) //Si estamos auxLib es 1 debemos enviar mensaje 
                                        {                //para dejar libre la posicion trasera  
                                            message.type= 136;
                                            message.pos = 136; // Hay que tener en cuanta que posicion 0 no puede
                                            msgsnd (shm.msqid, (struct msgbuf *)&message, //implementarse con mensajes
                                                                    sizeof(message.pos), //ya que al intentar recivir el mensaje tipo 0
                                                                            IPC_NOWAIT); //se recivira cualquier mensaje en su lugar 
                                        }    
                                    
                                        auxLib = desp;
                                        
                                    }
                                }

                             }else{

                              
                                        typeMsg = desp+1;   //Intentamos avanzar reciviendo un mensaje de la posicion a la que queremos ir
                                        if ( msgrcv( shm.msqid, (struct msgbuf *)&message, 
                                                        sizeof(type_message)-sizeof(long), 
                                                                    (int)typeMsg, 0) != -1)
                                        {
                                            if (message.pos == typeMsg)
                                            { 
                                            message.type= desp;
                                            message.pos = desp;
                                            avance_coche(&carril, &desp, color+16);
                                           
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
                        }
                                }
                            }

                        // } 
                        
                      
                          
                           
                       
                       
                        if (gotSIGINT) break; //Revisamos si se ha registrado la señal SIGINT

                  
                    
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
    for (int i = 0; i < ret; i++) wait(NULL);

    shm.count=semctl( semid, LAP_COUNT, GETVAL);
    fin_falonso(&shm.count); //Enviamos la cuenta a la biblioteca 
    
    shmdt((char*)buf);
    if (gotSIGINT) 
        ipcrm(semid,shmid,msqid); //Eliminamos los recursos tanto si se ha detectado una señal SIGINT como si no
    else 
        ipcrm(semid,shmid,msqid);
      
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
       
    }else semctl( shm.semid, INTERCEPT_H,SETVAL, 0);

    semop_PV(shm.semid, SHM_SEM,1);
    
    semop_PV(shm.semid, SHM_SEM,-1);
    if( shm.buf[275] == VERDE)
    { 
         semop_PV( shm.semid, INTERCEPT_V, 1); 
        
    }else semctl( shm.semid, INTERCEPT_V, SETVAL, 0);

    semop_PV(shm.semid, SHM_SEM,1);
   
    sleep(1); // Los semaforos etardaran en cambiar de luz  3 segundos 
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