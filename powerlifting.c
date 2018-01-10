#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>


// Definición de constantes.

#define ATLETAS 10


/* Declaración de las variables globales. */

// Semáforos y condiciones.

pthread_mutex_t semaforo_atletas, semaforo_fuente;
pthread_cond_t condicion; // Condición para la fuente.

// Contador de atletas.

int contador_atletas;
int descansoTarima1; // Contador de atletas que han participado en la tarima para empezar a descansar.
int descansoTarima2;

// Lista de 10 atletas con sus datos.

struct atletas_competicion 
{
	int id;
	int ha_competido;
	int tarima_asignada;
	int puntuacion; 
	int necesita_beber;
	pthread_t atleta;
};

struct atletas_competicion atletas[ATLETAS];

// Fichero.

FILE *ficheroLog;
char *mensaje;

// Puntuación del podio formado por los tres competidores que obtengan más puntuación.

int podio[3]; 

// Identificador de los 3 mejores atletas.

int ids_mejores[3]; //no tengo claro porque hay un podio y un registro de los 3 mejores ¿no es lo mismo?

// Bandera de la fuente para saber si está vacía (0) u ocupada (1).

int fuente;

// Bandera para finalizar cuando sea igual a 1.

int finalizar;


/* Declaración de las funciones. */

int CalculaAleatorios (int , int );
void EscribirMensajeLog (char* , char* );
void InicializarCompeticion ();
int HaySitioEnCompeticion ();
void NuevoCompetidor (int );
void FinalizarCompeticion (int );
void *AccionesAtleta (void* );
void *AccionesTarima (void* );


/* Función principal. */

int main (int argc, char *argv[]) 
{
	// Se inicializan las señales para las tarimas y para finalizar la competición, además de comprobar si hay errores.

	if (signal(SIGUSR1, NuevoCompetidor)==SIG_ERR) 
	{
		perror("Error en la llamada a la señal SIGUSR1.\n");
		exit(-1);
	}

	if (signal(SIGUSR2, NuevoCompetidor)==SIG_ERR) 
	{
		perror("Error en la llamada a la señal SIGUSR2.\n");
		exit(-1);
	}

	if (signal(SIGINT, FinalizarCompeticion)==SIG_ERR) 
	{
		perror("Error en la llamada a la señal SIGINT.\n");
		exit(-1);
	}

	// Se inicializan los semáforos y la condición, además de comprobar si hay errores.

	if (pthread_mutex_init(&semaforo_atletas, NULL)!=0)
	{
		perror("Error en la creación del semáforo de los atletas.\n");
		exit(-1);
	}

	if (pthread_mutex_init(&semaforo_fuente, NULL)!=0)
	{
		perror("Error en la creación del semáforo de la fuente.\n");
		exit(-1);
	}

	if (pthread_cond_init(&condicion, NULL)!=0)
	{
		perror("Error en la creación de la condición.\n");
		exit(-1);
	}

	// Se inicializan los contadores de atletas, la fuente y finalizar.

	contador_atletas=0;
	descansoTarima1=0;
	descansoTarima2=0;
	fuente=0;
	finalizar=0;

	srand(time(NULL)); // Se genera la semilla aleatoria en función de la hora del sistema.

	// Se inicializan los datos de los atletas.

	InicializarCompeticion();

	// Se incializan las tarimas y se crean los hilos.

	pthread_t tarima_1, tarima_2;
	pthread_create(&tarima_1, NULL, AccionesTarima, (void*)1);
	pthread_create(&tarima_2, NULL, AccionesTarima, (void*)2);

	// Se crea el fichero log.

	FILE *ficheroLog=fopen("registroTiempos.log", "w");

	// Identificador del campeonato para enviar las señales.

	printf("El pid del campeonato es: %d.\n", getpid());

 	// Se espera por las señales. NOTA:cuando recibo sigingt, pongo finalizar a 1, para que al finalizar espere a que termine la cola.

	while(1)
	{
		
	}


/*	
	// Visualización final de la competición.

	for (int i=0;i<10;i++) 
	{
		printf("Atleta %d: ha competido %d, su tarima actual es %d y necesita beber %d.\n", atletas[i].id, atletas[i].ha_competido, atletas[i].tarima_asignada, atletas[i].necesita_beber);
	}
*/

}


/* Definición de las funciones. */

