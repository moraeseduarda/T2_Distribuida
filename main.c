
#include <mpi.h>

int main() {
    
    // Inicializa MPI
    MPI_Init(NULL, NULL);

    // MPI_COMM_WORLD
    // É o comunidador padrão
    // Um comunicador define um grupo de processos cooperantes

    // Obtem o rank do processo requisitante associado ao comunicador. Cada processo sera associado a um unico
    //rank inteiro entre 0 e (número de processos – 1)
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD , &world_rank);
        
    // Determina numero de processos total no programa
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Processo coordenador, rank 0
    if (world_rank == 0) {
        // Distribuir os arquivos entre os workers
        
    }
    // Demais processos "workers"
    else {
        
        // Aguardar recebimento
        MPI_Recv();
    }

    MPI_Finalize();
    return 0;
}

int busca_arquivo() {}
