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


// Definición de constantes.
#define MAXIMOATLETAS 10
#define NUMEROTARIMAS 2



/* Declaración de las variables globales. */


int contadorAtletas; // Contador de atletas que participan a lo largo del campeonato.


// Semáforos y condiciones.
pthread_mutex_t semaforo_atletas; // Semáforo para la entrada de nuevos atletas.
pthread_mutex_t semaforo_tarimas; // Semáforo para la cola de las tarimas.
pthread_mutex_t semaforo_escribir; // Semáforo para escritura en el log.
pthread_mutex_t semaforo_fuente; // Semáforo que controla el acceso a la fuente.
pthread_cond_t condicion; // Condición para la fuente.


// Estructura de punteros para la lista de atletas con sus datos.
struct atletasCompeticion 
{
	int id;
	int ha_competido;
	int tarima_asignada;
	int puntuacion;
	int necesita_beber;
	int calentamiento;
	pthread_t atleta;
};
struct atletasCompeticion *atletas;


// Estructura de punteros para las tarimas con sus datos.
struct tarimasCompeticion
{
	int id;
	int descansa; // Cuenta hasta cuatro para descansar y después se pone a cero.
	int contador; // Cuenta todos los atletas que han pasado por la tarima.
	pthread_t tatami;
};
struct tarimasCompeticion *punteroTarimas;


// Fichero.
FILE *registro;
char *nombreArchivo = "registroTiempos.log";


int podio[2][3]={{0,0,0},{0,0,0}}; // Matriz del podio donde se incluye tanto la puntuación como el identificador de los tres mejores atletas.


// Variables para el número máximo de atletas y el número de tarimas.
int maxAtletas;
int numTarimas;


// Fuente.
struct atletasCompeticion colaFuente[1]; // Estructura para guardar los datos del que espera para beber en la fuente.
int estadoFuente; // Bandera de la fuente para saber si está vacía (0) u ocupada (1).


int finalizar; // Bandera para finalizar cuando sea igual a 1.



/* Declaración de las funciones. */


int calculaAleatorios(int min, int max);

void inicializaCampeonato(int maxAtletas, int numTarimas);
int haySitioEnCampeonato(); // Para saber si hay sitio (y si lo hay nos dice el primer hueco) para que entre un atleta a competir.
void nuevoCompetidor(int sig); // REVISAR para múltiples tarimas.
void finalizaCompeticion(int sig);

void *accionesAtleta(void*); // El argumento que le pasaremos es el atleta.
void *accionesTarima(void*); // El argumento que le pasaremos es la tarima.

void  writeLogMessage(char *id, char *msg);

void eliminaAtleta(int pos);
void meteEnFuente(int pos);


/* Función principal. */


