// seqmod_assembler.cpp – Mersenne primality via a(n) = (2+√3)^(2^n)
//
// Criterion:
//   Mod[ Ceil[ (2+√3)^(2^n) ], 2^(n+2)−1 ] = 0  ⟺  M_{n+2} = 2^(n+2)−1  is prime.
//
// Mathematical equivalence to Lucas–Lehmer:
//   (2−√3)^(2^k) ∈ (0,1) for all k ≥ 0, so Ceil[ (2+√3)^(2^k) ] = S_k exactly,
//   where  S_0 = 4,  S_k = S_{k-1}^2 − 2.
//   M_p = 2^p − 1  is prime  iff  S_{p-2} ≡ 0 (mod M_p).
//   This is the Lucas–Lehmer test.
//
// Computation:
//   All arithmetic is delegated to mersenne::lucas_lehmer() from BigNum.cpp.
//   The backend is chosen automatically by exponent size:
//     p < 128           → GenericBackend  (boost::multiprecision::cpp_int)
//     128 ≤ p < ~4000   → LimbBackend     (Comba squaring + Mersenne fold)
//     p ≥ ~4000         → FftMersenneBackend (Crandall–Bailey DWT/FFT)
//   Override the Limb↔FFT crossover at runtime with LL_LIMB_FFT_CROSSOVER.
//
// This file contains no big-integer arithmetic of its own — it is a thin
// dispatch layer over BigNum's fully optimised, profiled engine.  benchmark_mode
// is always set to true so the already-sieved exponents skip the redundant
// is_prime_exponent() check inside lucas_lehmer().
//
// Build (release):
//   g++ -std=c++20 -O3 -march=native -mtune=native -flto -pthread
//       -DBIGNUM_NO_MAIN src/BigNum.cpp src/seqmod_assembler.cpp
//       -o bin/seqmod_assembler -flto -pthread
//
// Build (gprof):
//   g++ -std=c++20 -O2 -march=native -pthread -pg
//       -DBIGNUM_NO_MAIN src/BigNum.cpp src/seqmod_assembler.cpp
//       -o bin/seqmod_assembler_prof -pthread -pg
//
// Usage:
//   seqmod_assembler [prime_count [start_n [threads]]]
//     prime_count – number of prime exponents to test  (default: 512)
//     start_n     – first candidate n                  (default: 2)
//     threads     – parallel workers                   (default: 4; 0 = hwconcurrency)
//
// Environment:
//   SEQMOD_OUTPUT_CSV      – write "n,is_prime" CSV to this path
//   SEQMOD_TIME_LIMIT_SECS – soft stop after N seconds (exit 42)
//   SEQMOD_STATE_FILE      – write JSON resume state on exit
//   SEQMOD_ASM_THREADS     – override the threads argument
//   LL_LIMB_FFT_CROSSOVER  – override BigNum's Limb↔FFT crossover exponent
//   LL_FFT_THREADS         – FFT-internal thread count (passed through to BigNum)
//   LL_FFT_ALLOW_NESTED    – allow FFT threading inside outer worker pool
//
// Exit codes: 0 = done, 42 = soft-stop, 1 = error

#include <algorithm>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// ─── BigNum engine forward declaration ────────────────────────────────────────
// Defined in BigNum.cpp (compiled with -DBIGNUM_NO_MAIN so its main() is omitted).
// benchmark_mode=true skips the redundant is_prime_exponent() check — callers
// already guarantee that p is prime via the sieve.
namespace mersenne {
bool lucas_lehmer(uint32_t p, bool progress, bool benchmark_mode);
} // namespace mersenne

// ─── Exit codes ───────────────────────────────────────────────────────────────
static constexpr int EXIT_DONE    = 0;
static constexpr int EXIT_TIMEOUT = 42;
static constexpr int EXIT_ERROR   = 1;

// ─── Sieve of Eratosthenes ────────────────────────────────────────────────────
// Returns a vector of the first `want` primes ≥ start_n.
static std::vector<int> sieve_primes(int start_n, int want) {
    if (want <= 0 || start_n < 2) return {};

    // Rosser upper bound for the (want + offset)-th prime.
    int extra = 0;
    if (start_n > 2) {
        const double lns = std::log(static_cast<double>(start_n));
        extra = static_cast<int>(1.1 * start_n / lns) + 200;
    }
    const int N     = want + extra + 200;
    const double lnN = std::log(static_cast<double>(N + 2));
    int limit = static_cast<int>(static_cast<double>(N) * (lnN + std::log(lnN) + 1.1)) + 2000;
    if (limit < 2000) limit = 2000;

    std::vector<bool> comp(static_cast<size_t>(limit + 1), false);
    comp[0] = comp[1] = true;
    for (int i = 2; static_cast<long long>(i) * i <= limit; i++)
        if (!comp[i])
            for (int j = i * i; j <= limit; j += i)
                comp[j] = true;

    std::vector<int> out;
    out.reserve(static_cast<size_t>(want));
    for (int i = (start_n < 2 ? 2 : start_n); i <= limit && static_cast<int>(out.size()) < want; i++)
        if (!comp[i]) out.push_back(i);
    return out;
}

