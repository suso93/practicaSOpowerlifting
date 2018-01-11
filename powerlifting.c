#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include<signal.h>
#include<pthread.h>
#include<sys/syscall.h>
#include<ctype.h> //DUDA mia: este para que es? Gracias :) --> es un include q tiene variables para chequear el caracter introduzido, pero no lo usamos todavía.

//Definicion de constantes

#define MAXIMOATLETAS 10
#define NUMEROTARIMAS 2
//contador de atletas que participen a lo largo del campeonato
int contadorAtletas=0;
/*int atletas_tarima1;
int atletas_tarima2;*/



//inicializar los semaforos:
pthread_mutex_t semaforo_fuente,semaforo_atletas;
pthread_cond_t condicion; // Condición para la fuente.

/*REVISAR que opcion de mutex es mejor y vamos a usar
//lo aclaró hoy y se usará como arriba en principio
pthread_mutex_t semaforo_Escribir;
pthread_mutex_t semaforo_Contador;
pthread_mutex_t semaforo_Entrada_Cola;
*/ // Yo diria q con estos 3 semaforos esta bien, el tercero es el q yo os decía q podíamos poner 2 en lugar de 1, depende de como lo miremos

struct atletasCompeticion {
	int id;
	int ha_competido;
	int tarima_asignada;
	int puntuacion;
	int necesita_beber;
	pthread_t atleta;
};
struct atletasCompeticion atletas[MAXIMOATLETAS];


/*//?Creamos struct para las tarimas? Sería algo asi:
struct tarimasCompeticion {
	int id;
	int estado;//0 para libre, 1 para ocupada, 2 para cerrada
	int atletas;//contador de atletas que han pasado por esta tarima
};
struct tarimasCompeticion tarima[NUMEROTARIMAS];*/ 
/*OTRA opcion para las tarimas:  //esta es la buena , pero esta tarde se mira bien
struct tarimasCompeticion{
	int id;
	int descansa; //estado: 3 estados segun las especificaciones
	int contador;
	pthread_t tatami;
};
struct tarimasCompeticion * punteroTarimas;*/

FILE *registro;
char *nombreArchivo = "registroTiempos.log";

/*int podio[3]; 
int mejoresAtletas[3];//no tengo claro porque hay un podio y un registro de los 3 mejores ¿no es lo mismo?*/ //MIERDA SE NOS OLVIDO PREGUNTARSELO
struct podioCompeticion {
	int id;//del atleta
	int puntuacion;
};
struct podioCompeticion podio[3]; //los tres mejores
int estadoFuente;//0 libre, 1 para ocupada
int finalizar;

//funciones
int calculaAleatorios();
void inicializaCampeonato();
int haySitioEnCampeonato();//nos dirá si hay sitio (y si lo hay nos dice el primer hueco) para que entre un atleta a competir
void nuevoCompetidor(int sig);//REVISAR al añadir para ya dos tarimas
//void competidorATarima1();
void finalizaCompeticion(int sig);
void *accionesAtleta(void*, int );//le pasaremos el atleta (VER ¿id?/¿posicion?) y la tarima
void *accionesTarima(void*);//le pasaremos la tarima

void  writeLogMessage(char *id, char *msg);

int main (int argc, char *argv[]) {
	//SIGUSR1 para enviar a tarima1 
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
	/*SI USAMOS lo de las tarimas con punteros:
	punteroTarimas = (struct tarimasCompeticion*)malloc((sizeof(struct tarimasCompeticion))*t);
	//se inicializan las tarimas
	for(i=0; i<t; i++){
		punteroTarimas[i].id=i+1;
		punteroTarimas[i].descansa=0;
		punteroTarimas[i].contador=0;
		pthread_create(&punteroTarimas[i].tatami , NULL, accionesTarima, (void*)&punteroTarimas[i].id);
	}
	*/
	/*if (pthread_mutex_init(&semaforo_atletas, NULL)!=0)
	{
		perror("Error en la creación del semáforo de los atletas.\n");
		exit(-1);
 	}*/
	//el contador de atletas lo tenemos inicializado arriba
	inicializaCampeonato();
	//tarimas:crear los 2 hilos de tarimas (primero solo 1)
	//pthread_create(...);
	registro = fopen (nombreArchivo,"w"); //errores al abrir?
	srand (time(NULL)); //Para generar numeros aleatorios, VER si hacemos asi o con otra semilla
	
	//visualizo la estructura inicial PARA IR PROBANDO -> BORRAR: 
	for (int i=0;i<MAXIMOATLETAS;i++) {
		printf("Atleta %d: ha competido %d, su tarima actual es %d y necesita beber %d\n",atletas[i].id,atletas[i].ha_competido,atletas[i].tarima_asignada,atletas[i].necesita_beber);
	}
	printf("El pid del campeonato es %d\n",getpid());
	if (haySitioEnCampeonato()==-1) {
		printf("La inscripción al campeonato está completa. No se puede inscribir aún.\n");
	} else {
		printf("Hueco para nuevo atleta: %d\n",haySitioEnCampeonato());
	}//HASTA AKI BORRAR


	while(1) {
	
		sleep(1);
		
	}
	
}

