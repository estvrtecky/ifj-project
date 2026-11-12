#!/bin/sh
# Spustenie testov generátora.
# Všetko (ast.h, types.h, codegen.h, codegen.c, test_codegen.c, run.sh)
# je v jednom priečinku. Spusti:   ./run.sh      (alebo  sh run.sh)
#Tento kod bol 12.11. vygenerovany Claude AI verziou Sonnet 5.5, na ziadost aby vygeneroval script na otestovanie codegen

cd "$(dirname "$0")" || exit 1

for f in ast.h types.h codegen.h codegen.c test_codegen.c; do
  if [ ! -f "$f" ]; then
    echo "CHYBA: v priečinku $(pwd) chýba súbor $f"
    echo "Obsah priečinka:"
    ls
    exit 1
  fi
done

SRC="codegen.c"
[ -f codegen_builtins.c ] && SRC="$SRC codegen_builtins.c"

gcc -Wall -Wextra -std=gnu11 -I. test_codegen.c $SRC -o test_codegen || exit 1
./test_codegen
