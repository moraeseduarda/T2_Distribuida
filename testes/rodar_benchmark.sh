#!/usr/bin/env bash
set -euo pipefail

mpicc -Wall -O2 -o busca_logs main.c dinamico.c estatico-v2.c

TERMO="${1:-ERROR}"
REPS="${2:-5}"
OUT_CSV="resultados.csv"

echo "dataset,modo,np,workers,rep,tempo_s" > "$OUT_CSV"

# np = 2, 3, 5, 9, 12 -> workers = 1, 2, 4, 8, 11
NPS=(2 3 5 9 12)
DATASETS=(dataset_A dataset_B dataset_C dataset_D dataset_E)
MODOS=(static dynamic)

for ds in "${DATASETS[@]}"; do
  dir_path="datasets/$ds"
  if [ ! -d "$dir_path" ]; then
    echo "Diretorio ausente: $dir_path (rode gerar_datasets.sh antes)"
    exit 1
  fi

  for modo in "${MODOS[@]}"; do
    for np in "${NPS[@]}"; do
      workers=$((np - 1))
      for rep in $(seq 1 "$REPS"); do
        # Captura o último número antes de 'segundos' na linha 'Tempo de execucao:'
        tempo=$(mpiexec -np "$np" ./busca_logs --dir "$dir_path" --search "$TERMO" --mode "$modo" \
          | awk '/Tempo de execucao:/ {print $(NF-1)}' \
          | tail -n 1)

        if [ -z "${tempo:-}" ]; then
          echo "Falha ao extrair tempo em ds=$ds modo=$modo np=$np rep=$rep"
          exit 1
        fi

        echo "$ds,$modo,$np,$workers,$rep,$tempo" >> "$OUT_CSV"
      done
    done
  done
done

echo "Concluido! Dados salvos em $OUT_CSV"