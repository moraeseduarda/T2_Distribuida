#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <dirent.h>

int main(int argc, char *argv[]) {
    if (argc != 7) {
        printf("O número de parâmetros deve ser 7\n");
        return 1;
    }
    
    // char *nome_arquivo[256];
    char *caminho_dir = argv[2];
    char *termo_busca = argv[4];

    MPI_Init(&argc, &argv); // Inicializa o MPI
    
    int rank_proc;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_proc); // pega rank do processo requisitante
    int total_processos;
    MPI_Comm_size(MPI_COMM_WORLD, &total_processos);
    int num_workers = total_processos - 1;

    if (rank_proc == 0) {
        DIR *diretorio = opendir(caminho_dir);
        if (diretorio == NULL) {
            printf("Erro ao abrir o diretório: %s\n", caminho_dir);
            return 1;
        }
        struct dirent *entrada;
        int contador_arquivos = 0;
        while (entrada = readdir(diretorio) != NULL) {
            if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) {
                continue;
            }

            char caminho_completo[256];
            snprintf(caminho_completo, sizeof(caminho_completo), "%s/%s", caminho_dir, entrada->d_name);
        
            int rank_worker = contador_arquivos % num_workers + 1;
            MPI_Send(caminho_completo, sizeof(caminho_completo), MPI_CHAR, rank_worker, 1, MPI_COMM_WORLD);
            contador_arquivos++;
        };
        
    } else {
        char nome_arquivo[256];
        MPI_Recv(nome_arquivo, sizeof(nome_arquivo), MPI_CHAR, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        FILE *pont_arq = fopen(nome_arquivo, "r");
        
        char linha_lida[1024];
        int num_linha = 0;
        int total_ocorrencias = 0;
        // Faz um laço para ler o arquivo, lendo uma linha por vez e verificando se tem o termo ou não.
        while (fgets(linha_lida, sizeof(linha_lida), pont_arq)) {
            num_linha++;
            if (strstr(linha_lida, termo_busca) != NULL) {
                total_ocorrencias++;
            }
        }
    }
    MPI_Finalize();
}