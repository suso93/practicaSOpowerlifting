#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <ctype.h> 

//Definicion de constantes

#define MAXIMOATLETAS 10
#define NUMEROTARIMAS 2
//contador de atletas que participen a lo largo del campeonato
int contadorAtletas=0;


//inicializar los semaforos:
pthread_mutex_t semaforo_fuente;//semáforo que controla el acceso a la fuente
pthread_mutex_t semaforo_atletas;//semáforo para la entrada de nuevos atletas
pthread_mutex_t semaforo_escribir;//semáforo para escritura en el log
pthread_cond_t condicion; 

struct atletasCompeticion {
	int id;
	int ha_competido;
	int tarima_asignada;
	int puntuacion;
	int necesita_beber;
	pthread_t atleta;
};

struct atletasCompeticion *atletas;


struct tarimasCompeticion{
	int id;
	int descansa; //cuenta hasta cuatro para descansar y después se pon a cero
	int contador; //cuenta todos los atletas que han pasado por la tarima
	pthread_t tatami;
};
struct tarimasCompeticion * punteroTarimas;

FILE *registro;
char *nombreArchivo = "registroTiempos.log";

int podium[2][3];

int maxAtletas;
int numTarimas;

int * cola;

int estadoFuente;//0 libre, 1 para ocupada
int finalizar;

//funciones
int calculaAleatorios();
void inicializaCampeonato();
int haySitioEnCampeonato();//nos dirá si hay sitio (y si lo hay nos dice el primer hueco) para que entre un atleta a competir
void nuevoCompetidor(int sig);//REVISAR al añadir para ya dos tarimas
void finalizaCompeticion(int sig);
void *accionesAtleta(void*);//le pasaremos el atleta
void *accionesTarima(void*);//le pasaremos la tarima

void  writeLogMessage(char *id, char *msg);

int main (int argc, char *argv[]) {
	//parte opcional--> Asignacion estatica de recursos
	maxAtletas = MAXIMOATLETAS;
	numTarimas = NUMEROTARIMAS;
	if(argc==2) maxAtletas=atoi(argv[1]);
	if(argc==3){
		 maxAtletas=atoi(argv[1]);
		 numTarimas=atoi(argv[2]);
	}
		
	//SIGUSR1 para enviar a tarima1 
	if(signal(SIGUSR1,nuevoCompetidor)==SIG_ERR) {
				perror("Llamada a signal.");
				exit(-1);
	}
	if(signal(SIGUSR2,nuevoCompetidor)==SIG_ERR) {
				perror("Llamada a signal.");
				exit(-1);
	}
	if(signal(SIGINT,finalizaCompeticion)==SIG_ERR) {
				perror("Llamada a signal.");
				exit(-1);
	}


	punteroTarimas = (struct tarimasCompeticion*)malloc(sizeof(struct tarimasCompeticion)*numTarimas);
	atletas = (struct atletasCompeticion*)malloc(sizeof(struct atletasCompeticion)*maxAtletas);
	
	
	if (pthread_mutex_init(&semaforo_atletas, NULL)!=0)
	{
		perror("Error en la creación del semáforo de los atletas.\n");
		exit(-1);
 	}
 	
 	if (pthread_mutex_init(&semaforo_fuente, NULL)!=0)
	{
		perror("Error en la creación del semáforo de los atletas.\n");
		exit(-1);
 	}
 	
 	if (pthread_mutex_init(&semaforo_escribir, NULL)!=0)
	{
		perror("Error en la creación del semáforo de los atletas.\n");
		exit(-1);
 	}
 	
 	if (pthread_cond_init(&condicion, NULL)!=0)
	{
		perror("Error en la creación del semáforo de los atletas.\n");
		exit(-1);
 	}
 	
 	
	//el contador de atletas lo tenemos inicializado arriba
	inicializaCampeonato(maxAtletas, numTarimas);

	registro = fopen (nombreArchivo,"w");
	if(registro==NULL){
		
	}else{
		fclose(registro);
		printf("El pid del campeonato es %d\n",getpid());
		srand (time(NULL)); //Para generar numeros aleatorios, VER si hacemos asi o con otra semilla
		while(finalizar==0) {
			sleep(1);
		}
	}
	return 0;	
}

