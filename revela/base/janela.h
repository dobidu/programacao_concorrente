/* janela.h - alargamento deliberado da janela de uma seção crítica.
 *
 * O laboratório não tem ThreadSanitizer nem Helgrind. Sem detector automático, o
 * caminho para tornar uma race reproduzível é alargar a janela entre a leitura e a
 * escrita, para que o escalonador tenha onde entrar.
 *
 * Compile com -DREVELA_AMPLIFICA para ativar. Sem a macro, JANELA() não gera
 * instrução nenhuma e o programa roda em velocidade normal.
 *
 *     gcc -Wall -Wextra -pthread -DREVELA_AMPLIFICA ...
 *
 * Isto não *cria* o defeito: um programa correto continua correto com a janela
 * aberta. Ela apenas torna provável o que já era possível.
 */
#ifndef REVELA_JANELA_H
#define REVELA_JANELA_H

#ifdef REVELA_AMPLIFICA
#include <sched.h>
#define JANELA() sched_yield()
#else
#define JANELA() ((void)0)
#endif

#endif