int main (int argc, char *argv[]) {
	// Parte opcional --> Asignación estática de recursos.
	maxAtletas = MAXIMOATLETAS; // Se inicia con el máximo de atletas por defecto.
	numTarimas = NUMEROTARIMAS; // Se inicia con el número de tarimas por defecto.	
	
	
	// Se crea el fichero log y se comprueba si hay errores.
	registro = fopen (nombreArchivo,"w");
	if(registro==NULL)
	{
		perror("Error en la creación del fichero.\n");
		exit(-1);
	}
	else
	{
		fclose(registro); // Se cierra para guardar los datos.
		printf("El pid del campeonato es %d.\n", getpid());
		writeLogMessage("Árbitro", "Comienza el campeonato de levantamiento de pesas, cuidado con los pinreles."); // Aquí no se necesita semáforo porque todavía no hay hilos creados.


		// Si se introducen argumentos por la terminal el primero será para el máximo de atletas y el segundo para el número de tarimas.
		if(argc==2) maxAtletas=atoi(argv[1]);
		if(argc==3)
		{
			 maxAtletas=atoi(argv[1]);
			 numTarimas=atoi(argv[2]);
		}
		

		// Se modifican los comportamientos de las señales para inscribir a los atletas y para finalizar la competición, además de comprobar si hay error.
		if(signal(SIGUSR1,nuevoCompetidor)==SIG_ERR) 
		{
			perror("Error en la llamada a la señal SIGUSR1.\n");
			exit(-1);
		}
		if(signal(SIGUSR2,nuevoCompetidor)==SIG_ERR) 
		{
			perror("Error en la llamada a la señal SIGUSR2.\n");
			exit(-1);
		}
		if(signal(SIGINT,finalizaCompeticion)==SIG_ERR) 
		{
			perror("Error en la llamada a la señal SIGINT.\n");
			exit(-1);
		}

		// Se reserva espacio en memoria para los punteros de las tarimas y los atletas.
		punteroTarimas = (struct tarimasCompeticion*)malloc(sizeof(struct tarimasCompeticion)*numTarimas);
		atletas = (struct atletasCompeticion*)malloc(sizeof(struct atletasCompeticion)*maxAtletas);
		
		// Se inicializan los semáforos y la condición, además de comprobar si hay errores.
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
	 	
	 	if (pthread_mutex_init(&semaforo_fuente, NULL)!=0)
		{
			perror("Error en la creación del semáforo de la fuente.\n");
			exit(-1);
	 	}
	 	
	 	if (pthread_mutex_init(&semaforo_escribir, NULL)!=0)
		{
			perror("Error en la creación del semáforo para escribir en el fichero.\n");
			exit(-1);
	 	}
	 	
	 	if (pthread_cond_init(&condicion, NULL)!=0)
		{
			perror("Error en la creación de la condición.\n");
			exit(-1);
	 	}
	 	
		// Con la función se inicializan el contador de atletas, la fuente, finalizar, los datos de los atletas y las tarimas y se crean los hilos para la tarimas.
		inicializaCampeonato(maxAtletas, numTarimas);

		srand (time(NULL)); // Semilla para generar números aleatorios.

		while(finalizar==0) // Bucle para recibir señales (permanece a la espera).
		{
			pause();
		}
	}
	
	return 0;	
}



/* Implementación de las funciones. */


void inicializaCampeonato(int maxAtletas, int numTarimas) 
{
	int i;

	contadorAtletas=0;
	estadoFuente=0;
	finalizar=0;

	// Se inicializan los datos de los atletas.
	for (i=0; i<maxAtletas; i++) 
	{
		atletas[i].id=0;
		atletas[i].ha_competido=0;
		atletas[i].tarima_asignada=0;
		atletas[i].puntuacion=0;
		atletas[i].necesita_beber=0;
		atletas[i].calentamiento=0;
	}


	// Se inicializan los datos de las tarimas y se crean los hilos.
	for (i=0; i<numTarimas; i++)
	{
		punteroTarimas[i].id=i+1; // Se asigna el número correspondiente a cada tarima.
		punteroTarimas[i].descansa=0;
		punteroTarimas[i].contador=0;
		pthread_create(&punteroTarimas[i].tatami, NULL, accionesTarima, (void*)&punteroTarimas[i].id);
	}
}


int haySitioEnCampeonato() 
{
	int i;

	for (i=0; i<maxAtletas; i++) 
	{
		if (atletas[i].id==0) 
		{
			return i;
		}
	}
	return -1;
} // Devuelve -1 en caso de no haber sitio y si no nos da el primer hueco que encuentre.


void nuevoCompetidor (int sig)
{ 
	int posicion;
	
	if(signal(SIGUSR1,nuevoCompetidor)==SIG_ERR) 
	{
		perror("Error en la llamada a la señal SIGUSR1.\n");
		exit(-1);
	}
	if(signal(SIGUSR2,nuevoCompetidor)==SIG_ERR) 
	{
		perror("Error en la llamada a la señal SIGUSR2.\n");
		exit(-1);
	}

	printf("Un atleta ha solicitado inscribirse...\n");
	

	// Se bloquea el semáforo para que los atletas entren de uno en uno.
	if (pthread_mutex_lock(&semaforo_atletas)!=0)
	{
		perror("Error en el bloqueo del semáforo de los atletas.\n");
		exit(-1);
	}

		// Se comprueba si hay sitio en la competición y cuál es.
		posicion = haySitioEnCampeonato();

		if(posicion!=-1) 
		{
			printf("Vas a ser inscrito, chavalote.\n");
			contadorAtletas++;
			atletas[posicion].id=contadorAtletas;
			atletas[posicion].puntuacion=0;
			
			// Según qué señal se recibe se asigna la tarima correspondiente.
			if (sig == 10)
			{
				atletas[posicion].tarima_asignada=1;
			}
			else if (sig == 12)
			{
				atletas[posicion].tarima_asignada=2;
			}

			atletas[posicion].ha_competido=0;
			atletas[posicion].necesita_beber=0;
			printf("El atleta %d se prepara para ir a la tarima %d.\n", atletas[posicion].id, atletas[posicion].tarima_asignada);
		
			// Se crea el hilo para el atleta.
			pthread_create(&atletas[posicion].atleta, NULL, accionesAtleta, (void *)&atletas[posicion].id);
		} 
		else 
		{ 
			printf("Ya están inscritos y participando %d atletas, de momento no puedes participar.\n", maxAtletas);
		}

	// Se desbloquea el semáforo, además se comprueba si falla.
	if (pthread_mutex_unlock(&semaforo_atletas)!=0)
	{
		perror("Error en el desbloqueo del semáforo de los atletas.\n");
		exit(-1);
	}
	
}


