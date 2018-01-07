#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


/* Declaración de las variables globales. */

// Semáforos y condiciones.

pthread_mutex_t semaforo_atletas, semaforo_tarimas; // ¿Vale sólo con uno para las dos funciones o es uno para cada función?
pthread_cond_t condicion; // Condición para la prioridad.

// Contador de atletas.

int contador_atletas;

// Lista de 10 atletas con sus datos.

struct atletas_competicion 
{
	int id;
	int ha_competido;
	int tarima_asignada;
	int juzgando; // Nota.
	int necesita_beber;
	pthread_t atleta;
};

struct atletas_competicion atletas[10];

// Fichero.

FILE *ficheroLog;

// Puntuación del podio formado por los tres competidores que obtengan más puntuación.

int podio[3];

// Identificador de los 3 mejores atletas.

int ids_mejores[3];

// Bandera de la fuente para saber si está vacía (0) u ocupada (1).

int fuente;

// Bandera para finalizar cuando sea igual a 1.

int finalizar;


/* Declaración de las funciones. */

int CalculaAleatorios (int , int );
void EscribirMensajeLog (char* , char* );
void NuevoCompetidor (int );
void *AccionesAtleta (void* );
void *AccionesTarima (void* );


/* Función principal. */

int main (int argc, char *argv[]) 
{
	int i;

	srand(time(NULL)); // Se genera la semilla aleatoria en función de la hora del sistema.

	// 1. signal SIGUSR1, nuevoCompetidor a tarima 1 ¿Cómo se hace esto de las señales, con gettid() y kill o cómo?
	// 2. signal SIGUSR2, nuevoCompetidor a tarima 2
	// 3. signal SIGINT, para terminar la competición. NOTA:cuando recibo sigingt, pongo finalizar a 1, para que al finalizar espere a que termine la cola.

	// Inicializar semáforos y condiciones, además de comprobar si hay errores.

	if (pthread_mutex_init(&semaforo_atletas, NULL)!=0)
	{
		perror("Error en la creación del semáforo de los atletas.\n");
		exit(-1);
	}

	if (pthread_mutex_init(&semaforo_tarimas, NULL)!=0)
	{
		perror("Error en la creación del semáforo de las tarimas.\n");
		exit(-1);
	}

	if (pthread_cond_init(&condicion, NULL)!=0)
	{
		perror("Error en la creación de la condición.\n");
		exit(-1);
	}

	// Inicializar contador de atletas, fuente y finalizar.

	contador_atletas=0;
	fuente=0;
	finalizar=0;

	// Inicializar datos de atletas.

	for (i=0; i<10;i++)
	{
		atletas[i].id=0;
		atletas[i].ha_competido=0;
		atletas[i].tarima_asignada=0;
		atletas[i].juzgando=0; 
		atletas[i].necesita_beber=0;
	}

	// Incializar las tarimas y creación de los hilos.

	pthread_t tarima_1, tarima_2;
	pthread_create(&tarima_1, NULL, AccionesTarima, (void*) 1); // Pongo 1 y 2 por identificarlas, pero no creo que eso sea el parámetro.
	pthread_create(&tarima_2, NULL, AccionesTarima, (void*) 2);

	// Inicializar fichero de log. ???? NOTA:FILE * fd = fopen ["nombre" , "w"]; para q no falle lo del log, tiene q tener un archivo creado (w)
	FILE *fd=fopen["registroTiempos.log", "w"];
	
	// Espera la señal 10 veces (el número de atletas) para escoger la tarima 1 o la tarima 2.¿Cómo se hace esto de las señales, con gettid() o cómo?

	for (i=0; i<10; i++)
	{
		signal(SIGUSR1, NuevoCompetidor);
		kill(tarima_1, SIGUSR1);
		signal(SIGUSR2, NuevoCompetidor);
		kill(tarima_2, SIGUSR2);
	}

	return 0;
}


/* Definición de las funciones. */

int CalculaAleatorios (int min, int max) // Función que calcula números aleatorios entre los dos valores que admite como argumento.
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

void NuevoCompetidor (int sig)
{
	int i, sitio=-1;

	// Se comprueba si hay sitio en el campeonato y cuál es la posición de ese sitio.

	for (i=0;i<10;i++) 
	{
		if (atletas[i].id==0 && sitio==-1) // La segunda condición evita que se sobreescriba el sitio, dando así sólo el primero libre.
		{
			sitio=i;
		}
	}

	if (sitio!=-1)
	{
		printf("Posición para nuevo atleta: %i.\n", sitio+1);
		
		// Se añade un atleta y se le asignan los datos. (2.a.i.???), NOTA:al comprobar algo de la cola, hay q bloquarla primero , acceder y desbloquearla

		contador_atletas++;
		atletas[sitio].id=contador_atletas;
		atletas[sitio].ha_competido=0; // ¿Por qué hay que ponerlo a 0 otra vez?

		// Se establecen las señales, se comprueba si fallan y se asignan las tarimas.

		if (sig==SIGUSR1)
		{
			if (signal(SIGUSR1, NuevoCompetidor)==SIG_ERR) 
			{
				perror("Error en la llamada a la señal SIGUSR1.\n");
				exit(-1);
			}
			atleta[sitio].tarima_asignada=1;	
		}
	
		if (sig==SIGUSR2)
		{
			if (signal(SIGUSR2, NuevoCompetidor)==SIG_ERR) 
			{
				perror("Error en la llamada a la señal SIGUSR2.\n");
				exit(-1);
			}
			atleta[sitio].tarima_asignada=2;
		}

		if (sig==SIGINT)
		{
			if (signal(SIGINT, NuevoCompetidor)==SIG_ERR) 
			{
				perror("Error en la llamada a la señal SIGINT.\n");
				exit(-1);
			}
			finalizar=1;
		}
		
		// Se especifica si necesita beber y se crea el hilo.

		atletas[sitio].necesita_beber=0;
		pthread_create(&atletas[sitio].atleta, NULL, AccionesAtleta, (void *)&atletas[sitio].id);
	}
	else
	{
		printf("La inscripción al campeonato está completa. No se puede inscribir aún.\n");
	}
}

