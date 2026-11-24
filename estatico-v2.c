#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <dirent.h>

// Tags MPI usadas ao longo do código
#define TAG_ENCERRAR 0
#define TAG_TAREFA 1
#define TAG_TEXTO 2
#define TAG_RESULTADO 3

// Retorna apenas o nome do arquivo a partir do caminho completo
static char *pegar_nome_arquivo(char *caminho) {
    char *barra = strrchr(caminho, '/');
    return barra ? barra + 1 : caminho;
}

// Função de comparação usada pelo qsort para garantir ordenação
static int comparar_nomes_estatico(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

int main_estatico(int argc, char *argv[]) {
    MPI_Init(&argc, &argv); // Inicializa o MPI
    
    int rank_proc, total_processos;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_proc); // pega rank do processo requisitante
    MPI_Comm_size(MPI_COMM_WORLD, &total_processos); // pega total de processos
    int num_workers = total_processos - 1;

    // Leitura flexível de argumentos de linha de comando
    char *caminho_dir = NULL;
    char *termo_busca = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dir") == 0) caminho_dir = argv[i + 1];
        if (strcmp(argv[i], "--search") == 0) termo_busca = argv[i + 1];
    }

    if (caminho_dir == NULL || termo_busca == NULL || num_workers < 1) {
        if (rank_proc == 0) {
            printf("Parâmetros inválidos. Use mpirun -np N ./busca_logs --dir <diretório> --search <termo> --mode static\n");
        }
        MPI_Finalize();
        return 1;
    }

    if (rank_proc == 0) {
        DIR *diretorio = opendir(caminho_dir);
        if (diretorio == NULL) {
            printf("Erro ao abrir o diretório: %s\n", caminho_dir);
            for (int i = 1; i <= num_workers; i++) {
                MPI_Send(NULL, 0, MPI_CHAR, i, TAG_ENCERRAR, MPI_COMM_WORLD);
            }
            MPI_Finalize();
            return 1;
        }

        int capacidade_arqs = 64;
        int contador_arquivos = 0;
        char **caminhos = malloc(capacidade_arqs * sizeof(char *));
        struct dirent *entrada;
        
        while ((entrada = readdir(diretorio)) != NULL) {
            if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) {
                continue;
            }
            if (contador_arquivos == capacidade_arqs) {
                capacidade_arqs *= 2;
                caminhos = realloc(caminhos, capacidade_arqs * sizeof(char *));
            }

            char caminho_completo[512];
            snprintf(caminho_completo, sizeof(caminho_completo), "%s/%s", caminho_dir, entrada->d_name);
            caminhos[contador_arquivos] = strdup(caminho_completo);

            contador_arquivos++;
        }
        closedir(diretorio);

        qsort(caminhos, contador_arquivos, sizeof(char *), comparar_nomes_estatico);
        
        int total_geral = 0;
        int *ocorrencias_por_arquivo = calloc(contador_arquivos, sizeof(int));
        char **textos_resultado = calloc(contador_arquivos, sizeof(char *));

        double tempo_inicio = MPI_Wtime();

        // Distribuição estática usando round-robin
        for (int arq = 0; arq < contador_arquivos; arq++) {
            int rank_worker = arq % num_workers + 1;
            MPI_Send(caminhos[arq], strlen(caminhos[arq]) + 1, MPI_CHAR, rank_worker, TAG_TAREFA, MPI_COMM_WORLD);
        }

        for (int i = 1; i <= num_workers; i++) {
            MPI_Send(NULL, 0, MPI_CHAR, i, TAG_ENCERRAR, MPI_COMM_WORLD);
        }

        // Recebimento dos resultados e guarda em memória
        for (int arq = 0; arq < contador_arquivos; arq++) {
            int rank_worker = arq % num_workers + 1;
            int cab[2]; //cab[0] = ocorrencias, cab[1] = tamanho do texto

            MPI_Recv(cab, 2, MPI_INT, rank_worker, TAG_RESULTADO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            ocorrencias_por_arquivo[arq] = cab[0];
            total_geral += cab[0];

            textos_resultado[arq] = malloc(cab[1] + 1);
            MPI_Recv(textos_resultado[arq], cab[1], MPI_CHAR, rank_worker, TAG_TEXTO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            textos_resultado[arq][cab[1]] = '\0'; // Adiciona o terminador de string
        }
        double tempo_fim = MPI_Wtime();

        printf("=== RESULTADOS DA BUSCA ===\n");
        for (int arq = 0; arq < contador_arquivos; arq++) {
            printf("%s", textos_resultado[arq]);
        }

        printf("\n=== RESUMO ===\n");
        printf("Total de arquivos: %d\n", contador_arquivos);
        printf("Total de ocorrencias: %d\n", total_geral);
        for (int i = 0; i < contador_arquivos; i++) {
            printf("    %s: %d ocorrencias\n", pegar_nome_arquivo(caminhos[i]), ocorrencias_por_arquivo[i]);
        }
        printf("\nTempo de execucao: %.4f segundos\n", tempo_fim - tempo_inicio);

        for (int i = 0; i < contador_arquivos; i++) {
            free(caminhos[i]);
            free(textos_resultado[i]);
        }
        free(caminhos);
        free(ocorrencias_por_arquivo);
        free(textos_resultado);

    } else {
        while (1) {
            MPI_Status status;
            char nome_arquivo[512];
            MPI_Recv(nome_arquivo, sizeof(nome_arquivo), MPI_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (status.MPI_TAG == TAG_ENCERRAR) {
                break;
            }
            // Buffer dinâmico
            size_t cap_saida = 4096;
            size_t tam_saida = 0;
            char *saida = malloc(cap_saida);
            // Zera a primeira posição do array para esvaziar o texto do arquivo anterior
            saida[0] = '\0';

            int total_ocorrencias = 0;
            FILE *pont_arq = fopen(nome_arquivo, "r");
            
            if (pont_arq != NULL) {
                char linha_lida[4096];
                char *nome_curto = pegar_nome_arquivo(nome_arquivo);
                int num_linha = 0;
                
                // Faz um laço para ler o arquivo, lendo uma linha por vez e verificando se tem o termo ou não.
                while (fgets(linha_lida, sizeof(linha_lida), pont_arq)) {
                    num_linha++;
                    if (strstr(linha_lida, termo_busca) != NULL) {
                        total_ocorrencias++;
                        size_t necessario = strlen(nome_curto) + strlen(linha_lida) + 32;
                        while (tam_saida + necessario >= cap_saida) {
                            cap_saida *= 2;
                            saida = realloc(saida, cap_saida);
                        }

                        tam_saida += sprintf(saida + tam_saida, "%s:%d: %s", nome_curto, num_linha, linha_lida);
                    }
                }
                fclose(pont_arq);
            }

            int cab[2] = {total_ocorrencias, (int)tam_saida};
            MPI_Send(cab, 2, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
            MPI_Send(saida, (int)tam_saida, MPI_CHAR, 0, TAG_TEXTO, MPI_COMM_WORLD);
            free(saida);
        }
    }
    // Finaliza o MPI
    MPI_Finalize();
    return 0;
}