void *accionesAtleta (void *arg)
{ 
	int dorsal = *(int*)arg; // Se convierte el argumento a tipo entero.
	int pos;
	int i;
	int estado_salud;
	char *elemento;
	char *msg;
	
	// Se reserva espacio en memoria.
	elemento = (char*)malloc(sizeof(char)*30);
	msg = (char*)malloc(sizeof(char)*256);
	
	
	// Se calcula la posición del atleta.
	for(i=0; i<maxAtletas; i++)
	{
		if(atletas[i].id==dorsal)
		{
			pos = i;
			break;
		}
	}


	// Se guarda a qué tarima va a competir en el log.	
	sprintf(elemento, "Atleta %d", dorsal); 
	sprintf(msg, "He entrado a la tarima %d, ¡os vais a enterar!", atletas[pos].tarima_asignada);
	printf("%s: %s\n", elemento, msg); // Se imprime el mensaje por pantalla.

	if (pthread_mutex_lock(&semaforo_escribir)!=0)	// Se bloquea el semáforo para que los mensajes entren de uno en uno.
	{
		perror("Error en el bloqueo del semáforo para escribir en el fichero.\n");
		exit(-1);
	}

			writeLogMessage(elemento, msg); // La hora de entrada a la tarima la escribe la función del log.

	if (pthread_mutex_unlock(&semaforo_escribir)!=0) // Se desbloquea el semáforo para que los mensajes entren de uno en uno.
	{
		perror("Error en el desbloqueo del semáforo para escribir en el fichero.\n");
		exit(-1);
	}
	


	// Cálculo del comportamiento del atleta mientras está en la cola esperando para subir a la tarima correspondiente.

	do 
	{
		// Comprobación del estado de salud.
		estado_salud=calculaAleatorios(1,100); // Número aleatorio para calcular el estado de salud.

		if (estado_salud<=15)
		{
			eliminaAtleta(pos); // Se libera la posición del atleta en la cola (se inicializan los datos de nuevo).
			
			// Se escribe en el log.
			sprintf(elemento, "Atleta %d", dorsal); 
			sprintf(msg, "Estoy deshidratado de tanto estrés y no puedo realizar el levantamiento.");
			printf("%s: %s\n", elemento, msg);
			
			if (pthread_mutex_lock(&semaforo_escribir)!=0)	
			{
				perror("Error en el bloqueo del semáforo para escribir en el fichero.\n");
				exit(-1);
			}

				writeLogMessage(elemento, msg);

			if (pthread_mutex_unlock(&semaforo_escribir)!=0) 
			{
				perror("Error en el desbloqueo del semáforo para escribir en el fichero.\n");
				exit(-1);
			}
			pthread_exit(NULL); // Se finaliza el hilo.
		}	
		else
		{
			sleep (3);
		}
	}while (atletas[pos].ha_competido==0); // Fin del atleta en la cola.



	// El atleta llega a la tarima y espera 4 segundos para realizar su levantamiento.

	// Se escribe en el log.
	sprintf(elemento, "Atleta %d", dorsal); 
	sprintf(msg, "Voy a calentar un poco los pies antes de realizar el levantamiento.");
	printf("%s: %s\n", elemento, msg);
			
	if (pthread_mutex_lock(&semaforo_escribir)!=0)	
	{
		perror("Error en el bloqueo del semáforo para escribir en el fichero.\n");
		exit(-1);
	}

		writeLogMessage(elemento, msg);

	if (pthread_mutex_unlock(&semaforo_escribir)!=0) 
	{
		perror("Error en el desbloqueo del semáforo para escribir en el fichero.\n");
		exit(-1);
	}
	sleep(4);
	
	atletas[pos].calentamiento=1;
	
	// Se espera a que termine de competir.
	do
	{
		sleep(1);

	}while(atletas[pos].puntuacion==0 && atletas[pos].ha_competido!=2);
	

	// Se escribe en el log la hora a la que ha finalizado su levantamiento.
	sprintf(elemento, "Atleta %d", dorsal); 
	sprintf(msg, "He finalizado el levantamiento y me duelen los pies.");
	printf("%s: %s\n", elemento, msg);
			
	if (pthread_mutex_lock(&semaforo_escribir)!=0)	
	{
		perror("Error en el bloqueo del semáforo para escribir en el fichero.\n");
		exit(-1);
	}

		writeLogMessage(elemento, msg);

	if (pthread_mutex_unlock(&semaforo_escribir)!=0) 
	{
		perror("Error en el desbloqueo del semáforo para escribir en el fichero.\n");
		exit(-1);
	}



	// Fuente.
	if (atletas[pos].necesita_beber==1)
	{
		estadoFuente++;
		if (estadoFuente==1) // Un atleta en la fuente (sólo para el primero que entra).
		{
			meteEnFuente(pos); // Se registra qué atleta entra en la fuente.
		}
		else // Dos atletas en la fuente.
		{
			estadoFuente=1; // Se indica que sigue ocupada porque se quedaría un atleta dentro esperando a beber.

			// Se escribe en el log.
			sprintf(elemento, "Atleta %d", dorsal); 
			sprintf(msg, "Voy a beber a la fuente, pero ... ¡vaya por Dios! No me toca beber sino apretar el botón.");
			printf("%s: %s\n", elemento, msg);
			if (pthread_mutex_lock(&semaforo_escribir)!=0)	
			{
				perror("Error en el bloqueo del semáforo para escribir en el fichero.\n");
				exit(-1);
			}

				writeLogMessage(elemento, msg);

			if (pthread_mutex_unlock(&semaforo_escribir)!=0) 
			{
				perror("Error en el desbloqueo del semáforo para escribir en el fichero.\n");
				exit(-1);
			}
			
			// Se envía la señal para indicar que un atleta ya ha bebido.	
			if (pthread_cond_signal(&condicion)!=0)	
			{
				perror("Error en el envío de la señal de la fuente.\n");
				exit(-1);
			}	
			meteEnFuente(pos); // Se registra al último atleta que entra en la fuente y que ahora espera para beber.
		}
	}
	else
	{
		pthread_exit(NULL); // Si no necesita beber se finaliza el hilo del atleta.
	}

	// Se libera el espacio de memoria reservado.
	free(elemento);
	free(msg);	
}