int CalculaAleatorios (int min, int max)
{
	return rand() % (max-min+1) + min;
}

void EscribirMensajeLog (char *identificador, char *mensaje)
{
	// Se calcula la hora actual.

	time_t now=time(0);
	struct tm *tlocal=localtime(&now);
	char stnow[19];
	strftime (stnow, 19, "%d/ %m/ %y %H: %M: %S ", tlocal);

	// Se escribe en el fichero log llamado registroTiempos.log.

	ficheroLog=fopen("registroTiempos.log", "a");
	fprintf(ficheroLog, "[%s] %s: %s\n", stnow, identificador, mensaje) ;
	fclose(ficheroLog);
}

void InicializarCompeticion () 
{
	for (int i=0; i<ATLETAS;i++)
	{
		atletas[i].id=0;
		atletas[i].ha_competido=0;
		atletas[i].tarima_asignada=0;
		atletas[i].puntuacion=0; 
		atletas[i].necesita_beber=0;
	}
} // Es posible que no lo necesitemos ya que se inicializan al crearse un nuevo competidor.

int HaySitioEnCompeticion ()
{
	for (int i=0;i<10;i++) 
	{
		if (atletas[i].id==0) 
		{
			return i;
		}
	}
	return -1;
} // Devuelve -1 en caso de no haber sitio y sino nos da el primer hueco que encuentre.

void NuevoCompetidor (int sig)
{
	// Primero se comprueba si hay sitio en la competición y cuál es.

	if (HaySitioEnCompeticion()!=-1)
	{
		printf("El nuevo atleta va a ser incrito en la posición: %d.\n", HaySitioEnCompeticion()+1);

		// Se añaden los atletas de uno en uno (bloqueando el semáforo) y se le asignan los datos al atleta.

		if (pthread_mutex_lock(&semaforo_atletas)!=0)
		{
			perror("Error en el bloqueo del semáforo de los atletas.\n");
			exit(-1);
		}

		contador_atletas++;
		int posicion=HaySitioEnCompeticion();
		atletas[posicion].id=contador_atletas;
		
		// Se establecen las señales para asignar las tarimas, además se comprueba si fallan.

		if (sig==SIGUSR1)
		{
			if (signal(SIGUSR1, NuevoCompetidor)==SIG_ERR) 
			{
				perror("Error en la llamada a la señal SIGUSR1.\n");
				exit(-1);
			}
			atletas[posicion].tarima_asignada=1;	
			printf("El atleta %i se prepara para ir a la tarima 1.\n", posicion+1);
		}
	
		if (sig==SIGUSR2)
		{
			if (signal(SIGUSR2, NuevoCompetidor)==SIG_ERR) 
			{
				perror("Error en la llamada a la señal SIGUSR2.\n");
				exit(-1);
			}
			atletas[posicion].tarima_asignada=2;
			printf("El atleta %i se prepara para ir a la tarima 2.\n", posicion+1);
		}

	
		// Se especifica si necesita beber y se crea el hilo.

		atletas[posicion].necesita_beber=0;
		pthread_create(&atletas[posicion].atleta, NULL, AccionesAtleta, (void *)&atletas[posicion].id);

		// Se desbloquea el semáforo, además se comprueba si falla.

		if (pthread_mutex_unlock(&semaforo_atletas)!=0)
		{
			perror("Error en el desbloqueo del semáforo de los atletas.\n");
			exit(-1);
		}
	}
	else
	{ 
		printf("Ya están inscritos y participando 10 atletas, de momento no puedes participar.\n");
	}
}


void FinalizarCompeticion (int sig)
{	
	if (signal(SIGINT, FinalizarCompeticion)==SIG_ERR) 
	{
		perror("Error en la llamada a la señal SIGINT.\n");
		exit(-1);
	}
	printf("Has pulsado finalizar competición.\n");
		//Parar de recibir señales!!!!
	//se realiza todo lo necesario para terminar con los atletas activos


//PRUEBA para ir viendo como están en el momento de la petición:
	for (int i=0;i<ATLETAS;i++) {
		printf("Atleta %d: ha competido %d, su tarima actual es %d y necesita beber %d\n",atletas[i].id,atletas[i].ha_competido,atletas[i].tarima_asignada,atletas[i].necesita_beber);
	}//


	sleep(3);
	printf("Te mostraré los resultados.\n");
	signal(SIGTERM, SIG_DFL);
	raise(SIGTERM);
}


