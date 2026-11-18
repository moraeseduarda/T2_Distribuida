#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
    if (argc != 7) {
        printf("O número de parâmetros deve ser 7\n");
        return 1;
    }
    int total_processos;
    int num_workers = total_processos - 1;
    char nome_arquivo = argv[2];

    MPI_Init(&argc, &argv); // Inicializa o MPI
    
    int rank_proc;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_proc); // pega rank do processo requisitante
    // MPI_Comm_size(MPI_COMM_WORLD, &total_processos);

    if (rank_proc == 0) {
        FILE *pont_arq = fopen(nome_arquivo, "r");
        // Faz um laço para ler o arquivo, lendo uma linha por vez e verificando se tem o termo ou não.
        for (int i = 0; i < num_workers; i++) {
            rank_vez = i % num_workers + 1;
            // MPI_Send(buffer, quantidade, MPI_CHAR, rank_vez, tag, MPI_COMM_WORLD);
        }
        
    } else {
        // Worker orquestrado
        //MPI_Recv(&buffer, quantidade, MPI_CHAR, 0, tag, MPI_COMM_WORLD, status?);
    }
}