void inicializaCampeonato() {
	for (int i=0;i<MAXIMOATLETAS;i++) {
		atletas[i].id=0;
		atletas[i].ha_competido=0;
		atletas[i].tarima_asignada=0;
		atletas[i].puntuacion=0;
		atletas[i].necesita_beber=0;
	}
} //es posible que no lo necesitemos ya que se inicializan al crearse un nuevo competidor.. 

int haySitioEnCampeonato() {
	for (int i=0;i<MAXIMOATLETAS;i++) {
		if (atletas[i].id==0) {
			return i;
		}
	}
	return -1;
}//devuelve -1 en caso de no haber, y sino nos da el primer hueco que encuentre

void nuevoCompetidor (int sig){ 
//al recibir SIGUSR1:
	int posicion;
	if(signal(SIGUSR1,nuevoCompetidor)==SIG_ERR) {
				perror("Llamada a signal.");
				exit(-1);
	}
	printf("Un atleta ha solicitado inscribirse...\n");
	if(haySitioEnCampeonato()!=-1) {
		printf("Vas a ser inscrito\n");
		
		if (pthread_mutex_lock(&semaforo_atletas)!=0)
		{
			perror("Error en el bloqueo del semáforo de los atletas.\n");
			exit(-1);
		}
		
		contadorAtletas++;
		posicion=haySitioEnCampeonato();
		atletas[posicion].id=contadorAtletas;
		atletas[posicion].puntuacion=0;
		atletas[posicion].tarima_asignada=1;//al principio solo una tarima, ademas con SIGUSR1 va para tarima1 //DUDA: ¿aqui se ponia la tarima o se indicaba si la tenia asignada o no?
		atletas[posicion].ha_competido=0;
		atletas[posicion].necesita_beber=0;
		
		printf("El atleta %i se prepara para ir a la tarima 1.\n", posicion+1);
		
		//creo hilo de atleta:
		pthread_create(&atletas[posicion].atleta, NULL, accionesAtleta, (void *)&atletas[posicion].id);

		
		//ANTES o despues de accionesAtleta?
		// Se desbloquea el semáforo, además se comprueba si falla.

		if (pthread_mutex_unlock(&semaforo_atletas)!=0)
		{
			perror("Error en el desbloqueo del semáforo de los atletas.\n");
			exit(-1);
		}
		
	} else { 
		printf("Ya están inscritos y participando 10 atletas, de momento no puedes participar\n");
	}
}
void *accionesAtleta (void *arg){ //
	

// CALCULAR LA POSICIÓN DEL ATLETA CON EL ID ???

	//guardar en log:
	//hora de entrada a tarima y a cual => La hora la escribe la funcion del log
	char *msg = "He entrado a la tarima 1";   //Modificar cuando utilicemos 2 tarimas
	writeLogMessage(arg, msg);
	//calculo del comportamiento del atleta

	// El atleta llega a la tarima y espera 4 segundos para realiza su levantamiento.

	printf("El atleta %i se prepara para realizar el levantamiento.\n", *(int*) arg);
	sleep(4);

	// Comportamiento del atleta: comprobación del estado de salud.

	int estado_salud=calculaAleatorios(0,100);
	if (estado_salud<=15)
	{
		printf("El atleta %i no puede realizar el levantamiento por problemas de deshidratación.\n", *(int*) arg);		
	// Salir de la cola ???. 3.a. Si no llega a realizar el levantamiento, no llega a subir a la tarima y se escribe en el log, se daría fin al hilo Atleta y se liberaría espacio en la cola.
	}	
	else
	{
		sleep (3);
	}
	// 4. Si ya ha salido a competir, debemos esperar a que termine.
	// 5. Guardamos en el log la hora a la que ha finalizado su levantamiento.
	// 6. Fin del hilo del atleta. NOTA: el hilo se sigue ejecutando aun asi este el tio ataascado en la fuente.
		
}
/*
void *actosTarima(void *id){

}
*/
void *AccionesTarima (void *arg){
	int descansoTarima=0; //contador de atletas que han participado en la tarima para empezar a descansar
	//busca primer atleta en espera de su cola, sino el primero de la otra tarima
	// 2. Cambiamos el flag . (NOTA: se pone a 1) ¿QUÉ FLAG? 


	// Se calcula lo que le sucede al atleta y se guarda en el fichero log la hora a la que realizó el levantamiento.
	
	int comportamiento = calculaAleatorios(0,10); 	//Numero aleatorio para calcular el comportamiento
	if(comportamiento <8) {
		int tiempo = calculaAleatorios(2,6);
		sleep(tiempo);
		char *msg = "He hecho un levantamiento valido en: ";    //La hora la indica la funcion del log
		writeLogMessage(atletas[posicion].id, msg);  //Duda sobre si poner id del atleta o la tarima 
		
		int puntuacion = calculaAleatorios(60,300);
		atletas[posicion].puntuacion = puntuacion;
		printf("El juez califica el movimiento con la puntuación de: %i.\n", puntuacion);
		//Escribir en el log la puntuacion ganada por el atleta
		writeLogMessage(atletas[posicion].id, msg);
	} else if(comportamiento = 8) {
		int tiempo = calculaAleatorios(1,4);
		sleep(tiempo);
		char *msg = "Movimiento nulo por incumplimiento de normas";    
	//	writeLogMessage(atletas[posicion].id, msg);	//Duda sobre si poner id del atleta o la tarima 
		printf("%s\n", msg);
		int puntuacion = 0;
	} else {
		int tiempo = calculaAleatorios(6,10);
		sleep(tiempo);
		char *msg = "Movimiento nulo por falta de fuerzas";    
	//	writeLogMessage(atletas[posicion].id, msg);	//Duda sobre si poner id del atleta o la tarima 
		printf("%s\n", msg);
		int puntuacion = 0;
	//	atletas[posicion].puntuacion = puntuacion;
		printf("El juez califica el movimiento como nulo, por lo tanto la puntuación es: %i.\n", puntuacion);
	}
	
	if(calculaAleatorios(0,10) == 1) {		//calcula si el atleta necesita beber o no
	//	atletas[posicion].necesita_beber=1;	
	}
	
	//Finaliza el atleta que esta participando
	descansoTarima ++;
	// Se comprueba si al juez le toca descansar (cada 4 atletas 10 segundos)
	if(descansoTarima == 4) {
		sleep(10);
		
		descansoTarima = 0;
	}
	// 12. Volvemos al paso 1 y buscamos el siguiente (siempre priorizando entre los atletas asignados a dicha tarima).
	//para la parte opcional, con el malloc reservo espacio para muchos mas, en plan 300, y no con eso quiero decir q vaya  a permitir entrarl a todos, sino que hay espacio en memoria
 
}
void finalizaCompeticion (int sig){
	printf("Has pulsado finalizar competicion\n");
	//Parar de recibir señales!!!!

	//Y se realiza todo lo necesario para terminar con los atletas activos
	//PRUEBA para ir viendo como están en el momento de la petición:
	for (int i=0;i<MAXIMOATLETAS;i++) {
		printf("Atleta %d: ha competido %d, su tarima actual es %d y necesita beber %d\n",atletas[i].id,atletas[i].ha_competido,atletas[i].tarima_asignada,atletas[i].necesita_beber);
	}//
	sleep(3);
	printf("Te mostraré los resultados\n");//
	fclose(registro);
	signal(SIGTERM, SIG_DFL);
	raise(SIGTERM);//Que es y paa que sirve
}
void  writeLogMessage(char *id, char *msg) {
	//la hora  actual
	time_t  now = time (0);
	struct  tm *tlocal = localtime (&now);
	char  stnow [19];
	strftime(stnow , 19, " %d/ %m/ %y  %H: %M: %S", tlocal);
	//  Escribimos  en el log
	registro = fopen(nombreArchivo, "a");
	fprintf(registro , "[ %s]  %s:  %s\n", stnow , id, msg);
	fclose(registro);
}

int calculaAleatorios(int min, int max) {
	return rand() % (max-min+1) + min;
}

