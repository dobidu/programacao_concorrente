/* LAB-08 - latch à mão, em C++17.   preparatório, sem nota
 *
 * O desfoque é separável: primeiro uma passagem horizontal, depois uma vertical
 * sobre o resultado da primeira. Cada thread cuida da sua faixa - mas a passagem
 * vertical de uma faixa lê linhas VIZINHAS, que pertencem à faixa de outra thread.
 *
 * Logo: nenhuma thread pode começar a fase vertical antes que TODAS tenham
 * terminado a horizontal. É um ponto de encontro de uso único, e o nome disso na
 * biblioteca padrão é std::latch.
 *
 * Que não existe em C++17. `std::latch` é C++20, e o laboratório tem GCC 9 - 10.
 * Então implementa-se - com o mesmo mutex e a mesma variável de condição do
 * Capítulo 7, agora em outra linguagem. É o mesmo problema, reencontrado.
 *
 *   make lab08
 *   ./verifica.sh ./lab08/solucao entrada.ppm --filtro desfoque --threads 8
 *   ./lab08/solucao entrada.ppm /tmp/s.ppm --filtro desfoque --threads 8 \
 *       --modo semlatch          # sem a barreira: saída errada, e reproduzível
 *
 * O QUE VOCÊ TEM DE CONSEGUIR FAZER: o portão passando com o latch, e a demonstração de que sem ele a
 * saída diverge. As duas metades juntas são o que se pontua - mostrar que
 * funciona não prova que a sincronização era necessária.
 */
#include "crono.h"
#include "filtro.h"
#include "revela.h"

#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

/* Ponto de encontro de uso único para um número fixo de participantes.
 *
 * Uso único é o que o distingue da barreira: depois que todos chegam, ele fica
 * aberto para sempre. Uma barreira reutilizável precisa de contagem de geração,
 * senão uma thread rápida atravessa duas vezes a mesma barreira. */
class Latch {
public:
    explicit Latch(int participantes) : restantes_(participantes) {}

    void chegar_e_esperar() {
        /* TODO: implemente o ponto de encontro.
         *   · tome o mutex;
         *   · decremente o contador;
         *   · se chegou a zero, acorde todo mundo e saia;
         *   · senão, espere até o contador zerar - com predicado, não sem.
         * Pense em por que notify_all e não notify_one. */
    }

private:
    std::mutex mutex_;
    std::condition_variable condicao_;
    int restantes_;
};

int main(int argc, char **argv) {
    Opcoes o;
    if (opcoes_ler(argc, argv, &o)) return 1;
    const bool sem_latch = std::strcmp(o.modo, "semlatch") == 0;

    Imagem entrada{}, temp{}, saida{};
    if (ppm_ler(o.entrada, &entrada)) return 1;
    if (imagem_criar(&temp, entrada.largura, entrada.altura)) return 1;
    if (imagem_criar(&saida, entrada.largura, entrada.altura)) return 1;

    Latch latch(o.threads);
    std::vector<std::thread> fios;
    std::vector<std::vector<unsigned long>> baldes(o.threads,
                                                   std::vector<unsigned long>(256, 0));

    const int por_thread = (entrada.altura + o.threads - 1) / o.threads;
    const double t0 = agora_ms();

    for (int i = 0; i < o.threads; i++) {
        fios.emplace_back([&, i] {
            int y0 = i * por_thread;
            int y1 = y0 + por_thread;
            if (y1 > entrada.altura) y1 = entrada.altura;
            if (y0 > y1) y0 = y1;

            filtro_desfoque_h(&entrada, &temp, y0, y1);

            if (!sem_latch) latch.chegar_e_esperar();   /* a fronteira das fases */

            filtro_desfoque_v(&temp, &saida, y0, y1);
            histograma_faixa(&saida, y0, y1, baldes[i].data());
        });
    }
    for (auto &f : fios) f.join();

    const double ms = agora_ms() - t0;

    unsigned long total[256] = {0};
    for (int i = 0; i < o.threads; i++)
        for (int k = 0; k < 256; k++) total[k] += baldes[i][k];

    const int erro = ppm_escrever(o.saida, &saida);
    imprimir_resumo(&o, total, static_cast<size_t>(entrada.largura) * entrada.altura);
    std::fprintf(stderr, "threads=%d latch=%s  %.2f ms\n",
                 o.threads, sem_latch ? "não" : "sim", ms);

    imagem_destruir(&entrada);
    imagem_destruir(&temp);
    imagem_destruir(&saida);
    return erro ? 1 : 0;
}
