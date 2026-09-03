/* raii.cpp - a exceção atravessando a seção crítica.
 *
 * O exemplo habitual de RAII é o `return` antecipado, e ele é fraco: um revisor
 * atento encontra um return sem unlock. O caminho que de fato escapa é a
 * exceção, porque ela não está escrita na função.
 *
 * O programa faz a MESMA operação lançar a MESMA exceção nos dois estilos, e
 * mede o que interessa: se a trava volta a ser livre. Com unlock explícito ela
 * fica presa, e o desfecho não é o esperado - o chamador captura, registra o
 * erro e o programa segue, de modo que um observador conclui que a falha foi
 * tratada.
 *
 *     g++ -std=c++17 -Wall -Wextra -pthread raii.cpp -o raii
 *     ./raii
 */
#include <chrono>
#include <cstdio>
#include <mutex>
#include <stdexcept>
#include <thread>

static std::mutex m;

static void debitar(long v) {
    if (v > 100) throw std::runtime_error("saldo insuficiente");
}

/* Estilo POSIX: a liberacao e uma linha, e a excecao salta por cima dela. */
static void transferir_explicito(long v) {
    m.lock();
    debitar(v);              /* se lancar, o unlock abaixo NUNCA roda */
    m.unlock();
}

/* Estilo RAII: o destrutor roda por qualquer caminho de saida do escopo. */
static void transferir_raii(long v) {
    std::lock_guard<std::mutex> g(m);
    debitar(v);              /* se lancar, ~lock_guard() libera na desmontagem */
}

/* Tenta adquirir a trava numa OUTRA thread: se ela conseguir, a trava esta
   livre. try_lock da propria thread nao serve, porque std::mutex nao e
   recursivo e o comportamento seria indefinido. */
static bool trava_livre() {
    bool livre = false;
    std::thread t([&] {
        livre = m.try_lock();
        if (livre) m.unlock();
    });
    t.join();
    return livre;
}

int main() {
    std::printf("estilo            excecao tratada   trava ao fim\n");

    try { transferir_explicito(200); }
    catch (const std::exception& e) { std::printf("unlock explicito  sim (%-14s) ", e.what()); }
    std::printf("%s\n", trava_livre() ? "livre" : "PRESA");

    /* a trava ficou presa; solta-se aqui so para o segundo teste ser justo */
    if (!trava_livre()) m.unlock();

    try { transferir_raii(200); }
    catch (const std::exception& e) { std::printf("lock_guard        sim (%-14s) ", e.what()); }
    std::printf("%s\n", trava_livre() ? "livre" : "PRESA");

    std::printf("\nCom unlock explicito o programa NAO morre: o erro e capturado,\n"
                "registrado, e a execucao segue - com a trava presa para todos.\n"
                "A proxima thread que a pedir bloqueia para sempre, longe da causa.\n");
    return 0;
}