// ─── Persistent thread-pool worker slot ───────────────────────────────────────
// Each slot has its own mutex + two condition variables so main and worker
// communicate without a shared queue.  All dynamic memory is allocated once
// at startup; zero allocation inside the dispatch/collect hot path.
struct WorkSlot {
    // Written by main before signalling cv_start; read by worker.
    uint32_t p        = 0u;
    // Written by worker before signalling cv_done; read by main.
    bool     result   = false;
    double   elapsed  = 0.0;
    // Synchronisation state.
    bool     has_work = false;
    bool     ready    = false;

    std::mutex              mu;
    std::condition_variable cv_start;
    std::condition_variable cv_done;
};

static void worker_fn(WorkSlot* sl) {
    for (;;) {
        // Wait for work.
        {
            std::unique_lock<std::mutex> lk(sl->mu);
            sl->cv_start.wait(lk, [sl] { return sl->has_work; });
            sl->has_work = false;
        }
        const uint32_t p = sl->p;
        if (p == 0u) break; // shutdown sentinel

        // Run Lucas–Lehmer via BigNum's backend.
        // progress=false: no per-iteration stdout noise from BigNum.
        // benchmark_mode=true: skip redundant is_prime_exponent() inside LL.
        const auto t0 = std::chrono::steady_clock::now();
        const bool ok  = mersenne::lucas_lehmer(p, /*progress=*/false, /*benchmark_mode=*/true);
        const auto t1 = std::chrono::steady_clock::now();

        // Publish result.
        {
            std::lock_guard<std::mutex> lk(sl->mu);
            sl->result  = ok;
            sl->elapsed = std::chrono::duration<double>(t1 - t0).count();
            sl->ready   = true;
            sl->cv_done.notify_one();
        }
    }
}

