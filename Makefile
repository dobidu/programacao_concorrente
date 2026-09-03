# Makefile - compila TODOS os exemplos com o portão de qualidade do projeto.
#
# -Werror não é zelo: o material afirma "zero warnings", e gcc devolve 0 com
# warnings na tela. Dizer "compila sem avisos" a partir do código de saída é
# falso, e foi assim que um `usleep` sem _XOPEN_SOURCE quase entrou.
#
#     make            compila tudo
#     make roda       compila e executa cada um (alguns levam segundos)
#     make limpa
#
# O Revela tem Makefile próprio, em revela/, com o portão de 200 execuções.

CFLAGS   = -O2 -Wall -Wextra -Werror -pthread
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Werror -pthread

FONTES_C   := $(shell find c -name '*.c' 2>/dev/null) $(shell find medidas -name '*.c' 2>/dev/null)
FONTES_CPP := $(shell find cpp -name '*.cpp' 2>/dev/null)
ALVOS      := $(FONTES_C:.c=) $(FONTES_CPP:.cpp=)

.PHONY: all roda limpa
all: $(ALVOS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

%: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

roda: all
	@for x in $(ALVOS); do \
	  printf '\n=== %s ===\n' "$$x"; \
	  timeout 120 ./$$x || echo "  (saida $$? - alguns exemplos falham de proposito)"; \
	done

limpa:
	@rm -f $(ALVOS)
