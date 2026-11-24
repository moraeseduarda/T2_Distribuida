# Guia de Compilação e Execução — Trabalho 2: Busca Distribuída em Arquivos de Log

**Grupo:** Eduarda Moraes dos Santos, Helena Callado, Isabela Serafim

---

## Estrutura do Projeto

**Antes da compilação:**
* `busca_logs`
* `dinamico.c`
* `estatico-v2.c`
* `gerador_logs_t2.c`
* `INSTRUCOES.md`
* `main.c`

**Após a compilação e execução dos testes:**
Além dos arquivos originais, são gerados:
* **Executáveis compilados**
* **Diretórios:** `testes/` (contendo os arquivos gerados nos testes contidos no relatório) e ` logs/` (contendo arquivos de teste de compilação)


## 1. Preparação do Ambiente com Docker (Linux Ubuntu)

Para garantir reprodutibilidade e isolamento de dependências, utilizamos o ambiente containerizado baseado na imagem e no tutorial de uso do Docker fornecidos no Moodle, em um Linux Ubuntu.

### Passos de Configuração

1. Caso não tenha o Docker instalado, realizar o tutorial **Rodando MPI no Docker**, disponível no Moodle da disciplina.

2. Iniciar o contêiner:

   ```bash
   sudo docker run --rm -it --user "$(id -u):$(id -g)" --mount "type=bind,source=$PWD,target=/exercicios" mpi-lab
   ```

3. Acessar o diretório da aplicação no contêiner:

   ```bash
   cd /exercicios/T2_Distribuida
   ```

4. *(Opcional)* Verificar a quantidade de núcleos lógicos disponíveis:

   ```bash
   nproc
   ```

---

## 2. Compilação dos Programas

Todos os comandos de compilação devem ser executados a partir do diretório `/exercicios/T2_Distribuida`, no terminal do contêiner.

### 2.1. Gerador de Logs (C Sequencial)

Compilação do gerador sintético em C padrão, utilizando o compilador `gcc`:

```bash
gcc -Wall -O2 -o gerador_logs_t2 gerador_logs_t2.c
```

- `-Wall`: ativa todos os avisos de compilação padrão, para garantir um código limpo.
- `-O2`: aplica otimizações de nível 2, para alta performance na geração de dados.
- `-o gerador_logs_t2`: define o nome do binário executável gerado.

### 2.2. Aplicação Distribuída MPI

Compilação dos arquivos do trabalho:

```bash
mpicc -Wall -O2 -o busca_logs main.c dinamico.c estatico-v2.c
```


---

## 3. Geração da Massa de Dados de Teste

O gerador cria arquivos de logs sintéticos com parâmetros configuráveis para testes de escala.

### Sintaxe do Comando

```bash
./gerador_logs_t2 <qtd_arquivos> <linhas_por_arquivo> <palavras_por_linha> <seed> <diretorio_saida>
```

### Exemplo Prático de Teste Inicial

```bash
./gerador_logs_t2 10 500 8 12345 logs
```

### Explicação dos Parâmetros

| Valor | Descrição |
|-------|-----------|
| `10` | Quantidade total de arquivos `.log` a serem criados |
| `500` | Número de linhas por arquivo |
| `8` | Quantidade de palavras geradas por linha em cada mensagem |
| `12345` | Semente (*seed*) do gerador pseudoaleatório (garante a reprodutibilidade exata da massa de dados) |
| `logs` | Diretório de destino onde os arquivos gerados serão salvos |

---

## 4. Execução da Busca Distribuída

A execução distribuída é coordenada via `mpiexec`. O processo de rank 0 atua como **Master/Coordenador**, enquanto os ranks 1 a N-1 atuam como **Workers**.

### Sintaxe Geral

```bash
mpiexec -np <num_processos> ./busca_logs --dir <caminho_logs> --search "<termo>" --mode <static|dynamic>
```

### Parâmetros de Execução

| Parâmetro | Descrição |
|-----------|-----------|
| `-np <num_processos>` | Quantidade total de processos MPI a serem instanciados (mínimo de 2: 1 Master + 1 Worker) |
| `--dir <caminho>` | Caminho absoluto ou relativo para a pasta contendo os arquivos `.log` |
| `--search "<termo>"` | Termo exato (*case-sensitive*) a ser localizado nas mensagens de log |
| `--mode <estratégia>` | Estratégia de distribuição: `dynamic` (sob demanda) ou `static` (divisão prévia) |

### Exemplos de Execução

**Execução no modo dinâmico:**

```bash
mpiexec -np 4 ./busca_logs --dir /exercicios/T2_Distribuida/logs --search "ERROR" --mode dynamic
```

**Execução no modo estático:**

```bash
mpiexec -np 4 ./busca_logs --dir /exercicios/T2_Distribuida/logs --search "ERROR" --mode static
```