# Makefile global — LPII Programação Concorrente
# Compila TODOS os exemplos de todos os módulos

SUBDIRS := $(shell find modulo1-fundamentos modulo2-sincronizacao modulo3-comunicacao minichat -name Makefile -exec dirname {} \;)

.PHONY: all clean modulo1 modulo2 modulo3 minichat

all:
	@echo "=== Compilando todos os $(words $(SUBDIRS)) exemplos ==="
	@OK=0; FAIL=0; \
	for dir in $(SUBDIRS); do \
		if $(MAKE) -C $$dir -s 2>/dev/null; then \
			OK=$$((OK+1)); \
		else \
			echo "FALHA: $$dir"; \
			FAIL=$$((FAIL+1)); \
		fi; \
	done; \
	echo "=== OK: $$OK | Falhas: $$FAIL ==="

clean:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean -s 2>/dev/null; done
	@echo "Todos os binários removidos."

modulo1:
	@for dir in $(shell find modulo1-fundamentos -name Makefile -exec dirname {} \;); do $(MAKE) -C $$dir; done

modulo2:
	@for dir in $(shell find modulo2-sincronizacao -name Makefile -exec dirname {} \;); do $(MAKE) -C $$dir; done

modulo3:
	@for dir in $(shell find modulo3-comunicacao -name Makefile -exec dirname {} \;); do $(MAKE) -C $$dir; done

minichat:
	@for dir in $(shell find minichat -name Makefile -exec dirname {} \;); do $(MAKE) -C $$dir; done