void meteEnFuente (int pos)
{
	char *elemento;
	char *msg;
	int dorsal = atletas[pos].id;
	
	// Se reserva espacio en memoria.
	elemento = (char*)malloc(sizeof(char)*30);
	msg = (char*)malloc(sizeof(char)*256);
	
	// Se escribe en el log.
	sprintf(elemento, "Atleta %d", dorsal); 
	sprintf(msg, "Voy a beber a la fuente, pero ... ¡qué lástima! No soy capaz de apretar el botón, no tengo fuerza.");
	printf("%s: %s\n", elemento, msg);
	if (pthread_mutex_lock(&semaforo_escribir)!=0)	
	{
		perror("Error en el bloqueo del semáforo para escribir en el fichero.\n");
		exit(-1);
	}

		writeLogMessage(elemento, msg);

	if (pthread_mutex_unlock(&semaforo_escribir)!=0) 
	{
		perror("Error en el desbloqueo del semáforo para escribir en el fichero.\n");
		exit(-1);
	}
	
	// Se guardan los datos del atleta en la fuente.
	colaFuente[0].id = atletas[pos].id;
	colaFuente[0].tarima_asignada = atletas[pos].tarima_asignada;
	colaFuente[0].ha_competido = atletas[pos].ha_competido;
	colaFuente[0].puntuacion = atletas[pos].puntuacion;
	colaFuente[0].necesita_beber = atletas[pos].necesita_beber;
	
	eliminaAtleta(pos); // Se libera la posición del atleta en la cola.
	

	// Semáforo de la fuente.
	if (pthread_mutex_lock(&semaforo_fuente)!=0)	
	{
		perror("Error en el bloqueo del semáforo para la fuente.\n");
		exit(-1);
	}
		
		if (pthread_cond_wait(&condicion, &semaforo_fuente)!=0) // Se espera la condición para desbloquear el semáforo de la fuente.
		{
			perror("Error en la espera de la condición del semáforo para la fuente.\n");
			exit(-1);
		}

	
		// Se escribe en el log que el atleta ya ha bebido.
		sprintf(elemento, "Atleta %d", dorsal); 
		sprintf(msg, "Ya he bebido, pero el agua está caliente como en mi gimnasio.");
		printf("%s: %s\n", elemento, msg);
		if (pthread_mutex_lock(&semaforo_escribir)!=0)	
		{
			perror("Error en el bloqueo del semáforo para escribir en el fichero.\n");
			exit(-1);
		}

			writeLogMessage(elemento, msg);

		if (pthread_mutex_unlock(&semaforo_escribir)!=0) 
		{
			perror("Error en el desbloqueo del semáforo para escribir en el fichero.\n");
			exit(-1);
		}
	
		pthread_exit(NULL); // Finaliza el hilo del atleta que ha bebido.
	
	if (pthread_mutex_unlock(&semaforo_fuente)!=0)	
	{
		perror("Error en el desbloqueo del semáforo para la fuente.\n");
		exit(-1);
	}

	// Se libera el espacio de memoria reservado.
	free(elemento);
	free(msg);	
}

