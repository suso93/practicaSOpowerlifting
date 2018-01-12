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


int podio[2][3]; // Matriz del podio donde se incluye tanto la puntuación como el identificador de los tres mejores atletas.


// Variables para el número máximo de atletas y el número de tarimas.
int maxAtletas;
int numTarimas;


// Fuente.
int *cola; // Puntero para la cola de la fuente.
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



/* Función principal. */


int main (int argc, char *argv[]) {
	// Parte opcional --> Asignación estática de recursos.
	maxAtletas = MAXIMOATLETAS; // Se inicia con el máximo de atletas por defecto.
	numTarimas = NUMEROTARIMAS; // Se inicia con el número de tarimas por defecto.

	// Si se introducen argumentos por la terminal el primero será para el máximo de atletas y el segundo para el número de tarimas.
	if(argc==2) maxAtletas=atoi(argv[1]);
	if(argc==3)
	{
		 maxAtletas=atoi(argv[1]);
		 numTarimas=atoi(argv[2]);
	}
		

	// Se modifican los comportamientos de las señales para inscribir a los atletas y para finalizar la competición, además de comprobar si hay errorer.
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

	// Se crea el fichero log para escritura y se comprueba si hay errores.
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
		srand (time(NULL)); // Semilla para generar números aleatorios, VER si hacemos así o con otra semilla.

		while(finalizar==0) // Bucle para recibir señales, duerme cada segundo para no sobrecargar al procesador.
		{
			sleep(2);
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
} // Devuelve -1 en caso de no haber sitio y sino nos da el primer hueco que encuentre.


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
			if (sig== SIGUSR1)
			{
				atletas[posicion].tarima_asignada=1;
			}
			else if (sig== SIGUSR2)
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


	// Se calcula la posición del atleta.
	for(i=0; i<maxAtletas; i++)
	{
		if(atletas[i].id==dorsal)
		{
			pos = i;
			break;
		}
	}


	// Se guardan los datos en log.	
	sprintf(elemento, "Atleta %d", dorsal); 
	sprintf(msg, "He entrado a la tarima %d, ¡os vais a enterar!\n", atletas[pos].tarima_asignada);
	printf("%s: %s", elemento, msg); // Se imprime el mensaje por pantalla.

	if (pthread_mutex_lock(&semaforo_escribir)!=0)	// Se bloquea el semáforo para que los mensajes entren de uno en uno.
	{
		perror("Error en el bloqueo del semáforo para escribir en el fichero.\n");
		exit(-1);
	}

		writeLogMessage(elemento, msg); // La hora de entrada a la tarima la escribe la función del log.

	if (pthread_mutex_unlock(&semaforo_escribir)!=0) // Se desbloquea el semáforo para que los mensajes entren de uno en uno.
	{
		perror("Error en el bloqueo del semáforo para escribir en el fichero.\n");
		exit(-1);
	}
	

	// Cálculo del comportamiento del atleta mientras está en la cola.

	do 
	{
		// Comprobación del estado de salud.
		estado_salud=calculaAleatorios(1,100); // Número aleatorio para calcular el estado de salud.

		if (estado_salud<=15)
		{
			printf("El atleta %d no puede realizar el levantamiento por problemas de deshidratación.\n", dorsal);

			sprintf(elemento, "Atleta %d", dorsal); 
			sprintf(msg, "Estoy deshidratado de tanto estrés y no puedo realizar el levantamiento.\n");
			printf("%s: %s", elemento, msg);
			if (pthread_mutex_lock(&semaforo_escribir)!=0)	
			{
				perror("Error en el bloqueo del semáforo para escribir en el fichero.\n");
				exit(-1);
			}

				writeLogMessage(elemento, msg);

			if (pthread_mutex_unlock(&semaforo_escribir)!=0) 
			{
				perror("Error en el bloqueo del semáforo para escribir en el fichero.\n");
				exit(-1);
			}
		pthread_exit(NULL);
		// Salir de la cola ???. 3.a. Si no llega a realizar el levantamiento, no llega a subir a la tarima y se escribe en el log, se daría fin al hilo Atleta y se liberaría espacio en la cola.
		}	
		else
		{
			sleep (3);
		}
	}while (atletas[pos].ha_competido==0); // Fin del atleta en la cola.


	// El atleta llega a la tarima y espera 4 segundos para realizar su levantamiento.
	printf("El atleta %d se prepara para realizar el levantamiento.\n", dorsal);
	sleep(4);
	// 4. Si ya ha salido a competir, debemos esperar a que termine.
	
	// 5. Guardamos en el log la hora a la que ha finalizado su levantamiento.
	
	// 6. Fin del hilo del atleta. NOTA: el hilo se sigue ejecutando aun asi este el tio atascado en la fuente.
			
}


