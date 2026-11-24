//// Inclusoes de bibliotecas relevantes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <mpi.h>

// Constantes que vao ser usadas ao longo dos blocos
#define TAG_ENCERRAR 0
#define TAG_TAREFA 1
#define TAG_RESULTADO 2
#define TAG_TEXTO 3

#define TAM_CAMINHO 512
#define TAM_LINHA 4096

// Formatacao do resultado de um arquivo (total de linhas e texto delas)
typedef struct
{
    int total;
    char *texto;
} Resultado;

//// FUNCOES REUTILIZAVEIS

// Diz qual de dois nomes de arquivos vem antes
// Precisa disso aqui pro resultado sair deterministgico
int comparar_nomes(const void *a, const void *b)
{
    return strcmp(*(char **)a, *(char **)b);
}

// Olha dentro da pasta e devolve os caminhos dos arquivos

char **listar_arquivos(const char *pasta, int *total)
{
    DIR *dir = opendir(pasta);
    if (dir == NULL)
    {
        *total = -1;
        return NULL;
    }

    int capacidade = 16;
    int n = 0;
    char **lista = malloc(capacidade * sizeof(char *));

    struct dirent *entrada;
    while ((entrada = readdir(dir)) != NULL)
    {
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0)
        {
            continue;
        }
        if (n == capacidade)
        {
            capacidade = capacidade * 2;
            lista = realloc(lista, capacidade * sizeof(char *));
        }
        char caminho[TAM_CAMINHO];
        snprintf(caminho, sizeof(caminho), "%s/%s", pasta, entrada->d_name);
        lista[n] = strdup(caminho);
        n++;
    }
    closedir(dir);

    qsort(lista, n, sizeof(char *), comparar_nomes);
    *total = n;
    return lista;
}

// "Seleciona" o arquivo dentro da pasta

char *nome_do_arquivo(char *caminho)
{
    char *barra = strrchr(caminho, '/');
    return barra ? barra + 1 : caminho;
}

// Abre um arquivo e guarda as linhas que contém o termo

int buscar_no_arquivo(char *caminho, char *termo, char **texto, int *tam_texto)
{
    *texto = NULL;
    *tam_texto = 0;

    FILE *arq = fopen(caminho, "r");
    if (arq == NULL)
    {
        return 0;
    }

    // Aqui usando a funcao nome_do_arquivo
    char *nome = nome_do_arquivo(caminho);
    char linha[TAM_LINHA];
    int num_linha = 0;
    int total = 0;

    while (fgets(linha, sizeof(linha), arq) != NULL)
    {
        num_linha++;
        linha[strcspn(linha, "\n")] = '\0';

        if (strstr(linha, termo) != NULL)
        {
            total++;
            // Realloc permite tamanhos variaveis de texto
            *texto = realloc(*texto, *tam_texto + strlen(nome) + strlen(linha) + 32);
            *tam_texto += sprintf(*texto + *tam_texto, "%s:%d: %s\n", nome, num_linha, linha);
        }
    }

    fclose(arq);
    return total;
}