void eliminaAtleta (int pos)
{
	atletas[pos].id=0;
	atletas[pos].ha_competido=0;
	atletas[pos].tarima_asignada=0;
	atletas[pos].puntuacion=0;
	atletas[pos].necesita_beber=0;
}


void *accionesTarima (void *arg)
{
	int numero = *(int*)arg; // Se convierte el argumento a tipo entero.
	int comportamiento;
	int tiempo;
	int puntuacion;
	char *id;
	char *msg;
	int i;
	int atleta_cogido;//guarda la posicion del atleta en la cola de las tarimas
	int atl_tar[numTarimas];

	
	id = (char*)malloc(sizeof(char)*30);
	msg = (char*)malloc(sizeof(char)*256);

	// Se calcula lo que le sucede al atleta y se guarda en el fichero log la hora a la que realizó el levantamiento.
	do
	{
	
		if (pthread_mutex_lock(&semaforo_tarimas)!=0)
		{
			perror("Error en el bloqueo del semáforo de las tarimas.\n");
			exit(-1);
		}
			for(i=0; i<numTarimas; i++)
			{
				atl_tar[i]=10000;
			}
			atleta_cogido=10000;

			//dentro de su propia cola
			int j;
		
			for(j=1; j<=numTarimas; j++)
			{
				for(i=0; i<maxAtletas; i++)
				{
					if(atletas[i].id!=0 && atletas[i].ha_competido== 0)
					{
						if(atletas[i].tarima_asignada==j && atl_tar[j-1]>atletas[i].id)
						{
							atl_tar[j-1]=i;
						}
					}
				}
			}
		
			if(atl_tar[numero-1]!=10000)
			{
				atleta_cogido=atl_tar[numero-1];
			}
			else
			{
				for(i=0; i<numTarimas; i++)
				{
					if(atl_tar[i]!=10000)
					{
						atleta_cogido = atl_tar[i];
						i=numTarimas;
					
					}
				}
			}
	
		if (pthread_mutex_unlock(&semaforo_tarimas)!=0)
		{
			perror("Error en el desbloqueo del semáforo de las tarimas.\n");
			exit(-1);
		}
		
		if(atleta_cogido!=10000)
		{
			atletas[atleta_cogido].ha_competido=1;
			comportamiento = calculaAleatorios(1,10); // Número aleatorio para calcular el comportamiento.
		
			while(atletas[atleta_cogido].calentamiento==0)
			{
				sleep(1);
			}

			if(comportamiento <=8) // Movimiento válido.
			{
				tiempo = calculaAleatorios(2,6);
				sleep(tiempo);
			
				puntuacion = calculaAleatorios(60,300);
			
				atletas[atleta_cogido].puntuacion = puntuacion;

				// Se escribe en el log.
				sprintf(id, "Juez %d", numero);  
				sprintf(msg, "El dorsal %d hizo un levantamiento asombroso: %d puntos.", atletas[atleta_cogido].id, puntuacion);
				printf("%s: %s\n", id, msg);
		
				if (pthread_mutex_lock(&semaforo_escribir)!=0)	
				{
					perror("Error en el bloqueo del semáforo para escribir en el fichero.\n");
					exit(-1);
				}
				
					writeLogMessage(id, msg); 

				if (pthread_mutex_unlock(&semaforo_escribir)!=0) 
				{
					perror("Error en el desbloqueo del semáforo para escribir en el fichero.\n");
					exit(-1);
				}
		
			} 
			else if(comportamiento == 9) // Movimiento nulo por indumentaria.
				{
					tiempo = calculaAleatorios(1,4);
					sleep(tiempo);

					sprintf(id, "Juez %d", numero);  
					sprintf(msg, "El dorsal %d no lleva pantalones: ¡un CERO!.", atletas[atleta_cogido].id);
					atletas[atleta_cogido].puntuacion = 0;
					printf("%s: %s\n", id, msg);
				
					if (pthread_mutex_lock(&semaforo_escribir)!=0)
					{
						perror("Error en el bloqueo del semáforo para escribir en el fichero.\n");
						exit(-1);
					}
				
						writeLogMessage(id, msg); 

					if (pthread_mutex_unlock(&semaforo_escribir)!=0)
					{
						perror("Error en el desbloqueo del semáforo para escribir en el fichero.\n");
						exit(-1);
					}

					puntuacion = 0;
				} 
				else // Movimiento nulo por falta de fuerza.
				{
					tiempo = calculaAleatorios(6,10);
					sleep(tiempo);

					sprintf(id, "Juez %d", numero);  
					sprintf(msg, "El dorsal %d es un enclenque: ¡un CERO!.", atletas[atleta_cogido].id);
					atletas[atleta_cogido].puntuacion = 0;
					printf("%s: %s\n", id, msg);
				
					if (pthread_mutex_lock(&semaforo_escribir)!=0)
					{
						perror("Error en el bloqueo del semáforo para escribir en el fichero.\n");
						exit(-1);
					}
				
						writeLogMessage(id, msg); 

					if (pthread_mutex_unlock(&semaforo_escribir)!=0)
					{
						perror("Error en el desbloqueo del semáforo para escribir en el fichero.\n");
						exit(-1);
					}

					puntuacion = 0;
				}
	
			// Se guarda la puntuación en el podio.
			for (i=0; i<3; i++)
			{
				if (atletas[atleta_cogido].puntuacion>=podio[1][i])
				{
					if(i!=2){
						for(j=2; j>=i+1; j--){
							podio[0][j] = podio[0][j-1];
							podio[1][j] = podio[1][j-1];
						}
					}
					podio[0][i]=atletas[atleta_cogido].id;
					podio[1][i]=atletas[atleta_cogido].puntuacion;
					i=3;
					
				}	
			}
	

			// Se calcula si el atleta necesita beber o no.
			if(calculaAleatorios(1,10) == 1) 
			{		
				atletas[atleta_cogido].necesita_beber=1;
				sprintf(id, "Juez %d", numero);
				sprintf(msg, "Dorsal %d necesitas ir a beber a la fuente.", atletas[atleta_cogido].id);   
				printf("%s: %s\n", id, msg);
			
				if (pthread_mutex_lock(&semaforo_escribir)!=0)
				{
					perror("Error en el bloqueo del semáforo para escribir en el fichero.\n");
					exit(-1);
				}
		
					writeLogMessage(id, msg); 

				if (pthread_mutex_unlock(&semaforo_escribir)!=0)
				{
					perror("Error en el desbloqueo del semáforo para escribir en el fichero.\n");
					exit(-1);
				}
			}
	
			// Finaliza el atleta que está participando.
			atletas[atleta_cogido].ha_competido=2;



			// Se comprueba si al juez le toca descansar (cada 4 atletas 10 segundos).
			punteroTarimas[numero-1].descansa++;
			punteroTarimas[numero-1].contador++;

			if (punteroTarimas[numero-1].descansa == 4) 
			{	
				// Inicio descanso.
				sprintf(id, "Juez %d", numero);  
				sprintf(msg, "Esto es muy aburrido, me voy a descansar.");
				printf("%s: %s\n", id, msg);
					
				if (pthread_mutex_lock(&semaforo_escribir)!=0)
				{
					perror("Error en el bloqueo del semáforo para escribir en el fichero.\n");
					exit(-1);
				}
		
					writeLogMessage(id, msg); 

				if (pthread_mutex_unlock(&semaforo_escribir)!=0)
				{
					perror("Error en el desbloqueo del semáforo para escribir en el fichero.\n");
					exit(-1);
				}

				sleep(10);
			
				// Fin descanso.
				sprintf(id, "Juez %d", numero);  
				sprintf(msg, "Ya he acabado de descansar.");
				printf("%s: %s\n", id, msg);
					
				if (pthread_mutex_lock(&semaforo_escribir)!=0)
				{
					perror("Error en el bloqueo del semáforo para escribir en el fichero.\n");
					exit(-1);
				}
		
					writeLogMessage(id, msg); 

				if (pthread_mutex_unlock(&semaforo_escribir)!=0)
				{
					perror("Error en el desbloqueo del semáforo para escribir en el fichero.\n");
					exit(-1);
				}

				punteroTarimas[numero-1].descansa = 0;
			}
		// 12. Volvemos al paso 1 y buscamos el siguiente (siempre priorizando entre los atletas asignados a dicha tarima).
		//para la parte opcional, con el malloc reservo espacio para muchos mas, en plan 300, y no con eso quiero decir q vaya  a permitir entrarl a todos, sino que hay espacio en memoria*/
		}
	}while (finalizar==0);

	free(id);
	free(msg);
}

