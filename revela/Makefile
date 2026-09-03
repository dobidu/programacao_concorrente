# Revela - pipeline de tratamento de imagem da disciplina.
#
# Sem sanitizer: as máquinas do laboratório têm gcc, g++, pthreads, semaphore.h e
# gdb, e o portão é `make verifica`. O alvo `amplifica` recompila com a janela da
# seção crítica alargada, que é como se torna uma race reproduzível à mão.

CC      := gcc
CXX     := g++
CFLAGS  := -Wall -Wextra -pthread -O2 -I base
CXXFLAGS:= -std=c++17 -Wall -Wextra -pthread -O2 -I base
LDLIBS  := -pthread

BASE_SRC := base/ppm.c base/filtro.c base/revela.c
BASE_OBJ := $(BASE_SRC:.c=.o)
BASE     := base/librevela.a
LARGURA ?= 1200
ALTURA  ?= 800
ENTRADA := entrada.ppm

LABS := 01 02 03 04 05 06 07 08 09 10 11 12
ALVOS := $(foreach n,$(LABS),lab$(n)/esqueleto lab$(n)/solucao)

.PHONY: all laboratorios entrada verifica amplifica limpa $(foreach n,$(LABS),lab$(n))

all: base/revela_seq base/gerar_imagem laboratorios

laboratorios: $(ALVOS)

# `make lab03` compila só aquele laboratório.
$(foreach n,$(LABS),$(eval lab$(n): lab$(n)/esqueleto lab$(n)/solucao))

# Um laboratório é C ou C++17 conforme a extensão do fonte. O Cap. 9 é em C++, e o
# resto em C - a regra dupla evita um Makefile por laboratório.
lab%/esqueleto: lab%/esqueleto.c $(BASE)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

lab%/solucao: lab%/solucao.c $(BASE)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

lab%/esqueleto: lab%/esqueleto.cpp $(BASE)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

lab%/solucao: lab%/solucao.cpp $(BASE)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

# A base é sempre compilada como C, mesmo quando o laboratório é C++: um .c passado
# ao g++ vira C++ e nem compila (void* não converte implicitamente).
base/%.o: base/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BASE): $(BASE_OBJ)
	ar rcs $@ $^

base/revela_seq: base/revela_seq.c $(BASE)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

base/gerar_imagem: base/gerar_imagem.c base/ppm.o
	$(CC) $(CFLAGS) $^ -o $@

# A imagem de entrada é gerada, não versionada: mesma entrada em toda máquina.
entrada: $(ENTRADA)
$(ENTRADA): base/gerar_imagem
	./base/gerar_imagem $@ $(LARGURA) $(ALTURA)

# Recompila tudo com a janela aberta. Um programa correto passa assim também.
amplifica: CFLAGS += -DREVELA_AMPLIFICA
amplifica: CXXFLAGS += -DREVELA_AMPLIFICA
amplifica: limpa all

# Roda o portão em todas as soluções de referência. LAB-03 é quebrado de propósito
# e por isso é esperado que falhe sob amplificação - ele não entra aqui.
verifica: all entrada
	@./verifica.sh ./lab01/solucao $(ENTRADA) --filtro cinza
	@./verifica.sh ./lab02/solucao $(ENTRADA) --filtro cinza
	@./verifica.sh ./lab04/solucao $(ENTRADA) --filtro cinza --threads 4
	@./verifica.sh ./lab05/solucao $(ENTRADA) --threads 8 --modo mutex
	@./verifica.sh ./lab05/solucao $(ENTRADA) --threads 8 --modo atomico
	@./verifica.sh ./lab05/solucao $(ENTRADA) --threads 8 --modo reducao
	@./verifica.sh ./lab06/solucao $(ENTRADA) --threads 8
	@./verifica.sh ./lab07/solucao $(ENTRADA) --threads 8 --modo ordenado
	@./verifica.sh ./lab08/solucao $(ENTRADA) --filtro desfoque --threads 8
	@./verifica.sh ./lab09/solucao $(ENTRADA) --threads 8
	@./verifica.sh ./lab10/solucao $(ENTRADA) --filtro cinza
	@./verifica.sh ./lab11/solucao $(ENTRADA) --clientes 4
	@./verifica.sh ./lab12/solucao $(ENTRADA) --clientes 4

limpa:
	rm -f base/revela_seq base/gerar_imagem $(ALVOS) $(BASE) $(BASE_OBJ)
	rm -f divergencia-*.ppm divergencia-*.txt