void *AccionesAtleta (void *arg)
{
	int estado_salud;
	// 1. Guardar en el log la hora de entrada a la cola que accede. 
	// 2. Guardar en el log el tipo de tarima al que accede.

	sleep(4); // Si el atleta llega a la tarima espera 4 segundos para realizar su levantamiento.

	// Comportamiento del atleta: comprobación del estado de salud.

	do
	{ 
		estado_salud=CalculaAleatorios(0,100);
		if (estado_salud<=15)
		{
			printf("El atleta %i no puede realizar el levantamiento por problemas de deshidratación.\n", arg);
			// Salir de la cola ???. 3.a. Si no llega a realizar el levantamiento, no llega a subir a la tarima y se escribe en el log, se daría fin al hilo Atleta y se liberaría espacio en la cola.
		}	
		else
		{
			sleep (3);
		}
	} while (1); // Poner condición de esperar en la cola.???

	// Se bloquea el semáforo para que los competidores accedan de uno en uno a la cola.
	
	pthread_mutex_lock(&semaforo_atletas); // ¿Incluir comprobación de error, es decir, si devuelve 0?

	// 4. Si ya ha salido a competir, debemos esperar a que termine.
	// 5. Guardamos en el log la hora a la que ha finalizado su levantamiento.
	// 6. Fin del hilo del atleta. NOTA: el hilo se sigue ejecutando aun asi este el tio ataascado en la fuente
	pthread_mutex_unlock(&semaforo_atletas);


}

void *AccionesTarima (void *arg)
{
	int movimiento, espera, puntuacion=0, agua;
 	// 1. Busca el primer atleta para competir, que será el que más tiempo lleve a la espera. Si no hay nadie esperando en esta tarima, busca a otro atleta al que se le asignó la otra tarima, para echar una mano).??? NOTA:si tarima 2 esta libre y no tiene nadie en su cola(primera comprobar que no tiene nadie en la suya IMPORTANTE), puede ayudar a tarima 1, y viceversa.



	// Se bloquea el semáforo para que el competidor que está a la espera acceda a sólo una de las tarimas.
	
	pthread_mutex_lock(&semaforo_tarimas);

	// 2. Cambiamos el flag . (NOTA: se pone a 1) ¿QUÉ FLAG?

	// Se calcula lo que le sucede al atleta.
	// 4. Guardamos en el log la hora a la que realizó el levantamiento.

	movimiento=CalculaAleatorios(0,100);
	if (movimiento<=80)
	{
		printf("Movimiento válido.\n");
		espera=CalculaAleatorios(2,6);
		sleep(espera);	// Tiempo del levantamiento del atleta.
		puntuacion=CalculaAleatorios(60,300);
		printf("El juez califica el movimiento con la puntuación de: %i.\n", puntuacion);
	}

	if (movimiento>80 && movimiento<=90)
	{
		printf("Movimiento nulo por incumplimiento de la normativa: El atleta ha accedido a la tarima con indumentaria incorrecta.\n");
		espera=CalculaAleatorios(1,4);
		sleep(espera);	// Tiempo del levantamiento del atleta.
		printf("El juez califica el movimiento como nulo, por lo tanto la puntuación es: %i.\n", puntuacion);
	}

	if (movimiento>90 && movimiento<=100)
	{
		printf("Movimiento nulo: Falta de fuerza del atleta.\n");
		espera=CalculaAleatorios(6,10);
		sleep(espera);	// Tiempo del levantamiento del atleta.
		printf("El juez califica el movimiento como nulo, por lo tanto la puntuación es: %i.\n", puntuacion);
	}

	// 6. Calculamos si el atleta tiene que ir a beber agua (10%) y cambiamos el flag. NOTA:si tiene q ir a beber agua, pongo el flag al 2 ¿Cuál: variable fuente o necesita_beber?

	agua=CalculaAleatorios(0,100);
	if (agua<=10)
	{
		printf("El juez ordena al atleta beber agua.\n");
	}


	

	// 7. Guardamos en el log la hora de fin del levantamiento del atleta.
	// 8. Guardamos el motivo del fin del levantamiento y su puntuación en caso de ser válido (válido, nulo por indumentaria o nulo por fallo del atleta).
	// 9. Cambiamos el flag de ha_competido.
	// 10. Incrementamos el contador de atletas que han competido.
	// 11. Mira si le toca descansar a sus jueces (cada 2 atletas descansa 10s).
	// 12. Volvemos al paso 1 y buscamos el siguiente (siempre priorizando entre los atletas asignados a dicha tarima).

	pthread_mutex_unlock(&semaforo_tarimas);

	// para la parte opcional, con el malloc reservo espacio para muchos mas, en plan 300, y no con eso quiero decir q vaya  a permitir entrarl a todos, sino que hay espacio en memoria

}
