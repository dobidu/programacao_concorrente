# LPII — Programação Concorrente em C/C++

**Universidade Federal da Paraíba — Centro de Informática**  
**Linguagem de Programação II — Turma 02 — 2025.2**  
**Prof. Carlos Eduardo Coelho Freire Batista**

---

## 📋 Visão Geral

Repositório de exemplos de código da disciplina de Programação Concorrente. Todos os programas compilam com `gcc -Wall -Wextra -pthread` e foram testados em Ubuntu Linux.

## 🗂️ Estrutura

```
lpii-programacao-concorrente/
├── modulo1-fundamentos/        ← Processos, fork, pipes, threads
│   ├── 01-fork-basico/
│   ├── 02-fork-multiplos/
│   ├── 03-fork-exec/
│   ├── 04-pipe-basico/
│   ├── 05-pipe-bidirecional/
│   ├── 06-pipeline-multi/      ← Pipeline A | B | C
│   ├── 07-named-pipe/          ← FIFOs
│   ├── 08-estoque-fork-mmap/   ← Race condition entre processos
│   ├── 09-threads-basico/
│   ├── 10-threads-retorno/
│   ├── 11-race-condition/      ← Demonstração de race condition
│   └── 12-soma-paralela/       ← Scatter-gather sem sincronização
│
├── modulo2-sincronizacao/      ← Mutex, semáforos, barreiras, condvar
│   ├── 01-spinlock/
│   ├── 02-ticket-lock/
│   ├── 03-pthread-spinlock/
│   ├── 04-mutex-basico/        ← Correção de race condition
│   ├── 05-mutex-tipos/         ← Recursive, errorcheck
│   ├── 06-trylock/
│   ├── 07-atomics/             ← stdatomic.h (lock-free)
│   ├── 08-semaforo-pool/       ← Semáforo de contagem
│   ├── 09-semaforo-nomeado/    ← sem_open entre processos
│   ├── 10-barreira-posix/
│   ├── 11-condvar-prod-cons/   ← Produtor-consumidor
│   ├── 12-condvar-timedwait/   ← Timeout com pthread_cond_timedwait
│   ├── 13-barreira-condvar/    ← Barreira reutilizável manual
│   ├── 14-rwlock/              ← Leitores-escritores
│   ├── 15-jantar-filosofos/    ← Dining Philosophers
│   └── 16-deadlock-demo/       ← Deadlock + correção
│
├── modulo3-comunicacao/        ← Monitores, shm, sockets
│   ├── 01-monitor-estoque-c/   ← Monitor em C
│   ├── 04-shm-posix/           ← Memória compartilhada POSIX
│   ├── 06-servidor-tcp/
│   ├── 07-cliente-tcp/
│   ├── 08-servidor-tcp-mt/     ← Multi-thread + signal handler
│   ├── 09-servidor-udp/
│   ├── 10-cliente-udp/
│   └── 11-drone-telemetria/    ← Telemetria UDP (drone + central)
│
└── minichat/                   ← Projeto evolutivo
    ├── v0.1-pipe-uni/          ← Módulo 1: pipe simples
    ├── v1.0-threads-buf/       ← Módulo 2: threads SEM sync
    ├── v1.1-mutex-bug/         ← Módulo 2: mutex PARCIAL (bug!)
    ├── v1.1-mutex-fix/         ← Módulo 2: mutex CORRETO
    └── v2.0-tcp-server/        ← Módulo 3: TCP + broadcast
```

## 🚀 Como Usar

### Pré-requisitos
- GCC com suporte a C11
- Linux (Ubuntu 20.04+ recomendado)
- Bibliotecas: pthreads (padrão), librt (para shm_open)

### Compilar tudo
```bash
make all
```

### Compilar um exemplo específico
```bash
cd modulo1-fundamentos/01-fork-basico
make
./fork_basico
```

### Limpar todos os binários
```bash
make clean
```

## 📊 Tabela de Exemplos por Conceito

| Conceito | Exemplo | Módulo |
|----------|---------|--------|
| `fork()` | `01-fork-basico` | 1 |
| `fork()` + múltiplos filhos | `02-fork-multiplos` | 1 |
| `fork()` + `exec()` | `03-fork-exec` | 1 |
| `pipe()` | `04-pipe-basico` | 1 |
| Pipe bidirecional | `05-pipe-bidirecional` | 1 |
| Pipeline multi-estágio | `06-pipeline-multi` | 1 |
| Named pipes (FIFO) | `07-named-pipe` | 1 |
| Race condition (processos) | `08-estoque-fork-mmap` | 1 |
| `pthread_create/join` | `09-threads-basico` | 1 |
| Race condition (threads) | `11-race-condition` | 1 |
| Soma paralela (scatter-gather) | `12-soma-paralela` | 1 |
| Spinlock (atomic_flag) | `01-spinlock` | 2 |
| Ticket lock (FIFO justo) | `02-ticket-lock` | 2 |
| `pthread_spinlock_t` | `03-pthread-spinlock` | 2 |
| Mutex POSIX | `04-mutex-basico` | 2 |
| Mutex recursive/errorcheck | `05-mutex-tipos` | 2 |
| `pthread_mutex_trylock` | `06-trylock` | 2 |
| `stdatomic.h` (lock-free) | `07-atomics` | 2 |
| Semáforo de contagem | `08-semaforo-pool` | 2 |
| Semáforo nomeado | `09-semaforo-nomeado` | 2 |
| `pthread_barrier` | `10-barreira-posix` | 2 |
| Produtor-consumidor (condvar) | `11-condvar-prod-cons` | 2 |
| `pthread_cond_timedwait` | `12-condvar-timedwait` | 2 |
| Barreira com condvar | `13-barreira-condvar` | 2 |
| `pthread_rwlock` | `14-rwlock` | 2 |
| Jantar dos Filósofos | `15-jantar-filosofos` | 2 |
| Deadlock + correção | `16-deadlock-demo` | 2 |
| Monitor em C | `01-monitor-estoque-c` | 3 |
| Memória compartilhada POSIX | `04-shm-posix` | 3 |
| Servidor/Cliente TCP | `06/07-tcp` | 3 |
| Servidor TCP multi-thread | `08-servidor-tcp-mt` | 3 |
| Servidor/Cliente UDP | `09/10-udp` | 3 |
| Telemetria de drone (UDP) | `11-drone-telemetria` | 3 |

## 📖 Referências

- Andrews, G. R. *Foundations of Multithreaded, Parallel, and Distributed Programming*. Addison-Wesley, 2000.
- Butenhof, D. R. *Programming with POSIX Threads*. Addison-Wesley, 1997.
- Stevens, W. R. et al. *UNIX Network Programming, Vol. 1*. 3ª ed. Addison-Wesley, 2004.
- Williams, A. *C++ Concurrency in Action*. 2ª ed. Manning, 2019.

## 📄 Licença

Material didático — UFPB/CI/DI — Prof. Carlos Eduardo Batista — 2025.