void finalizaCompeticion (int sig)
{
	int i;
	char *id;
	char *msg;

	id = (char*)malloc(sizeof(char)*30);
	msg = (char*)malloc(sizeof(char)*256);

	if (signal(SIGINT, finalizaCompeticion)==SIG_ERR) 
	{
		perror("Error en la llamada a la señal SIGINT.\n");
		exit(-1);
	}
	printf("Has pulsado finalizar competición.\n");
	finalizar=1; // Se para de recibir señales.

	// Se escribe en el log que ha finalizado el programa.
	sprintf(id, "FIN DEL PROGRAMA");  
	sprintf(msg, "Se acabó este suplicio.\n");
	printf("%s: %s\n", id, msg);
					
	if (pthread_mutex_lock(&semaforo_escribir)!=0)
	{
		perror("Error en el bloqueo del semáforo para escribir en el fichero.\n");
		exit(-1);
	}
		
		writeLogMessage(id, msg); 

	if (pthread_mutex_unlock(&semaforo_escribir)!=0)
	{
		perror("Error en el desbloqueo del semáforo para escribir en el fichero.\n");
		exit(-1);
	}


	// Se cancelan y finalizan los hilos de los atletas.
	for (i=0; i<maxAtletas; i++)
	{
		pthread_cancel(atletas[i].atleta);
	}
	
	// Se escribe en el log qué atleta se ha ido sin beber.
	if (estadoFuente!=0)
	{
		sprintf(id, "Atleta %d", colaFuente[0].id);  
		sprintf(msg, "Me voy sin beber así que dadme agua, pero que esté bien fresquita.");
		printf("%s: %s\n", id, msg);
					
		if (pthread_mutex_lock(&semaforo_escribir)!=0)
		{
			perror("Error en el bloqueo del semáforo para escribir en el fichero.\n");
			exit(-1);
		}
		
			writeLogMessage(id, msg); 

		if (pthread_mutex_unlock(&semaforo_escribir)!=0)
		{
			perror("Error en el desbloqueo del semáforo para escribir en el fichero.\n");
			exit(-1);
		}						
		pthread_cancel(colaFuente[0].atleta); // Se finaliza el hilo del atleta que espera en la fuente.
	}

	sleep(3);
	printf("Te mostraré los resultados.\n");

	// Se escribe en el log los atletas que han pasado por cada tarima.
	for(i=1; i<=numTarimas; i++)
	{
		sprintf(id, "Total atletas tarima %d", i);  
		sprintf(msg, "%d", punteroTarimas[i-1].contador);
		printf("%s: %s\n", id, msg);
		if (pthread_mutex_lock(&semaforo_escribir)!=0)
		{
			perror("Error en el bloqueo del semáforo para escribir en el fichero.\n");
			exit(-1);
		}
		
			writeLogMessage(id, msg); 

		if (pthread_mutex_unlock(&semaforo_escribir)!=0)
		{
			perror("Error en el desbloqueo del semáforo para escribir en el fichero.\n");
			exit(-1);
		}
	}

	// Se cancelan y finalizan los hilos de las tarimas.
	for (i=0; i<numTarimas; i++)
	{
		pthread_cancel(punteroTarimas[i].tatami);
	}
	
	// Podio.
	if (pthread_mutex_lock(&semaforo_escribir)!=0)
	{
		perror("Error en el bloqueo del semáforo para escribir en el fichero.\n");
		exit(-1);
	}
		sprintf(id, "PRIMERA POSICIÓN");  
		sprintf(msg, "Atleta %d con %d puntos.", podio[0][0], podio[1][0]);	
		printf("%s: %s\n", id, msg);
	
		writeLogMessage(id, msg); 

		sprintf(id, "SEGUNDA POSICIÓN");  
		sprintf(msg, "Atleta %d con %d puntos.", podio[0][1], podio[1][1]);	
		printf("%s: %s\n", id, msg);
		
		writeLogMessage(id, msg); 

		sprintf(id, "TERCERA POSICIÓN");  
		sprintf(msg, "Atleta %d con %d puntos.", podio[0][2], podio[1][2]);
		printf("%s: %s\n", id, msg);	
	
		writeLogMessage(id, msg); 

	if (pthread_mutex_unlock(&semaforo_escribir)!=0)
	{
		perror("Error en el desbloqueo del semáforo para escribir en el fichero.\n");
		exit(-1);
	}


	// Destrucción de los semáforos y la condición.
	if (pthread_mutex_destroy(&semaforo_atletas)!=0)
	{
		perror("Error en la destrucción del semáforo para los atletas.\n");
		exit(-1);
	}
	
	if (pthread_mutex_destroy(&semaforo_tarimas)!=0)
	{
		perror("Error en la destrucción del semáforo para las tarimas.\n");
		exit(-1);
	}
	
	if (pthread_mutex_destroy(&semaforo_fuente)!=0)
	{
		perror("Error en la destrucción del semáforo para la fuente.\n");
		exit(-1);
	}
	
	if (pthread_cond_destroy(&condicion)!=0)
	{
		perror("Error en la destrucción de la condición.\n");
		exit(-1);
	}
	
	if (pthread_mutex_destroy(&semaforo_escribir)!=0)
	{
		perror("Error en la destrucción del semáforo para escribir en el fichero.\n");
		exit(-1);
	}
	

	// Se libera toda la memoria reservada.
	free(atletas);
	free(punteroTarimas);
	free(id);
	free(msg);	
}


void  writeLogMessage(char *id, char *msg) 
{
	// Se calcula la hora actual.
	time_t  now = time (0);
	struct  tm *tlocal = localtime (&now);
	char  stnow [19];
	strftime(stnow , 19, " %d/ %m/ %y  %H: %M: %S", tlocal);

	// Se escribe en el fichero log llamado registroTiempos.log.
	registro = fopen(nombreArchivo, "a");
	fprintf(registro , "[ %s]  %s:  %s\n", stnow , id, msg);
	fclose(registro);
}


int calculaAleatorios(int min, int max) 
{
	return rand() % (max-min+1) + min;
}

