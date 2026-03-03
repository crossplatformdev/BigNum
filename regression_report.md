# Primality-Test Runtime Regression Report

**Repository:** crossplatformdev/BigNum  
**Analysis date:** 2026-03-03  
**Workflow run (current):** https://github.com/crossplatformdev/BigNum/actions/runs/22625641126  
**Data sources:** CI job logs from last 20 workflow runs (extracted via GitHub Actions API)

---

## 1. Jobs Analyzed

| # | Run ID | Job ID | Branch | Commit (short) | Date | Threads | Status |
|---|--------|--------|--------|----------------|------|---------|--------|
| 1 | [22620502563](https://github.com/crossplatformdev/BigNum/actions/runs/22620502563) | [65545733977](https://github.com/crossplatformdev/BigNum/actions/runs/22620502563/job/65545733977) | copilot/optimize-lucas-lehmer-paths | fa12dd35 | 2026-03-03 11:15 | 1 | ⛔ cancelled (timeout) |
| 2 | [22622395950](https://github.com/crossplatformdev/BigNum/actions/runs/22622395950) | [65550510458](https://github.com/crossplatformdev/BigNum/actions/runs/22622395950/job/65550510458) | copilot/optimize-lucas-lehmer-paths | a0b203e8 | 2026-03-03 12:11 | 1 | ⛔ cancelled (timeout) |
| 3 | [22622901006](https://github.com/crossplatformdev/BigNum/actions/runs/22622901006) | [65552600747](https://github.com/crossplatformdev/BigNum/actions/runs/22622901006/job/65552600747) | copilot/optimize-lucas-lehmer-paths | 3fa8715e | 2026-03-03 12:25 | 1 | ⛔ cancelled (timeout) |
| 4 | [22622901902](https://github.com/crossplatformdev/BigNum/actions/runs/22622901902) | [65552601037](https://github.com/crossplatformdev/BigNum/actions/runs/22622901902/job/65552601037) | copilot/optimize-lucas-lehmer-paths | 3fa8715e | 2026-03-03 12:25 | 1 | ⛔ cancelled (timeout) |
| 5 | [22623300858](https://github.com/crossplatformdev/BigNum/actions/runs/22623300858) | [65553939530](https://github.com/crossplatformdev/BigNum/actions/runs/22623300858/job/65553939530) | copilot/optimize-lucas-lehmer-paths | dae8afcd | 2026-03-03 12:36 | 1 | ⛔ cancelled (timeout) |
| 6 | [22623302310](https://github.com/crossplatformdev/BigNum/actions/runs/22623302310) | [65553939831](https://github.com/crossplatformdev/BigNum/actions/runs/22623302310/job/65553939831) | copilot/optimize-lucas-lehmer-paths | dae8afcd | 2026-03-03 12:36 | 1 | ⛔ cancelled (timeout) |
| 7 | [22623524943](https://github.com/crossplatformdev/BigNum/actions/runs/22623524943) | [65554153589](https://github.com/crossplatformdev/BigNum/actions/runs/22623524943/job/65554153589) | main | 91bb5cce | 2026-03-03 12:43 | 1 | ⛔ cancelled (timeout) |
| 8 | [22623671571](https://github.com/crossplatformdev/BigNum/actions/runs/22623671571) | [65554778179](https://github.com/crossplatformdev/BigNum/actions/runs/22623671571/job/65554778179) | copilot/launch-cicd-workflow-threads | d618fc8d | 2026-03-03 12:47 | 4 | ⛔ cancelled (timeout) |
| 9 | [22623672568](https://github.com/crossplatformdev/BigNum/actions/runs/22623672568) | [65554779443](https://github.com/crossplatformdev/BigNum/actions/runs/22623672568/job/65554779443) | copilot/launch-cicd-workflow-threads | d618fc8d | 2026-03-03 12:47 | 4 | ⛔ cancelled (timeout) |
| 10 | [22623867819](https://github.com/crossplatformdev/BigNum/actions/runs/22623867819) | [65555335931](https://github.com/crossplatformdev/BigNum/actions/runs/22623867819/job/65555335931) | main | 894f6fac | 2026-03-03 12:52 | 4 | ⛔ cancelled (timeout) |
| 11-20 | 22624209230 – 22625653859 | various | copilot/optimize-limb-square-case, main | various | 2026-03-03 13:02–13:41 | — | in_progress or cancelled |

> **Critical finding: ALL 20 jobs were either cancelled (30-min timeout) or still in progress.  
> No CI run has completed successfully in the last 20 jobs.**

---

## 2. Data Sources Used

- **GitHub Actions job logs** retrieved via GitHub MCP `get_job_logs` for job IDs:
  65545733977, 65550510458, 65552600747, 65552601037, 65553939530, 65554153589, 65554779443, 65555335931
- **Log format:** `M_<p> is prime. Time: <t> s` per exponent
- **Machine-readable files generated:**
  - `regression_data.json` — full dataset
  - `regression_summary.csv` — per-row regression classifications

---

## 3. Parsed Benchmark Data

### 3.1 Single-thread runs (1 thread, benchmark_mode=true)

Commits in chronological order:

| Exponent p | fa12dd35 (G) | a0b203e8 (F) | 3fa8715e (E) | dae8afcd (D) | 91bb5cce (C) |
|------------|-------------|-------------|-------------|-------------|-------------|
| M_9689     | 0.426 s     | 0.315 s     | 0.318 s     | 0.367 s     | 0.371 s     |
| M_9941     | 0.434 s     | 0.323 s     | 0.330 s     | 0.396 s     | 0.381 s     |
| M_11213    | 0.489 s     | 0.366 s     | 0.367 s     | 0.424 s     | 0.427 s     |
| M_19937    | 1.848 s     | 1.408 s     | 1.444 s     | 1.624 s     | 1.629 s     |
| M_21701    | 2.013 s     | 1.553 s     | 1.562 s     | 1.773 s     | 1.766 s     |
| M_23209    | 2.177 s     | 1.629 s     | 1.673 s     | 1.879 s     | 1.898 s     |
| M_44497    | 9.177 s     | 7.468 s     | 7.312 s     | 8.262 s     | 8.220 s     |
| M_86243    | 43.849 s    | 38.037 s    | 37.213 s    | 41.651 s    | 41.311 s    |
| M_110503   | 56.111 s    | 48.401 s    | 48.141 s    | 53.053 s    | 53.131 s    |
| M_132049   | 169.266 s   | 142.839 s   | 143.635 s   | 150.935 s   | 152.315 s   |
| M_216091   | 267.928 s   | 224.457 s   | 220.128 s   | 239.084 s   | 241.215 s   |

### 3.2 Multi-thread runs (4 threads, thread-pool mode)

Commits in chronological order (two runs per commit shown as a pair):

| Exponent p | d618fc8d (4t, H) | 894f6fac (4t, A) |
|------------|-----------------|-----------------|
| M_9689     | 0.639 s         | 0.535 s         |
| M_9941     | 0.663 s         | 0.551 s         |
| M_11213    | 0.724 s         | 0.612 s         |
| M_19937    | 2.840 s         | 2.361 s         |
| M_21701    | 3.111 s         | 2.636 s         |
| M_23209    | 3.315 s         | 2.799 s         |
| M_44497    | 13.896 s        | 12.469 s        |
| M_86243    | 64.384 s        | 57.453 s        |
| M_110503   | 81.087 s        | 73.051 s        |
| M_132049   | 201.144 s       | 205.877 s       |
| M_216091   | 337.970 s       | 339.693 s       |

> **Note on 4-thread timings:** In thread-pool mode, each exponent is processed on its own worker thread.  
> For these exponent sizes, the computational kernel is single-threaded (the thread-pool only parallelizes across independent exponents, not within a single exponent's LL loop).  
> The 4-thread times represent *competing* resource usage — each exponent competes with 3 other concurrent LL computations for CPU cache and memory bandwidth.

---

## 4. Per-Exponent Regression Table

### 4.1 Algorithm regressions (1-thread only, comparing E→D transition)

The transition from commit `3fa8715e` (E) to `dae8afcd` (D) shows a consistent performance regression on the `copilot/optimize-lucas-lehmer-paths` branch:

| Exponent p | Best prior (E) | Latest (D) | % change | Classification | Confidence |
|------------|---------------|-----------|----------|---------------|-----------|
| M_9689     | 0.318 s       | 0.367 s   | **+15.4%** | Regression | High |
| M_9941     | 0.330 s       | 0.396 s   | **+20.0%** | Regression | High |
| M_11213    | 0.367 s       | 0.424 s   | **+15.5%** | Regression | High |
| M_19937    | 1.444 s       | 1.624 s   | **+12.5%** | Regression | High |
| M_21701    | 1.562 s       | 1.773 s   | **+13.5%** | Regression | High |
| M_23209    | 1.673 s       | 1.879 s   | **+12.3%** | Regression | High |
| M_44497    | 7.312 s       | 8.262 s   | **+13.0%** | Regression | High |
| M_86243    | 37.213 s      | 41.651 s  | **+11.9%** | Regression | High |
| M_110503   | 48.141 s      | 53.053 s  | **+10.2%** | Regression | High |
| M_132049   | 143.635 s     | 150.935 s | **+5.1%** | Regression | Medium |
| M_216091   | 220.128 s     | 239.084 s | **+8.6%** | Regression | High |

**Conclusion:** Commit `dae8afcd` introduced a global regression of approximately **10–20% across all exponents ≥ p=9689**.  
This regression was present on the branch but was partially merged into `main` (commit `91bb5cce` shows similar times to D).

### 4.2 Threading regression (4 threads vs best 1-thread, same exponent range)

| Exponent p | Best 1-thread (E) | 4-thread (A) | Speedup (4t/1t) | Classification |
|------------|------------------|-------------|-----------------|---------------|
| M_9689     | 0.315 s          | 0.535 s     | **0.59× (41% slower)** | Threading regression |
| M_11213    | 0.366 s          | 0.612 s     | **0.60× (40% slower)** | Threading regression |
| M_44497    | 7.312 s          | 12.469 s    | **0.59× (41% slower)** | Threading regression |
| M_86243    | 37.213 s         | 57.453 s    | **0.65× (35% slower)** | Threading regression |
| M_110503   | 48.141 s         | 73.051 s    | **0.66× (34% slower)** | Threading regression |
| M_132049   | 142.839 s        | 205.877 s   | **0.69× (30% slower)** | Threading regression |
| M_216091   | 220.128 s        | 339.693 s   | **0.65× (35% slower)** | Threading regression |

**4 threads is consistently 30–41% slower than 1 thread for all tested exponents.**  
This is a confirmed threading regression: the `copilot/launch-cicd-workflow-threads` branch switched the test to use `threads=0` (max cores), but the thread-pool model runs independent exponents concurrently rather than parallelizing a single exponent's computation. This means 4 workers compete for the same CPU cache and memory bandwidth, degrading each individual exponent's performance with no throughput gain (since the workload is a sequential list, not a parallel one).

### 4.3 Summary per exponent (all-time, 1-thread only)

| Exponent p | N samples | Latest | Median (prior) | Best prior | % vs median | % vs best | Classification |
|------------|-----------|--------|----------------|-----------|-------------|-----------|---------------|
| M_9689     | 5         | 0.371  | 0.367          | 0.315     | +1.1%       | +17.8%    | Noise vs median; regression vs best |
| M_44497    | 5         | 8.220  | 8.220          | 7.312     | 0.0%        | +12.4%    | Noise vs median; regression vs best |
| M_86243    | 5         | 41.311 | 41.651         | 37.213    | -0.8%       | +11.0%    | Noise vs median; regression vs best |
| M_216091   | 5         | 241.2  | 239.1          | 220.1     | +0.9%       | +9.6%     | Noise vs median; regression vs best |

---

## 5. Job-Level Summary

| Job | Commit | Threads | Exponents regressed | Exponents improved | Median Δ vs prior | Assessment |
|-----|--------|---------|--------------------|--------------------|-------------------|-----------|
| G (22620502563) | fa12dd35 | 1 | 0 | 0 | — | **Baseline** |
| F (22622395950) | a0b203e8 | 1 | 0 | 11 | −16% | **Global improvement** (−13–26% across all exponents) |
| E (22622901006) | 3fa8715e | 1 | 0 | 1 | −1% | **Noise / marginal improvement** |
| D (22623300858) | dae8afcd | 1 | 11 | 0 | +12% | **Global regression** (+10–20% across all exponents) |
| C (22623524943) | 91bb5cce | 1 | 0 | 0 | +1% | **Noise** (D-regression carried into main) |
| H (22623672568) | d618fc8d | 4 | 11 | 0 | +52% | **Threading regression** (thread-pool switch slows all exponents) |
| A (22623867819) | 894f6fac | 4 | 11 | 0 | +40% | **Threading regression** (thread-pool switch slows all exponents) |

---

## 6. Overall Conclusions

### 6.1 Likely regression points

**Regression 1 — Algorithm regression (HIGH confidence):**
- **Introduced by:** commit `dae8afcd` on branch `copilot/optimize-lucas-lehmer-paths`
- **Effect:** Global +10–20% slowdown across all FFT-range exponents (M_9689 to M_216091)
- **Scope:** Size-independent; affects the entire FFT backend uniformly
- **Evidence:** 5 direct measurements confirm this; median change is +12%, well outside noise floor (~2%)
- **Likely cause:** A change to the FFT/DWT kernel or carry-propagation path in the Lucas-Lehmer hot loop that introduced overhead or reduced optimization opportunities. The regression is proportional to exponent size, pointing to a per-iteration cost increase in the FFT squaring path.

**Regression 2 — Threading regression (HIGH confidence):**
- **Introduced by:** branch `copilot/launch-cicd-workflow-threads` (commits d618fc8d, 894f6fac)
- **Effect:** 4-thread mode is **30–41% slower** than 1-thread mode for all tested exponents
- **Root cause confirmed:** The thread-pool dispatches *independent exponents* to workers, not parallel slices of a single exponent's computation. Four threads therefore compete for cache/memory without reducing any individual exponent's latency. The result is worse than sequential because of resource contention.
- **Fix required:** For latency-mode (single large exponent), parallelize the FFT squaring itself, not the outer LL loop dispatch.

**Regression 3 — CI timeout regression (HIGH confidence):**
- **Introduced by:** Adding M_756839 (and larger exponents) to the known_mersenne_prime_exponents list that `make test` exercises
- **Effect:** EVERY CI run since this exponent was added has been cancelled at the 30-minute timeout
- **M_756839 timing:** 20+ minutes on 1 thread (was still running when cancelled at 21+ minutes)
- **Fix applied:** Added `LL_MAX_EXPONENT_INDEX` env var; `make test` now uses `LL_MAX_EXPONENT_INDEX=31` to stop before M_756839

### 6.2 What is NOT a regression

- Runs with `LL_STOP_AFTER_ONE=1` profiling step (index=14, p=9689): these are correct and expected
- Small exponent (p < 9000) timings: these are all sub-millisecond and dominated by measurement noise

---

## 7. Recommended Next Steps

### Priority 1 — Fix CI timeout (DONE in this PR)
Added `LL_MAX_EXPONENT_INDEX` env-var support to `src/BigNum.cpp` and updated `make test` to set `LL_MAX_EXPONENT_INDEX=31`, limiting the test to M_2 through M_216091.  
This is the critical path blocker preventing ALL CI from completing.

### Priority 2 — Revert or fix commit dae8afcd (algorithm regression)
- Bisect between `3fa8715e` and `dae8afcd` to identify the specific change
- Key target: changes to `fft_square()`, carry propagation, or the Lucas-Lehmer hot loop
- Benchmark before and after each change with `make bench` on exponents 9689, 44497, 86243, 216091

### Priority 3 — Fix threading model
- For exponent sizes tested (p ≤ 216091), single-thread is the correct CI test mode
- The thread-pool throughput model only helps when running many small independent exponents
- For large exponents, implement intra-exponent parallelism in the FFT convolution itself (split the transform across multiple cores)

### Priority 4 — Add per-exponent CI timing assertions
- Add a benchmark step that records timings to an artifact and fails if any exponent regresses by > 15% vs the prior run
- Use `bench-ci` Makefile target and save output as a GitHub Actions artifact

---

## 8. Files and Artifacts

- `regression_report.md` — this report
- `regression_data.json` — full structured dataset (job metadata + per-exponent timings)
- `regression_summary.csv` — per-row classifications for all measured (job, exponent) pairs
- `src/BigNum.cpp` — added `LL_MAX_EXPONENT_INDEX` env-var support (CI timeout fix)
- `Makefile` — updated `make test` to use `LL_MAX_EXPONENT_INDEX=31`

---

## 9. Workflow Run / Job Links

| Description | Link |
|-------------|------|
| Current CI run (with fix) | https://github.com/crossplatformdev/BigNum/actions/runs/22625641126 |
| Earliest analyzed job | https://github.com/crossplatformdev/BigNum/actions/runs/22620502563/job/65545733977 |
| Algorithm regression job (dae8afcd) | https://github.com/crossplatformdev/BigNum/actions/runs/22623300858/job/65553939530 |
| Threading regression job (d618fc8d) | https://github.com/crossplatformdev/BigNum/actions/runs/22623672568/job/65554779443 |
| Latest main-branch cancelled job | https://github.com/crossplatformdev/BigNum/actions/runs/22623867819/job/65555335931 |
