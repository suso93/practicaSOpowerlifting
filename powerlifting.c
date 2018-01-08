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
void nuevoCompetidor();//en realidad esta funcion creo que es la que usaremos para cuando se recibe la señal
//void competidorATarima1();
void finalizaCompeticion();
void accionesAtleta();//le pasaremos el atleta (VER ¿id?/¿posicion?)
void accionesTarima();
void  writeLogMessage(char *id, char *msg);


int main (int argc, char *argv[]) {
	if(signal(SIGUSR1,nuevoCompetidor)==SIG_ERR) {
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

	//inicializar recursos:
	/*if (pthread_mutex_init(&semaforo_atletas, NULL)!=0)
+	{
+		perror("Error en la creación del semáforo de los atletas.\n");
+		exit(-1);
 	}*/
	//el contador de atletas lo tenemos inicializado arriba
	inicializaCampeonato();
	//tarimas:crear los 2 hilos de tarimas (primero solo 1)
	//pthread_create(...);
	FILE *ficherolog = fopen ("ficherolog.log","w"); //errores al abrir?

	//visualizo la estructura inicial PARA IR PROBANDO -> BORRAR: 
	for (int i=0;i<10;i++) {
		printf("Atleta %d: ha competido %d, su tarima actual es %d y necesita beber %d\n",atletas[i].id,atletas[i].ha_competido,atletas[i].tarima_asignada,atletas[i].necesita_beber);
	}
	printf("El pid del campeonato es %d\n",getpid());
	if (haySitioEnCampeonato()==-1) {
		printf("La inscripción al campeonato está completa. No se puede inscribir aún.\n");
	} else {
		printf("Hueco para nuevo atleta: %d\n",haySitioEnCampeonato());
	}//HASTA AKI BORRAR

	//espera por señales:
	while(1) {
		
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
} //es posible que no lo necesitemos ya que se inicializan al crearse un nuevo competidor.. 

int haySitioEnCampeonato() {
	for (int i=0;i<10;i++) {
		if (atletas[i].id==0) {
			return i;
		}
	}
	return -1;
}//devuelve -1 en caso de no haber, y sino nos da el primer hueco que encuentre

void nuevoCompetidor (){
//al recibir SIGUSR1:
	int posicion;
	if(signal(SIGUSR1,nuevoCompetidor)==SIG_ERR) {
				perror("Llamada a signal.");
				exit(-1);
	}
	printf("Un atleta ha solicitado inscribirse...\n");
	if(haySitioEnCampeonato()!=-1) {
		printf("Vas a ser inscrito\n");
		cuantos_atletas++;
		posicion=haySitioEnCampeonato();
		atletas[posicion].id=cuantos_atletas;
		atletas[posicion].puntuacion=0;
		atletas[posicion].tarima_asignada=1;//al principio solo una tarima, ademas con SIGUSR1 va para tarima1
		atletas[posicion].ha_competido=0;
		atletas[posicion].necesita_beber=0;
		//creo hilo de atleta:
		//pthread_create();
		
	} else { 
		printf("Ya están inscritos y participando 10 atletas, de momento no puedes participar\n");
	}
}
void AccionesAtleta (){
	//guardar en log:
	//hora de entrada a tarima y a cual
	//calculo del comportamiento del atleta
		
}

void AccionesTarima (){
	//busca primer atleta en espera de su cola, sino el primero de la otra tarima



}
void finalizaCompeticion (){
	printf("Has pulsado finalizar competicion\n");
	//se realiza todo lo necesario para terminar con los atletas activos
	sleep(3);
	printf("Te mostraré los resultados\n");
	signal(SIGTERM, SIG_DFL);
	raise(SIGTERM);
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
				exit(-1);
	}
	//inicializar recursos
	/*if (pthread_mutex_init(&semaforo_atletas, NULL)!=0)
+	{
+		perror("Error en la creación del semáforo de los atletas.\n");
+		exit(-1);
 	}*/
	inicializaCampeonato();
	
	FILE *ficherolog = fopen ("ficherolog.log","w"); //errores al abrir?
	//visualizo la estructura inicial: 
	for (int i=0;i<10;i++) {
		printf("Atleta %d: ha competido %d, su tarima actual es %d y necesita beber %d\n",atletas[i].id,atletas[i].ha_competido,atletas[i].tarima_asignada,atletas[i].necesita_beber);
	}
	printf("El pid del campeonato es %d\n",getpid());
	if (haySitioEnCampeonato()==-1) {
		printf("La inscripción al campeonato está completa. No se puede inscribir aún.\n");
	} else {
		printf("Hueco para nuevo atleta: %d\n",haySitioEnCampeonato());
	}
	while(1) {
		
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
//al recibir SIGUSR1:
	int posicion;
	if(signal(SIGUSR1,nuevoCompetidor)==SIG_ERR) {
				perror("Llamada a signal.");
				exit(-1);
	}
	printf("Un atleta ha solicitado inscribirse...\n");
	if(haySitioEnCampeonato()!=-1) {
		printf("Vas a ser inscrito\n");
		cuantos_atletas++;
		posicion=haySitioEnCampeonato();
		atletas[posicion].id=cuantos_atletas;
	} else {
		printf("Ya están inscritos y participando 10 atletas, de momento no puedes participar\n");
	}
}
void AccionesAtleta (){

}
/*void competidorATarima1 (){
	//al recibir SIGUSR1 meter crear atleta y meter en la cola de la tarima1
}*/
void AccionesTarima (){

}
void finalizaCompeticion (){
	printf("Has pulsado finalizar competicion\n");
	//se realiza todo lo necesario para terminar con los atletas activos
	sleep(3);
	printf("Te mostraré los resultados\n");
	signal(SIGTERM, SIG_DFL);
	raise(SIGTERM);
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
				exit(-1);
	}
	//inicializar recursos
	/*if (pthread_mutex_init(&semaforo_atletas, NULL)!=0)
+	{
+		perror("Error en la creación del semáforo de los atletas.\n");
+		exit(-1);
 	}*/
	inicializaCampeonato();
	
	FILE *ficherolog = fopen ("ficherolog.log","w"); //errores al abrir?
	//visualizo la estructura inicial: 
	for (int i=0;i<10;i++) {
		printf("Atleta %d: ha competido %d, su tarima actual es %d y necesita beber %d\n",atletas[i].id,atletas[i].ha_competido,atletas[i].tarima_asignada,atletas[i].necesita_beber);
	}
	printf("El pid del campeonato es %d\n",getpid());
	if (haySitioEnCampeonato()==-1) {
		printf("La inscripción al campeonato está completa. No se puede inscribir aún.\n");
	} else {
		printf("Hueco para nuevo atleta: %d\n",haySitioEnCampeonato());
	}
	while(1) {
		
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
//al recibir SIGUSR1:
	int posicion;
	if(signal(SIGUSR1,nuevoCompetidor)==SIG_ERR) {
				perror("Llamada a signal.");
				exit(-1);
	}
	printf("Un atleta ha solicitado inscribirse...\n");
	if(haySitioEnCampeonato()!=-1) {
		printf("Vas a ser inscrito\n");
		cuantos_atletas++;
		posicion=haySitioEnCampeonato();
		atletas[posicion].id=cuantos_atletas;
	} else {
		printf("Ya están inscritos y participando 10 atletas, de momento no puedes participar\n");
	}
}
void AccionesAtleta (){

}
/*void competidorATarima1 (){
	//al recibir SIGUSR1 meter crear atleta y meter en la cola de la tarima1
}*/
void AccionesTarima (){

}
void finalizaCompeticion (){
	printf("Has pulsado finalizar competicion\n");
	//se realiza todo lo necesario para terminar con los atletas activos
	sleep(3);
	printf("Te mostraré los resultados\n");
	signal(SIGTERM, SIG_DFL);
	raise(SIGTERM);
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