// Funcao do worker
void executar_worker(char *termo)
{
    char caminho[TAM_CAMINHO];
    MPI_Status status;

    while (1)
    {
        // Recebe as diretrizes
        MPI_Recv(caminho, TAM_CAMINHO, MPI_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        // Se for diretriz de encerramento, interrompe
        if (status.MPI_TAG == TAG_ENCERRAR)
        {
            break;
        }

        char *texto;
        int tam_texto;
        // Usando a funcao de busca definida ali em cima
        int total = buscar_no_arquivo(caminho, termo, &texto, &tam_texto);

        int cab[2] = {total, tam_texto};
        // Envia resultado (qtd)
        MPI_Send(cab, 2, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
        // Envia texto do resultado
        MPI_Send(texto, tam_texto, MPI_CHAR, 0, TAG_TEXTO, MPI_COMM_WORLD);

        free(texto);
    }
}

// BLOCO DO MESTRE

void executar_mestre_dinamico(char *pasta, int workers)
{
    int T;
    // Usa a funcao de listagem de arquivos
    char **lista = listar_arquivos(pasta, &T);

    if (T < 0)
    {
        // Se der erro ao abrir, interrompe
        printf("Erro ao abrir o diretorio: %s\n", pasta);
    }
    // Se nao tiver nada a procurar, encerra
    if (T <= 0)
    {
        for (int w = 1; w <= workers; w++)
        {
            MPI_Send(NULL, 0, MPI_CHAR, w, TAG_ENCERRAR, MPI_COMM_WORLD);
        }
        if (T == 0)
        {
            printf("Nenhum arquivo encontrado em %s\n", pasta);
        }
        return;
    }

    // Caminho normal
    // Vetor de resultados
    Resultado *quadro = calloc(T, sizeof(Resultado));

    // Guarda qual arquivo cada worker está processando
    int *arquivo_do_worker = malloc((workers + 1) * sizeof(int));

    // Proximo arquivo ainda nao entregue
    int proximo = 0;

    // Qtd de worker ativos
    int workers_ativos = 0;

    // Para marcar quando começou
    double hora_inicio = MPI_Wtime();

    // Envio de tarefa
    for (int w = 1; w <= workers; w++)
    { // Se o proximo ainda não tiver atingido o total de arquivos
        if (proximo < T)
        {
            // Envia o proximo
            MPI_Send(lista[proximo], strlen(lista[proximo]) + 1, MPI_CHAR, w, TAG_TAREFA, MPI_COMM_WORLD);
            arquivo_do_worker[w] = proximo;
            proximo++;        // Incrementa
            workers_ativos++; // Incrementa
        }
        else
        { // Quando acaba, envia a tag de encerramento
            MPI_Send(NULL, 0, MPI_CHAR, w, TAG_ENCERRAR, MPI_COMM_WORLD);
        }
    }

    // Recebimento de resultado
    while (workers_ativos > 0)
    {
        int cab[2];
        MPI_Status status;
        MPI_Recv(cab, 2, MPI_INT, MPI_ANY_SOURCE, TAG_RESULTADO, MPI_COMM_WORLD, &status);
        int origem = status.MPI_SOURCE;

        char *texto = malloc(cab[1] + 1);
        MPI_Recv(texto, cab[1], MPI_CHAR, origem, TAG_TEXTO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        texto[cab[1]] = '\0';

        int i = arquivo_do_worker[origem];
        quadro[i].total = cab[0];
        quadro[i].texto = texto;

        if (proximo < T)
        {
            MPI_Send(lista[proximo], strlen(lista[proximo]) + 1, MPI_CHAR, origem, TAG_TAREFA, MPI_COMM_WORLD);
            arquivo_do_worker[origem] = proximo;
            proximo++;
        }
        else
        {
            MPI_Send(NULL, 0, MPI_CHAR, origem, TAG_ENCERRAR, MPI_COMM_WORLD);
            workers_ativos--;
        }
    }

    double hora_fim = MPI_Wtime();

    printf("=== RESULTADOS DA BUSCA ===\n");
    int total_geral = 0;
    for (int i = 0; i < T; i++)
    {
        printf("%s", quadro[i].texto);
        total_geral += quadro[i].total;
    }

    // Saida e limpeza de dados
    printf("\n=== RESUMO ===\n");
    printf("Total de arquivos: %d\n", T);
    printf("Total de ocorrencias: %d\n", total_geral);
    for (int i = 0; i < T; i++)
    {
        printf("    %s: %d ocorrencias\n", nome_do_arquivo(lista[i]), quadro[i].total);
    }
    printf("\nTempo de execucao: %.4f segundos\n", hora_fim - hora_inicio);

    for (int i = 0; i < T; i++)
    {
        free(lista[i]);
        free(quadro[i].texto);
    }
    free(lista);
    free(quadro);
    free(arquivo_do_worker);
}

int main_dinamico(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int meu_rank, total_processos;
    MPI_Comm_rank(MPI_COMM_WORLD, &meu_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &total_processos);
    int workers = total_processos - 1;

    char *pasta = NULL;
    char *termo = NULL;
    char *modo = NULL;
    for (int i = 1; i < argc - 1; i++)
    {
        if (strcmp(argv[i], "--dir") == 0)
            pasta = argv[i + 1];
        if (strcmp(argv[i], "--search") == 0)
            termo = argv[i + 1];
        if (strcmp(argv[i], "--mode") == 0)
            modo = argv[i + 1];
    }

    int args_ok = pasta != NULL && termo != NULL && modo != NULL && (strcmp(modo, "static") == 0 || strcmp(modo, "dynamic") == 0) && workers >= 1;

    if (!args_ok)
    {
        if (meu_rank == 0)
        {
            printf("Uso: mpirun -np N ./busca_logs --dir PASTA --search TERMO --mode static|dynamic\n");
            printf("(N deve ser pelo menos 2)\n");
        }
        MPI_Finalize();
        return 1;
    }

    if (strcmp(modo, "dynamic") == 0)
    {
        if (meu_rank == 0)
        {
            executar_mestre_dinamico(pasta, workers);
        }
        else
        {
            executar_worker(termo);
        }
    }

    MPI_Finalize();
    return 0;
}
