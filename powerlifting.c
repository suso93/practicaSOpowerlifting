#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

//Definicion de constantes
//#define 

//contador de atletas
int cuantos_atletas=0;

//inicializar los semaforos
pthread_mutex_t mutex,semaforo_atletas;

struct atletas_competicion {
	int id;
	int ha_competido;
	int tarima_asignada;
	int puntuacion;//
	int necesita_beber;
};
struct atletas_competicion atletas[10];//en el campeonato habrá máximo 10 atletas compitiendo simultáneamente

FILE *ficherolog;
int podio[3]; //podio de las puntuaciones del campeonato

void inicializaCampeonato();
int haySitioEnCampeonato();//nos dirá si hay sitio (y si lo hay nos dice el primer hueco) para que entre un atleta a competir
void nuevoCompetidor();
void competidorATarima1();
void finalizaCompeticion();
void accionesAtleta();
void accionesTarima();
void  writeLogMessage(char *id, char *msg);


int main (int argc, char *argv[]) {
	if(signal(SIGUSR1,competidorATarima1)==SIG_ERR) {
				perror("Llamada a signal.");
				exit(-1);
	}
	/*if(signal(SIGUSR2,competidorATarima2)==SIG_ERR) {
				perror("Llamada a signal.");
				exit(-1);
	}*/
	if(signal(SIGINT,finalizaCompeticion)==SIG_ERR) {
				perror("Llamada a signal.");
				exit(-1);
	}
	//inicializar recursos
	if (pthread_mutex_init(&semaforo_atletas, NULL)!=0)
+	{
+		perror("Error en la creación del semáforo de los atletas.\n");
+		exit(-1);
 	}
	inicializaCampeonato();
	
	FILE *ficherolog = fopen ("ficherolog.log","w"); //errores al abrir?
	//visualizo la estructura inicial: 
	for (int i=0;i<10;i++) {
		printf("Atleta %d: ha competido %d, su tarima actual es %d y necesita beber %d\n",atletas[i].id,atletas[i].ha_competido,atletas[i].tarima_asignada,atletas[i].necesita_beber);
	}
	if (haySitioEnCampeonato()==-1) {
		printf("La inscripción al campeonato está completa. No se puede inscribir aún.\n");
	} else {
		printf("Hueco para nuevo atleta: %d\n",haySitioEnCampeonato());
	}
	
}

void inicializaCampeonato() {
	for (int i=0;i<10;i++) {
		atletas[i].id=0;
		atletas[i].ha_competido=0;
		atletas[i].tarima_asignada=0;
		atletas[i].puntuacion=0;
		atletas[i].necesita_beber=0;
	}
}

int haySitioEnCampeonato() {
	for (int i=0;i<10;i++) {
		if (atletas[i].id==0) {
			return i;
		}
	}
	return -1;
}

void nuevoCompetidor (){

}
void AccionesAtleta (){

}
void competidorATarima1 (){
	//al recibir SIGUSR1 meter crear atleta y meter en la cola de la tarima1
}
void AccionesTarima (){

}
void finalizaCompeticion (){
}
void  writeLogMessage(char *id, char *msg) {
	//la hora  actual
	time_t  now = time (0);
	struct  tm *tlocal = localtime (&now);
	char  stnow [19];
	strftime(stnow , 19, " %d/ %m/ %y  %H: %M: %S", tlocal);
	//  Escribimos  en el log
	ficherolog = fopen("ficherolog.log" , "a");
	fprintf(ficherolog , "[ %s]  %s:  %s\n", stnow , id, msg);
	fclose(ficherolog);
}
