/* coordenacao.cpp - latch, barreira e semáforo escritos à mão, em C++17.
 *
 * C++20 traz std::latch, std::barrier e std::counting_semaphore prontos; C++17,
 * que é o teto do laboratório, não os traz. Implementá-los é o exercício que
 * fecha o capítulo, e o objetivo não é suprir a ausência: as três primitivas
 * reduzem-se ao mesmo par - contador protegido por mutex e variável de condição
 * com predicado - e escrevê-las é a maneira mais direta de perceber que se
 * aprendeu um mecanismo, e não três.
 *
 * A barreira é a única em que a implementação óbvia está errada. Sem número de
 * geração, a thread rápida acorda, executa a fase seguinte, retorna à barreira
 * e é contada de novo antes que as demais tenham sido escalonadas; o contador
 * chega a zero com uma thread faltando, e o ciclo seguinte trava. O programa
 * roda as DUAS versões e mostra a diferença.
 *
 *     g++ -std=c++17 -Wall -Wextra -pthread coordenacao.cpp -o coordenacao
 *     ./coordenacao
 */
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

class Latch {
    std::mutex m_;
    std::condition_variable cv_;
    int n_;
public:
    explicit Latch(int n) : n_(n) {}
    void count_down() {
        std::lock_guard<std::mutex> g(m_);
        if (--n_ == 0) cv_.notify_all();
    }
    void wait() {
        std::unique_lock<std::mutex> g(m_);
        cv_.wait(g, [this] { return n_ == 0; });
    }
};

/* ERRADA de proposito: o predicado nao distingue ciclos. */
class BarreiraIngenua {
    std::mutex m_;
    std::condition_variable cv_;
    const int n_;
    int faltam_;
public:
    explicit BarreiraIngenua(int n) : n_(n), faltam_(n) {}
    bool wait(std::chrono::milliseconds limite) {
        std::unique_lock<std::mutex> g(m_);
        if (--faltam_ == 0) { faltam_ = n_; cv_.notify_all(); return true; }
        return cv_.wait_for(g, limite, [this] { return faltam_ == n_; });
    }
};

class Barreira {
    std::mutex m_;
    std::condition_variable cv_;
    const int n_;
    int faltam_;
    unsigned long geracao_ = 0;
public:
    explicit Barreira(int n) : n_(n), faltam_(n) {}
    bool wait(std::chrono::milliseconds limite) {
        std::unique_lock<std::mutex> g(m_);
        unsigned long minha = geracao_;
        if (--faltam_ == 0) {
            faltam_ = n_;
            ++geracao_;
            cv_.notify_all();
            return true;
        }
        return cv_.wait_for(g, limite, [this, minha] { return geracao_ != minha; });
    }
};

class Semaforo {
    std::mutex m_;
    std::condition_variable cv_;
    int n_;
public:
    explicit Semaforo(int n) : n_(n) {}
    void acquire() {
        std::unique_lock<std::mutex> g(m_);
        cv_.wait(g, [this] { return n_ > 0; });
        --n_;
    }
    void release() {
        { std::lock_guard<std::mutex> g(m_); ++n_; }
        cv_.notify_one();
    }
};

template <class B>
static int rodar_fases(B& b, int threads, int fases) {
    std::atomic<int> completas{0};
    std::vector<std::thread> t;
    for (int i = 0; i < threads; i++) {
        t.emplace_back([&, i] {
            for (int f = 0; f < fases; f++) {
                /* a thread 0 e propositalmente muito mais rapida */
                if (i != 0) std::this_thread::sleep_for(std::chrono::milliseconds(2));
                if (!b.wait(std::chrono::milliseconds(400))) return;   /* travou */
            }
            completas++;
        });
    }
    for (auto& x : t) x.join();
    return completas;
}

int main() {
    const int N = 4, FASES = 30;

    Latch l(N);
    std::vector<std::thread> t;
    for (int i = 0; i < N; i++) t.emplace_back([&] { l.count_down(); });
    l.wait();
    for (auto& x : t) x.join();
    std::printf("latch de %d:                 liberou apos %d count_down\n", N, N);

    Semaforo s(2);
    std::atomic<int> dentro{0}, pico{0};
    t.clear();
    for (int i = 0; i < 8; i++) {
        t.emplace_back([&] {
            s.acquire();
            int d = ++dentro;
            int p = pico.load();
            while (d > p && !pico.compare_exchange_weak(p, d)) {}
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            --dentro;
            s.release();
        });
    }
    for (auto& x : t) x.join();
    std::printf("semaforo de 2, 8 threads:   pico de %d simultaneas\n", pico.load());

    BarreiraIngenua bi(N);
    std::printf("barreira SEM geracao:       %d de %d threads completaram %d fases\n",
                rodar_fases(bi, N, FASES), N, FASES);

    Barreira bg(N);
    std::printf("barreira COM geracao:       %d de %d threads completaram %d fases\n",
                rodar_fases(bg, N, FASES), N, FASES);

    std::printf("\nA versao sem geracao trava porque a thread rapida reentra e e\n"
                "contada duas vezes no mesmo ciclo. So acontece quando ela e rapida\n"
                "o bastante - por isso nao aparece em teste leve, aparece sob carga.\n");
    return 0;
}