void *AccionesAtleta (void *arg)
{
	// 1. Guardar en el log la hora de entrada a la cola que accede. 
	mensaje = "He entrado a la tarima 1";   //Modificar cuando utilicemos 2 tarimas
//	EscribirMensajeLog(atletas[posicion].id, mensaje); 

	// 2. Guardar en el log el tipo de tarima al que accede.


	// El atleta llega a la tarima y espera 4 segundos para realiza su levantamiento.

	printf("El atleta %i se prepara para realizar el levantamiento.\n", *(int*) arg);
	sleep(4);

	// Comportamiento del atleta: comprobación del estado de salud.

	int estado_salud=CalculaAleatorios(0,100);
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


void *AccionesTarima (void *arg)
{
 	// 1. Busca el primer atleta para competir, que será el que más tiempo lleve a la espera. Si no hay nadie esperando en esta tarima, busca a otro atleta al que se le asignó la otra tarima, para echar una mano).??? NOTA:si tarima 2 esta libre y no tiene nadie en su cola(primera comprobar que no tiene nadie en la suya IMPORTANTE), puede ayudar a tarima 1, y viceversa.

	// 2. Cambiamos el flag . (NOTA: se pone a 1) ¿QUÉ FLAG?


	// Se calcula lo que le sucede al atleta y se guarda en el fichero log la hora a la que realizó el levantamiento.

	int movimiento=CalculaAleatorios(1,10); // Número aleatorio para calcular el comportamiento.
	if (movimiento<=8)
	{
		int tiempo=CalculaAleatorios(2,6);
		sleep(tiempo); // Tiempo del levantamiento del atleta.
		printf("Movimiento válido.\n");

		mensaje = "He hecho un levantamiento valido en: ";  //Añadir el tiempo.
//		EscribirMensajeLog(atletas[posicion].id, mensaje);  //Duda sobre si poner id del atleta o la tarima 
		int puntuacion=CalculaAleatorios(60,300);
//		atletas[posicion].puntuacion = puntuacion;
		printf("El juez califica el movimiento con la puntuación de: %i.\n", puntuacion);
	}
	else
	{
		if (movimiento==9)
		{

			int tiempo = CalculaAleatorios(1,4);
			sleep(tiempo);

			mensaje = "Movimiento nulo por incumplimiento de normas.";    
//			EscribirMensajeLog(atletas[posicion].id, mensaje);	//Duda sobre si poner id del atleta o la tarima 
			printf("%s\n", mensaje);

			int puntuacion = 0;
//			atletas[posicion].puntuacion = puntuacion;
			printf("El juez califica el movimiento como nulo, por lo tanto la puntuación es: %i.\n", puntuacion);
		}
		else
		{

			int tiempo = CalculaAleatorios(6,10);
			sleep(tiempo);
	
			mensaje = "Movimiento nulo por falta de fuerza.";    
//			EscribirMensajeLog(atletas[posicion].id, mensaje);	//Duda sobre si poner id del atleta o la tarima
			printf("%s\n", mensaje);

			int puntuacion = 0;
//			atletas[posicion].puntuacion = puntuacion;
			printf("El juez califica el movimiento como nulo, por lo tanto la puntuación es: %i.\n", puntuacion);
		}
	}

	// Se calcula si el atleta necesita beber o no.	NOTA:si tiene q ir a beber agua, pongo el flag al 2 ¿Cuál: variable fuente o necesita_beber?

	if(CalculaAleatorios(1,10) == 1) 
	{		
//		atletas[posicion].necesita_beber=1;	
	}
	
	// Finaliza el atleta que está participando.
	
//	atletas[posicion].ha_competido=1;

	// Se comprueba si al juez le toca descansar (cada 4 atletas 10 segundos).

	descansoTarima1 ++;
	if(descansoTarima1 == 4) 
	{
		sleep(10);
		
		descansoTarima1 = 0;
	}

	

	// 10. Incrementamos el contador de atletas que han competido.

	// 12. Volvemos al paso 1 y buscamos el siguiente (siempre priorizando entre los atletas asignados a dicha tarima).

	// para la parte opcional, con el malloc reservo espacio para muchos mas, en plan 300, y no con eso quiero decir q vaya  a permitir entrarl a todos, sino que hay espacio en memoria

}

