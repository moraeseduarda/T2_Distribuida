#include <stdio.h>
#include <string.h>

int main_estatico(int argc, char *argv[]);
int main_dinamico(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    // Descobre o modo olhando os argumentos
    char *modo = NULL;
    for (int i = 1; i < argc - 1; i++)
    {
        if (strcmp(argv[i], "--mode") == 0)
            modo = argv[i + 1];
    }

    if (modo != NULL && strcmp(modo, "static") == 0)
    {
        return main_estatico(argc, argv);
    }
    else if (modo != NULL && strcmp(modo, "dynamic") == 0)
    {
        return main_dinamico(argc, argv);
    }
    else
    {
        printf("Uso: mpirun -np N ./busca_logs --dir PASTA --search TERMO --mode static|dynamic\n");
        return 1;
    }
}