void *accionesTarima (void *arg)
{
	int numero = *(int*)arg; // Se convierte el argumento a tipo entero.
	int lugar;
	int comportamiento;
	int tiempo;
	int puntuacion;
	char *id;
	char *msg;
	int i;

	
	// Se calcula la posición (hemos llamado lugar) del atleta.
	for(i=0; i<maxAtletas; i++)
	{
		if(atletas[i].id==numero)
		{
			lugar = i;
			break;
		}
	}

	// Se calcula lo que le sucede al atleta y se guarda en el fichero log la hora a la que realizó el levantamiento.
	do
	{

		//Hay que sacar el atleta de la patata esa
	
	
		comportamiento = calculaAleatorios(1,10); // Número aleatorio para calcular el comportamiento.

		if(comportamiento <=8) // Movimiento válido.
		{
			tiempo = calculaAleatorios(2,6);
			sleep(tiempo);
			
			sprintf(id, "Juez %d", numero);  
			sprintf(msg, "El dorsal %d hizo un levantamiento asombroso.\n", atletas[lugar].id);
			printf("%s: %s", id, msg);

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
		

			puntuacion = calculaAleatorios(60,300);
		
			atletas[lugar].puntuacion = puntuacion;
			sprintf(msg, "Calificó el movimiento con la puntuación de: %d.\n", puntuacion);
			printf("%s: %s", id, msg);
		
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
		
		} else if(comportamiento == 9) // Movimiento nulo por indumentaria.
			{
				tiempo = calculaAleatorios(1,4);
				sleep(tiempo);

				sprintf(id, "Juez %d", numero);  
				sprintf(msg, "El dorsal %d no lleva pantalones: ¡un CERO!.\n", atletas[lugar].id);
				printf("%s: %s", id, msg);
				
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
				sprintf(msg, "El dorsal %d es un enclenque: ¡un CERO!.\n", atletas[lugar].id);
				printf("%s: %s", id, msg);
				
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
			if (atletas[lugar].puntuacion>=podio[1][i])
			{
				podio[0][i]=atletas[lugar].id;
				podio[1][i]=atletas[lugar].puntuacion;
			}	
		}
	

		// Se calcula si el atleta necesita beber o no.
		if(calculaAleatorios(1,10) == 1) 
		{		
			atletas[lugar].necesita_beber=1;
			sprintf(id, "Juez %d", numero);
			sprintf(msg, "Dorsal %d necesitas ir a beber.\n", atletas[lugar].id);   
			printf("%s: %s", id, msg);
			
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
		atletas[lugar].ha_competido=1;

		// Se comprueba si al juez le toca descansar (cada 4 atletas 10 segundos).
		punteroTarimas[numero-1].descansa++;

		if (punteroTarimas[numero-1].descansa == 4) 
		{	
			// Inicio descanso.
			sprintf(id, "Juez %d", numero);  
			sprintf(msg, "Esto es muy aburrido, me voy a descansar.\n");
			printf("%s: %s", id, msg);
					
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
			sprintf(msg, "Ya he acabado de descansar.\n");
			printf("%s: %s", id, msg);
					
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
		//para la parte opcional, con el malloc reservo espacio para muchos mas, en plan 300, y no con eso quiero decir q vaya  a permitir entrarl a todos, sino que hay espacio en memoria
	}while (finalizar==0);
}

void finalizaCompeticion (int sig)
{
	int i;
	char *id;
	char *msg;

	if (signal(SIGINT, finalizaCompeticion)==SIG_ERR) 
	{
		perror("Error en la llamada a la señal SIGINT.\n");
		exit(-1);
	}
	printf("Has pulsado finalizar competición.\n");
	finalizar=1; // Se para de recibir señales.

	sprintf(id, "FIN DEL PROGRAMA");  
	sprintf(msg, "Se acabó este suplicio.\n");
	printf("%s: %s", id, msg);
					
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


	// Se cancelan y finalizan los hilos, incluyendo al que está en la fuente.
	for (i=0; i<maxAtletas; i++)
	{
		pthread_cancel(atletas[i].atleta);
		pthread_exit(NULL); //????????
	}

	sleep(3);
	printf("Te mostraré los resultados.\n");

	// Atletas tarima 1.
	sprintf(id, "Total atletas tarima 1");  
	sprintf(msg, "%d", punteroTarimas[0].contador);
	printf("%s: %s", id, msg);
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
	
	
	// Atletas tarima 2.
	sprintf(id, "Total atletas tarima 2");  
	sprintf(msg, "%d", punteroTarimas[1].contador);
	printf("%s: %s", id, msg);
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
	
	
	// Podio.
	sprintf(id, "PRIMERA POSICIÓN");  
	sprintf(msg, "Atleta %d con %d puntos.\n", podio[0][0], podio[1][0]);	
	printf("%s: %s", id, msg);
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
	sprintf(id, "SEGUNDA POSICIÓN");  
	sprintf(msg, "Atleta %d con %d puntos.\n", podio[0][1], podio[1][1]);	
	printf("%s: %s", id, msg);
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
	sprintf(id, "TERCERA POSICIÓN");  
	sprintf(msg, "Atleta %d con %d puntos.\n", podio[0][2], podio[1][2]);
	printf("%s: %s", id, msg);	
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


	// Destrucción de los semáforos y la condición.
	if (pthread_mutex_destroy(&semaforo_atletas)!=0)
	{
		perror("Error en la destrucción del semáforo para los atletas.\n");
		exit(-1);
	}
	if (pthread_mutex_destroy(&semaforo_fuente)!=0)
	{
		perror("Error en la destrucción del semáforo para la fuente.\n");
		exit(-1);
	}
	if (pthread_mutex_destroy(&semaforo_escribir)!=0)
	{
		perror("Error en la destrucción del semáforo para escribir en el fichero.\n");
		exit(-1);
	}
	if (pthread_cond_destroy(&condicion)!=0)
	{
		perror("Error en la destrucción de la condición.\n");
		exit(-1);
	}


	// Se libera la memoria reservada.
	free(atletas);
	free(punteroTarimas);
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