// ─── main ─────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    // ── Parse arguments ─────────────────────────────────────────────────────
    int prime_count = (argc > 1) ? std::atoi(argv[1]) : 512;
    int start_n     = (argc > 2) ? std::atoi(argv[2]) : 2;
    int nthreads    = (argc > 3) ? std::atoi(argv[3]) : 4;

    // Environment overrides (same naming as old seqmod_assembler.c).
    if (const char* e = std::getenv("SEQMOD_ASM_THREADS"))
        nthreads = std::atoi(e);

    if (prime_count < 1) prime_count = 1;
    if (start_n     < 2) start_n     = 2;
    // threads=0 → all hardware cores.
    if (nthreads == 0) {
        const int hw = static_cast<int>(std::thread::hardware_concurrency());
        nthreads = (hw > 0) ? hw : 4;
    }
    if (nthreads < 1)   nthreads = 1;
    if (nthreads > 256) nthreads = 256;

    // ── Soft-stop timer ─────────────────────────────────────────────────────
    double time_limit = 0.0;
    if (const char* e = std::getenv("SEQMOD_TIME_LIMIT_SECS"))
        time_limit = std::atof(e);

    // ── Output paths ─────────────────────────────────────────────────────────
    const char* csv_path   = std::getenv("SEQMOD_OUTPUT_CSV");
    const char* state_path = std::getenv("SEQMOD_STATE_FILE");

    // ── Build the prime candidate list via sieve ─────────────────────────────
    const std::vector<int> primes = sieve_primes(start_n, prime_count);
    const int nprimes = static_cast<int>(primes.size());

    std::fprintf(stderr,
        "seqmod_assembler v2.0.0 (bignum engine): "
        "prime_count=%d  start_n=%d  threads=%d%s\n",
        nprimes, start_n, nthreads,
        time_limit > 0.0 ? "  (time-limited)" : "");

    // ── Thread pool ──────────────────────────────────────────────────────────
    // Slots and threads are allocated once; no allocation in the dispatch loop.
    std::vector<WorkSlot> slots(static_cast<size_t>(nthreads));
    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(nthreads));
    for (int i = 0; i < nthreads; i++)
        threads.emplace_back(worker_fn, &slots[static_cast<size_t>(i)]);

    // ── Dispatch helpers ─────────────────────────────────────────────────────
    const auto dispatch = [&](int slot, uint32_t p) {
        WorkSlot& sl = slots[static_cast<size_t>(slot)];
        std::lock_guard<std::mutex> lk(sl.mu);
        sl.p        = p;
        sl.ready    = false;
        sl.has_work = true;
        sl.cv_start.notify_one();
    };

    const auto collect = [&](int slot, bool& result, double& elapsed) {
        WorkSlot& sl = slots[static_cast<size_t>(slot)];
        std::unique_lock<std::mutex> lk(sl.mu);
        sl.cv_done.wait(lk, [&sl] { return sl.ready; });
        result  = sl.result;
        elapsed = sl.elapsed;
    };

    // ── Result accumulators ──────────────────────────────────────────────────
    std::vector<std::pair<int, bool>> csv_rows;
    if (csv_path && *csv_path) csv_rows.reserve(static_cast<size_t>(nprimes));
    std::vector<int> hits;

    // ── Pipeline dispatch loop ───────────────────────────────────────────────
    // Maintain a sliding window of nthreads in-flight exponents so every
    // worker is always busy (pipeline).  Slots are addressed round-robin.
    const auto t_start    = std::chrono::steady_clock::now();
    auto       t_last_prog = t_start;
    int  done               = 0;
    bool timed_out          = false;
    int  last_dispatched_n  = start_n - 1;

    int flight_base = 0; // next slot to collect from (index into primes[])
    int flight_top  = 0; // next slot to dispatch to  (index into primes[])
    int next_prime  = 0; // index of next prime to dispatch

    // Pre-fill the pipeline.
    for (int s = 0; s < nthreads && next_prime < nprimes; s++, next_prime++, flight_top++)
        dispatch(s, static_cast<uint32_t>(primes[next_prime]));

    while (flight_base < flight_top) {
        const int slot = flight_base % nthreads;
        bool   result; double elapsed;
        collect(slot, result, elapsed);
        flight_base++;
        done++;

        const int p = primes[flight_base - 1];
        last_dispatched_n = p;
        if (result) hits.push_back(p);
        if (csv_path && *csv_path) csv_rows.emplace_back(p, result);

        // Progress output: sparse (every 64 completions or every 10 s).
        const auto now = std::chrono::steady_clock::now();
        const double wall = std::chrono::duration<double>(now - t_start).count();
        const double since_prog = std::chrono::duration<double>(now - t_last_prog).count();
        if (done == 1 || done % 64 == 0 || since_prog >= 10.0) {
            const double pct = (nprimes > 0) ? 100.0 * done / nprimes : 0.0;
            std::fprintf(stderr,
                "  [%6.1f s] %d/%d (%.1f%%)  primes_found=%d  last_p=%d  last=%.3f s\n",
                wall, done, nprimes, pct,
                static_cast<int>(hits.size()), p, elapsed);
            t_last_prog = now;
        }

        // Soft-stop check.
        if (time_limit > 0.0 && wall >= time_limit) {
            timed_out = true;
            break;
        }

        // Dispatch the next exponent to the slot just freed.
        if (next_prime < nprimes) {
            dispatch(slot, static_cast<uint32_t>(primes[next_prime]));
            next_prime++;
            flight_top++;
        }
    }

    // ── Drain in-flight slots on soft-stop ───────────────────────────────────
    // Workers have already been given work; drain to avoid corrupted results.
    while (flight_base < flight_top) {
        bool r; double e;
        collect(flight_base % nthreads, r, e);
        flight_base++;
    }

    // ── Shut down thread pool ────────────────────────────────────────────────
    for (int i = 0; i < nthreads; i++) dispatch(i, 0u); // sentinel p=0
    for (auto& t : threads) t.join();

    // ── Write CSV ────────────────────────────────────────────────────────────
    if (csv_path && *csv_path && !csv_rows.empty()) {
        std::sort(csv_rows.begin(), csv_rows.end());
        std::ofstream f(csv_path);
        if (f) {
            f << "n,is_prime\n";
            for (const auto& [n, ok] : csv_rows)
                f << n << ',' << (ok ? "true" : "false") << '\n';
        } else {
            std::fprintf(stderr, "seqmod_assembler: cannot open CSV file: %s\n", csv_path);
        }
    }

    // ── Write JSON state (for resume support) ────────────────────────────────
    if (state_path && *state_path) {
        std::ofstream f(state_path);
        if (f) {
            f << "{\n"
              << "  \"start_n\": " << start_n << ",\n"
              << "  \"last_dispatched_n\": " << last_dispatched_n << ",\n"
              << "  \"timed_out\": " << (timed_out ? "true" : "false") << ",\n"
              << "  \"mersenne_primes_found\": [";
            for (size_t i = 0; i < hits.size(); i++) {
                if (i) f << ", ";
                f << hits[i];
            }
            f << "]\n}\n";
        }
    }

    // ── Summary ──────────────────────────────────────────────────────────────
    const double total = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - t_start).count();
    std::fprintf(stderr,
        "seqmod_assembler: done=%d  mersenne_primes=%d  "
        "wall=%.3f s  avg=%.3f ms/prime\n",
        done, static_cast<int>(hits.size()), total,
        done > 0 ? 1000.0 * total / done : 0.0);

    if (timed_out) {
        std::printf("SEQMOD_LAST_DISPATCHED_N=%d\n", last_dispatched_n);
        return EXIT_TIMEOUT;
    }
    return EXIT_DONE;
}
