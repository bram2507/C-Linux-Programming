#Makefile Abraham Alvarez Gomez 
# Macros Definition
GCC=      gcc
LIBS=     libfalonso.a
SOURCE=   falonso.c
OBJS= 	  falonso.c
OBJ=      -o
BUILD=    falonso
INCLUDES= -m32 -Wall -g
IPCS=	  sh kill_ipcs.sh
RM= rm -f -r
RUN= ./
CLRSCRN= clear

${BUILD}:${OBJS}
		${GCC} ${INCLUDES} ${SOURCE} ${LIBS} ${OBJ} ${BUILD}

clean:falonso
		${RM} ${BUILD}

run:falonso
		${CLRSCRN}
		${RUN}${BUILD} 5 1 
		
ipcs:
		${IPCS}