void inicializaCampeonato(int maxAtletas, int numTarimas) {
	int i;
	finalizar=0;
	for (i=0;i<maxAtletas;i++) {
		atletas[i].id=0;
		atletas[i].ha_competido=0;
		atletas[i].tarima_asignada=0;
		atletas[i].puntuacion=0;
		atletas[i].necesita_beber=0;
		//desbloquear acceso a esa posicion
	}
	//inicializar las colas de las tarimas
	for(i=0; i<numTarimas; i++){
		punteroTarimas[i].id=i+1;
		punteroTarimas[i].descansa=0;
		punteroTarimas[i].contador=0;
		pthread_create(&punteroTarimas[i].tatami , NULL, accionesTarima, (void*)&punteroTarimas[i].id);
	}
}

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
	if(signal(SIGUSR2,nuevoCompetidor)==SIG_ERR) {
				perror("Llamada a signal.");
				exit(-1);
	}

	printf("Un atleta ha solicitado inscribirse...\n");
	
	if (pthread_mutex_lock(&semaforo_atletas)!=0)
	{
		perror("Error en el bloqueo del semáforo de los atletas.\n");
		exit(-1);
	}
		posicion = haySitioEnCampeonato();

		if(posicion!=-1) {
			printf("Vas a ser inscrito, chavalote\n");
			contadorAtletas++;
			atletas[posicion].id=contadorAtletas;
			atletas[posicion].puntuacion=0;
			if(sig== SIGUSR1){
				atletas[posicion].tarima_asignada=1;//al principio solo una tarima, ademas con SIGUSR1 va para tarima1 //DUDA: ¿aqui se ponia la tarima o se indicaba si la tenia asignada o no?
			}else if(sig== SIGUSR2){
				atletas[posicion].tarima_asignada=2;
			}
			atletas[posicion].ha_competido=0;
			atletas[posicion].necesita_beber=0;
		
			printf("El atleta %d se prepara para ir a la tarima %d.\n", atletas[posicion].id, atletas[posicion].tarima_asignada);
		
			//creo hilo de atleta:
			pthread_create(&atletas[posicion].atleta, NULL, accionesAtleta, (void *)&atletas[posicion].id);

		
			//ANTES o despues de accionesAtleta?
			// Se desbloquea el semáforo, además se comprueba si falla.

		} else { 
			printf("Ya están inscritos y participando 10 atletas, de momento no puedes participar\n");
		}
		
	if (pthread_mutex_unlock(&semaforo_atletas)!=0)
	{
		perror("Error en el desbloqueo del semáforo de los atletas.\n");
		exit(-1);
	}
	
}
void *accionesAtleta (void *arg){ 
	int dorsal = *(int*)arg;
	int pos;
	int i;
	int estado_salud=calculaAleatorios(0,100);
	
	for(i=0; i<maxAtletas; i++){
		if(atletas[i].id==dorsal){
			pos = i;
			break;
		}
	}

	//guardar en log:
	//hora de entrada a tarima y a cual => La hora la escribe la funcion del log
	char *elemento;
	sprintf(elemento, "Dorsal %d ", dorsal); 
	char *msg;
	sprintf(msg, "He entrado a la tarima %d, os vais a enterar.", atletas[pos].tarima_asignada);   //Modificar cuando utilicemos 2 tarimas
	writeLogMessage(arg, msg);
	//calculo del comportamiento del atleta
	
	//tenemos al atleta en la cola
	
	do{
		
		
		
		
	
	}while(atleta[pos].ha_competido==0);
	//fin del atleta en la cola

	// El atleta llega a la tarima y espera 4 segundos para realiza su levantamiento.

	printf("El atleta %i se prepara para realizar el levantamiento.\n", dorsal);
	sleep(4);

	// Comportamiento del atleta: comprobación del estado de salud.

	
	if (estado_salud<=15)
	{
		printf("El atleta %i no puede realizar el levantamiento por problemas de deshidratación.\n", dorsal);
		msg = "No puedo realizar el levantamiento por problemas de deshidratación.";
		writeLogMessage(arg, msg);
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


void *accionesTarima (void *arg){
	int numero = *(int*)arg;
	int lugar;
	char *id;
	char *msg;
	do{

		// Se calcula lo que le sucede al atleta y se guarda en el fichero log la hora a la que realizó el levantamiento.
	
	
	
	
	
	
		//Hay que sacar el atleta de la patata esa
	
	
	
	
	
	
	
		int comportamiento = calculaAleatorios(1,10); 	//Numero aleatorio para calcular el comportamiento
	
		if(comportamiento <=8) {//movimiento bueno
			int tiempo = calculaAleatorios(2,6);
			sleep(tiempo);
		
			sprintf(id, "Juez %d", numero);    //La hora la indica la funcion del log
			sprintf(msg, "El dorsal %d hizo un levantamiento válido", atletas[lugar].id);
			printf("%s: %s", id, msg);
////////////////////////////////ATENCION CON LOS SEMAFOROS DE ESCRITURA///////////////////////////////////////			
			pthread_mutex_lock(&semaforo_escribe);
				writeLogMessage(id, msg);
			pthread_mutex_unlock(&semaforo_escribe);
		
			int puntuacion = calculaAleatorios(60,300);
		
			atletas[lugar].puntuacion = puntuacion;
			sprintf(msg, "Califico el movimiento con la puntuación de: %d.\n", puntuacion);
			printf("%s: %s", id, msg);
		
			//Escribir en el log la puntuacion ganada por el atleta
			pthread_mutex_lock(&semaforo_escribe);
			writeLogMessage(id, msg);
			pthread_mutex_unlock(&semaforo_escribe);
			
		} else if(comportamiento == 9) {//indumentaria fea
			int tiempo = calculaAleatorios(1,4);
			sleep(tiempo);
			char *msg = "Movimiento nulo por incumplimiento de normas";//------------------->REDEFINICION 
		//	writeLogMessage(atletas[posicion].id, msg);	//Duda sobre si poner id del atleta o la tarima 
			printf("%s\n", msg);
			int puntuacion = 0;
		} else {//alfeñique
			int tiempo = calculaAleatorios(6,10);
			sleep(tiempo);
			char *msg = "Movimiento nulo por falta de fuerzas"; //------------------->REDEFINICION    
		//	writeLogMessage(atletas[posicion].id, msg);	//Duda sobre si poner id del atleta o la tarima 
			printf("%s\n", msg);
			int puntuacion = 0;
		//	atletas[posicion].puntuacion = puntuacion;
			printf("El juez califica el movimiento como nulo, por lo tanto la puntuación es: %i.\n", puntuacion);
		}
	
		if(calculaAleatorios(0,10) == 1) {		//calcula si el atleta necesita beber o no
			atletas[lugar].necesita_beber=1;
			sprintf(id, "Juez %d", numero);
			sprintf(msg, "Dorsal %d Necesitas ir a beber", atletas[lugar].id);   
			writeLogMessage(id, msg);
		}
	
		//Finaliza el atleta que esta participando
		punteroTarima[numero-1].descansa++;
		// Se comprueba si al juez le toca descansar (cada 4 atletas 10 segundos)
		if(punteroTarima[numero-1].descansa == 4) {
			char *msg = "Inicio de descanso"; //------------------->REDEFINICION 
			writeLogMessage(arg, msg);	//supongo que pasamos por agumento el id de la tarima
			sleep(10);
			char *msg = "Fin de descanso"; //------------------->REDEFINICION 
					pthread_mutex_lock(&semaforo_escribe);
					writeLogMessage(arg, msg);	//supongo que pasamos por agumento el id de la tarima
			pthread_mutex_unlock(&semaforo_escribe);
			descansoTarima = 0;
		}
		// 12. Volvemos al paso 1 y buscamos el siguiente (siempre priorizando entre los atletas asignados a dicha tarima).
		//para la parte opcional, con el malloc reservo espacio para muchos mas, en plan 300, y no con eso quiero decir q vaya  a permitir entrarl a todos, sino que hay espacio en memoria
	}while(finaliza==0);
}

void finalizaCompeticion (int sig){
	finaliza = 1;
	printf("Has pulsado finalizar competicion\n");
	//Parar de recibir señales!!!!
			pthread_mutex_lock(&semaforo_escribe);
			writeLogMessage("", "Se acabó este suplicio");
			pthread_mutex_unlock(&semaforo_escribe);
	//Y se realiza todo lo necesario para terminar con los atletas activos
	//PRUEBA para ir viendo como están en el momento de la petición:
	for (int i=0;i<maxAtletas;i++) {
		//CANCELAR LOS HILOS QUE ESTÉN ACTIVOS INCLUYENDO AL PRINGAO QUE ESTÁ EN LA FUENTE
	}
	sleep(3);
	printf("Te mostraré los resultados\n");//
	
	char *msg[60];
	sprintf(msg, "Total atletas tarima 1: %d", punteroTarima[0].contador);
			pthread_mutex_lock(&semaforo_escribe);
			writeLogMessage(arg, msg);
			pthread_mutex_unlock(&semaforo_escribe);
	
	sprintf(msg, "Total atletas tarima 2: %d", punteroTarima[1].contador);
			pthread_mutex_lock(&semaforo_escribe);
			writeLogMessage(arg, msg);
			pthread_mutex_unlock(&semaforo_escribe);
	
	/*Hay que usar el nuevo podium
	
	sprintf(msg, "Primer clasificado: Atleta %d con %d puntos",podio[0].id, podio[0].puntuacion );		
	writeLogMessage(arg, msg);
	
	sprintf(msg, "Segundo clasificado: Atleta %d con %d puntos",podio[1].id, podio[1].puntuacion );		
	writeLogMessage(arg, msg);
	
	sprintf(msg, "Tercer clasificado: Atleta %d con %d puntos",podio[2].id, podio[2].puntuacion );		
	writeLogMessage(arg, msg);
	
	*/
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

