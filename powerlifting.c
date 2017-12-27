#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int cuantos_atletas=0;

struct atletas_competicion {
	int id;
	int ha_competido;
	int tarima_asignada;
	int necesita_beber;
};
struct atletas_competicion atletas[10];//en el campeonato habrá máximo 10 atletas compitiendo simultáneamente

void inicializaCampeonato();
int haySitioEnCampeonato();//nos dirá si hay sitio (y si lo hay nos dice el primer hueco) para que entre un atleta a competir

int main (int argc, char *argv[]) {
	inicializaCampeonato();
	//visualizo la estructura inicial: 
	for (int i=0;i<10;i++) {
		printf("Atleta %d: ha competido %d, su tarima actual es %d y necesita beber %d\n",atletas[i].id,atletas[i].ha_competido,atletas[i].tarima_asignada,atletas[i].necesita_beber);
	}
	printf("Hueco para nuevo atleta: %d\n",haySitioEnCampeonato());
}

void inicializaCampeonato() {
	for (int i=0;i<10;i++) {
		atletas[i].id=i;
		atletas[i].ha_competido=0;
		atletas[i].tarima_asignada=0;
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
