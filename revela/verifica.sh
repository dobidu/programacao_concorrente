#!/usr/bin/env bash
# verifica.sh - o portão do Revela, sem sanitizer nenhum.
#
# As máquinas do laboratório têm gcc, pthreads e gdb. Não têm ThreadSanitizer nem
# Helgrind. Este script é o que sobra - e é suficiente, porque testa a propriedade
# que de fato importa: o resultado não pode depender do escalonador.
#
#   1. compara a saída do candidato com a do oráculo sequencial, byte a byte;
#   2. roda o candidato N vezes e exige resultado idêntico em TODAS.
#
# Uma race que sobrevive a 200 execuções com a janela alargada por sched_yield()
# não é sorte de escalonamento: é ausência de race. Uma que não sobrevive aparece
# aqui, com o número da execução em que divergiu.
#
#   ./verifica.sh ./lab03/solucao entrada.ppm --threads 8
#
set -uo pipefail

REPETICOES="${REPETICOES:-200}"
LIMITE_S="${LIMITE_S:-20}"     # um programa que trava é falha, não espera eterna
ORACULO="${ORACULO:-./base/revela_seq}"

if [ $# -lt 2 ]; then
    echo "uso: $0 <candidato> <entrada.ppm> [args extras do candidato...]" >&2
    exit 2
fi

CANDIDATO="$1"; shift
ENTRADA="$1"; shift

tmp=$(mktemp -d) || exit 2
trap 'rm -rf "$tmp"' EXIT

falhar() { printf '\033[31mFALHOU\033[0m  %s\n' "$1"; exit 1; }
passar() { printf '\033[32mPASSOU\033[0m  %s\n' "$1"; }

# ── 1. o oráculo ──
if [ ! -x "$ORACULO" ]; then
    echo "oráculo não encontrado em $ORACULO - rode 'make' primeiro" >&2
    exit 2
fi
"$ORACULO" "$ENTRADA" "$tmp/oraculo.ppm" "$@" > "$tmp/oraculo.txt" 2>/dev/null \
    || falhar "o próprio oráculo não rodou"
ref=$(cat "$tmp/oraculo.ppm" "$tmp/oraculo.txt" | sha256sum | cut -d' ' -f1)

# ── 2. o candidato, muitas vezes ──
divergiu=0
for i in $(seq 1 "$REPETICOES"); do
    # O status vem numa linha própria: depois de `if ! cmd`, $? é o da negação.
    timeout "$LIMITE_S" "$CANDIDATO" "$ENTRADA" "$tmp/saida.ppm" "$@" \
        > "$tmp/saida.txt" 2>/dev/null
    estado=$?
    if [ "$estado" -ne 0 ]; then
        if [ "$estado" -eq 124 ]; then
            echo
            echo "  A execução $i não terminou em ${LIMITE_S}s: o programa travou."
            echo "  Rode de novo em outro terminal e olhe onde ele parou:"
            echo "      gdb -p \$(pgrep -n -f '$(basename "$CANDIDATO")')"
            echo "      (gdb) thread apply all bt"
            echo
            falhar "$CANDIDATO travou"
        fi
        falhar "o candidato saiu com erro ($estado) na execução $i"
    fi
    h=$(cat "$tmp/saida.ppm" "$tmp/saida.txt" | sha256sum | cut -d' ' -f1)
    if [ "$h" != "$ref" ]; then
        divergiu=$i
        cp "$tmp/saida.ppm" "divergencia-$i.ppm" 2>/dev/null
        cp "$tmp/saida.txt" "divergencia-$i.txt" 2>/dev/null
        break
    fi
done

if [ "$divergiu" -ne 0 ]; then
    echo
    echo "  execução $divergiu de $REPETICOES divergiu do oráculo."
    echo "  esperado: $(cat "$tmp/oraculo.txt")"
    echo "  obtido:   $(cat "$tmp/saida.txt")"
    echo
    echo "  A saída divergente ficou em divergencia-$divergiu.ppm - abra e olhe."
    echo "  Se o resumo bate mas a imagem não, o defeito está nos pixels;"
    echo "  se o hist= mudou, um incremento do histograma se perdeu."
    falhar "$CANDIDATO não é determinístico"
fi

passar "$REPETICOES execuções idênticas ao oráculo"
