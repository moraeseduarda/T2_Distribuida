#!/usr/bin/env bash
set -euo pipefail

# Garante que o gerador está compilado
gcc -Wall -O2 -o gerador_logs_t2 gerador_logs_t2.c

BASE_DIR="datasets"
TMP_DIR="_tmp_gen"
PALAVRAS=8
SEED=12345

rm -rf "$BASE_DIR" "$TMP_DIR"
mkdir -p "$BASE_DIR" "$TMP_DIR"

# Dataset A: 66 arquivos x 5.000 linhas (uniforme, pequeno)
./gerador_logs_t2 66 5000 "$PALAVRAS" "$SEED" "$BASE_DIR/dataset_A"

# Dataset B: 132 arquivos x 40.000 linhas (uniforme, grande)
./gerador_logs_t2 132 40000 "$PALAVRAS" "$SEED" "$BASE_DIR/dataset_B"

# Dataset C: 6 de 80.000 linhas nas posições 1..6 + 126 de 4.000 linhas nas posições 7..132
mkdir -p "$BASE_DIR/dataset_C"
./gerador_logs_t2 6 80000 "$PALAVRAS" "$SEED" "$TMP_DIR/c_grande"
./gerador_logs_t2 126 4000 "$PALAVRAS" "$SEED" "$TMP_DIR/c_pequeno"

idx=1
for f in "$TMP_DIR/c_grande"/log_*.log; do
  cp "$f" "$(printf '%s/log_%04d.log' "$BASE_DIR/dataset_C" "$idx")"
  idx=$((idx + 1))
done
for f in "$TMP_DIR/c_pequeno"/log_*.log; do
  cp "$f" "$(printf '%s/log_%04d.log' "$BASE_DIR/dataset_C" "$idx")"
  idx=$((idx + 1))
done

# Dataset D: 12 de 80.000 linhas nas posições 1, 12, 23, ... + 120 de 4.000 linhas (total 132)
mkdir -p "$BASE_DIR/dataset_D"
./gerador_logs_t2 12 80000 "$PALAVRAS" "$SEED" "$TMP_DIR/d_grande"
./gerador_logs_t2 120 4000 "$PALAVRAS" "$SEED" "$TMP_DIR/d_pequeno"

# Posições 1-based: 1, 12, 23, 34, 45, 56, 67, 78, 89, 100, 111, 122
grande_files=("$TMP_DIR/d_grande"/log_*.log)
peq_files=("$TMP_DIR/d_pequeno"/log_*.log)
g_idx=0
p_idx=0

for pos in $(seq 1 132); do
  is_big=0
  for k in $(seq 0 11); do
    big_pos=$((1 + k * 11))
    if [ "$pos" -eq "$big_pos" ]; then
      is_big=1
      break
    fi
  done

  out_name=$(printf '%s/log_%04d.log' "$BASE_DIR/dataset_D" "$pos")
  if [ "$is_big" -eq 1 ]; then
    cp "${grande_files[$g_idx]}" "$out_name"
    g_idx=$((g_idx + 1))
  else
    cp "${peq_files[$p_idx]}" "$out_name"
    p_idx=$((p_idx + 1))
  fi
done

# Dataset E: 990 arquivos x 300 linhas (muitos arquivos pequenos)
./gerador_logs_t2 990 300 "$PALAVRAS" "$SEED" "$BASE_DIR/dataset_E"

rm -rf "$TMP_DIR"
echo "Datasets A-E gerados em $BASE_DIR/"