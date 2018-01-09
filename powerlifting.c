#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

//Definicion de constantes
#define maximoAtletas 10

//contador de atletas que participen a lo largo del campeonato
int cuantos_atletas=0;
/*int atletas_tarima1;
int atletas_tarima2;*/
int descansoTarima1=0; //contador de atletas que han participado en la tarima para empezar a descansar
int descansoTarima2=0;

//inicializar los semaforos:
pthread_mutex_t mutex,semaforo_atletas;

struct atletas_competicion {
	int id;
	int ha_competido;
	int tarima_asignada;
	int puntuacion;
	int necesita_beber;
};
struct atletas_competicion atletas[maximoAtletas];


/*//?Creamos struct para las tarimas? Sería algo asi:
struct tarimas_competicion {
	int id;
	int estado;//0 para libre, 1 para ocupada, 2 para cerrada
	int atletas;//contador de atletas que han pasado por esta tarima
};
struct tarimas_competicion tarima[numTarimas];*/

FILE *ficherolog;

int podio[3]; 
int mejoresAtletas[3];//no tengo claro porque hay un podio y un registro de los 3 mejores ¿no es lo mismo?
int estadoFuente;//0 libre, 1 para ocupada

//funciones
void inicializaCampeonato();
int haySitioEnCampeonato();//nos dirá si hay sitio (y si lo hay nos dice el primer hueco) para que entre un atleta a competir
void nuevoCompetidor();//en realidad esta funcion creo que es la que usaremos para cuando se recibe la señal
//void competidorATarima1();
void finalizaCompeticion();
void accionesAtleta();//le pasaremos el atleta (VER ¿id?/¿posicion?) y la tarima
void accionesTarima();//le pasaremos la tarima
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
	srand (time(NULL)); //Para generar numeros aleatorios
	
	//visualizo la estructura inicial PARA IR PROBANDO -> BORRAR: 
	for (int i=0;i<maximoAtletas;i++) {
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
	for (int i=0;i<maximoAtletas;i++) {
		atletas[i].id=0;
		atletas[i].ha_competido=0;
		atletas[i].tarima_asignada=0;
		atletas[i].puntuacion=0;
		atletas[i].necesita_beber=0;
	}
} //es posible que no lo necesitemos ya que se inicializan al crearse un nuevo competidor.. 

int haySitioEnCampeonato() {
	for (int i=0;i<maximoAtletas;i++) {
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
		atletas[posicion].tarima_asignada=1;//al principio solo una tarima, ademas con SIGUSR1 va para tarima1 //DUDA: ¿aqui se ponia la tarima o se indicaba si la tenia asignada o no?
		atletas[posicion].ha_competido=0;
		atletas[posicion].necesita_beber=0;
		//creo hilo de atleta:
		//pthread_create();
		AccionesAtleta();//ver qué le pasamos, creo que habria que pasarle id atleta y id tarima
	} else { 
		printf("Ya están inscritos y participando 10 atletas, de momento no puedes participar\n");
	}
}
void AccionesAtleta (){ //
	//guardar en log:
	//hora de entrada a tarima y a cual
	char msg = "He entrado a la tarima 1";   //Modificar cuando utilicemos 2 tarimas
	writeLogMessage(atletas[posicion].id, msg);
	//calculo del comportamiento del atleta

	
		
}

void AccionesTarima (){
	//busca primer atleta en espera de su cola, sino el primero de la otra tarima
	
	
	int comportamiento = calculaAletorios(0,10); 	//Numero aleatorio para calcular el comportamiento
	if(comportamiento <8) {
		int tiempo = calculaAleatorios(2,6);
		sleep(tiempo);
		char msg = "He hecho un levantamiento valido en: ";    //Añadir el tiempo
		writeLogMessage(atletas[posicion].id, msg);  //Duda sobre si poner id del atleta o la tarima 
		
		int puntuacion = calculaAleatorios(60,300);
		atletas[posicion].puntuacion = puntuacion;
		
		//Escribir en el log la puntuacion ganada por el atleta
		
	} else if(comportamiento = 8) {
		int tiempo = calculaAleatorios(1,4);
		sleep(tiempo);
		char msg = "Movimiento nulo por incumplimiento de normas";    
		writeLogMessage(atletas[posicion].id, msg);	//Duda sobre si poner id del atleta o la tarima 
		
		int puntuacion = 0;
	} else {
		int tiempo = calculaAleatorios(6,10);
		sleep(tiempo);
		char msg = "Movimiento nulo por falta de fuerzas";    
		writeLogMessage(atletas[posicion].id, msg);	//Duda sobre si poner id del atleta o la tarima 
		
		int puntuacion = 0;
	}
	
	if(calculaAleatorios(0,10) == 1) {		//calcula si el atleta necesita beber o no
		atletas[posicion].necesita_beber=1;	
	}
	
	//Finaliza el atleta que esta participando
	descansoTarima1 ++;
	if(descansoTarima1 == 4) {
		sleep(10);
		
		descansoTarima1 = 0;
	}

}
void finalizaCompeticion (){
	printf("Has pulsado finalizar competicion\n");
	//Parar de recibir señales!!!!

	//Y se realiza todo lo necesario para terminar con los atletas activos
	//PRUEBA para ir viendo como están en el momento de la petición:
	for (int i=0;i<maximoAtletas;i++) {
		printf("Atleta %d: ha competido %d, su tarima actual es %d y necesita beber %d\n",atletas[i].id,atletas[i].ha_competido,atletas[i].tarima_asignada,atletas[i].necesita_beber);
	}//
	sleep(3);
	printf("Te mostraré los resultados\n");//
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

int calculaAleatorios(int min, int max) {
	return rand() % (max-min+1) + min;
}

