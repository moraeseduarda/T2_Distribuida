#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <dirent.h>

char *pegar_nome_arquivo(char *caminho) {
    char *barra = strrchr(caminho, '/');

    char *ultima_barra = barra ? barra + 1 : caminho;
    return ultima_barra;
}

int main(int argc, char *argv[]) {
    if (argc != 7) {
        printf("O número de parâmetros deve ser 7\n");
        return 1;
    }
    
    // char *nome_arquivo[256];
    char *caminho_dir = argv[2];
    char *termo_busca = argv[4];

    MPI_Init(&argc, &argv); // Inicializa o MPI
    double tempo_inicio = MPI_Wtime();

    // Variáveis para armazenar nomes de arquivos e ocorrências para o resumo final
    char nomes_arquivos[1000][256];
    int ocorrencias_por_arquivo[1000];
    int contador_arquivos = 0;
    int total_geral = 0;
    
    int rank_proc;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_proc); // pega rank do processo requisitante
    int total_processos;
    MPI_Comm_size(MPI_COMM_WORLD, &total_processos); // pega total de processos
    int num_workers = total_processos - 1;

    if (rank_proc == 0) {
        DIR *diretorio = opendir(caminho_dir);
        if (diretorio == NULL) {
            printf("Erro ao abrir o diretório: %s\n", caminho_dir);
            return 1;
        }
        struct dirent *entrada;
        
        while ((entrada = readdir(diretorio)) != NULL) {
            if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) {
                continue;
            }
            strcpy(nomes_arquivos[contador_arquivos], entrada->d_name);

            char caminho_completo[512];
            snprintf(caminho_completo, sizeof(caminho_completo), "%s/%s", caminho_dir, entrada->d_name);
        
            int rank_worker = contador_arquivos % num_workers + 1;
            MPI_Send(caminho_completo, strlen(caminho_completo) + 1, MPI_CHAR, rank_worker, 1, MPI_COMM_WORLD);
            contador_arquivos++;
        };

        closedir(diretorio);
        
        for (int i = 1; i <= num_workers; i++) {
            MPI_Send(NULL, 0, MPI_CHAR, i, 0, MPI_COMM_WORLD);
        }

        printf("=== RESULTADOS DA BUSCA ===\n");
        for (int arq = 0; arq < contador_arquivos; arq++) {
            MPI_Status status;
            int ocorrencias_arq = 0;
            char saida_arqs[10000];
            int rank_worker = arq % num_workers + 1;

            MPI_Recv(&ocorrencias_arq, 1, MPI_INT, rank_worker, 3, MPI_COMM_WORLD, &status);
            ocorrencias_por_arquivo[arq] = ocorrencias_arq;
            total_geral += ocorrencias_arq;

            MPI_Recv(saida_arqs, sizeof(saida_arqs), MPI_CHAR, rank_worker, 2, MPI_COMM_WORLD, &status);
            printf("%s", saida_arqs);
        }
       
    } else {
        while (1) {
            MPI_Status status;
        
            char nome_arquivo[512];
            MPI_Recv(nome_arquivo, sizeof(nome_arquivo), MPI_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (status.MPI_TAG == 0) {
                break;
            }
            char saida[10000];
            saida[0] = '\0';

            FILE *pont_arq = fopen(nome_arquivo, "r");
            if (pont_arq == NULL) {
                printf("Erro ao abrir o arquivo: %s\n", nome_arquivo);
                int total_ocorrencias = 0;
                MPI_Send(&total_ocorrencias, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
                MPI_Send(saida, strlen(saida) + 1, MPI_CHAR, 0, 2, MPI_COMM_WORLD);
                
                continue;
            }
            
            
            char linha_lida[1024];
            int num_linha = 0;
            int total_ocorrencias = 0;
            
            // Faz um laço para ler o arquivo, lendo uma linha por vez e verificando se tem o termo ou não.
            while (fgets(linha_lida, sizeof(linha_lida), pont_arq)) {
                num_linha++;
                if (strstr(linha_lida, termo_busca) != NULL) {
                    total_ocorrencias++;
                    char linha_format[2048];
                    sprintf(linha_format, "%s:%d: %s", pegar_nome_arquivo(nome_arquivo), num_linha, linha_lida); // Talvez voltar \n para formatação
                    strcat(saida, linha_format);
                }
            }

            fclose(pont_arq);
            MPI_Send(&total_ocorrencias, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
            MPI_Send(saida, strlen(saida) + 1, MPI_CHAR, 0, 2, MPI_COMM_WORLD);
        }
    }
    // Sincroniza todos os processos antes de finalizar
    MPI_Barrier(MPI_COMM_WORLD);
    double tempo_fim = MPI_Wtime();

    if (rank_proc == 0) {
        printf("=== RESUMO ===\n");
        printf("Total de arquivos: %d\n", contador_arquivos);
        printf("Total de ocorrencias: %d\n", total_geral);
        for (int i = 0; i < contador_arquivos; i++) {
            printf("%s: %d ocorrencias\n", nomes_arquivos[i], ocorrencias_por_arquivo[i]);
        }
        printf("\nTempo de execucao: %f segundos\n", tempo_fim - tempo_inicio);
    }

    MPI_Finalize();
}