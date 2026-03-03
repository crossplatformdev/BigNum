#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

#define BIGNUM_NO_MAIN
#include "../src/BigNum.cpp"

// Verify that precharge_work_matrix distributes all items across threads.
static void test_precharge_distribution() {
    const std::vector<uint32_t> exponents = {2, 3, 5, 7, 13, 17, 19, 31};
    const unsigned threads = 3u;
    const size_t startIndex = 1u; // skip first element

    const auto work_matrix = mersenne::precharge_work_matrix(exponents, startIndex, threads);

    // Total items distributed must equal exponents.size() - startIndex
    size_t total = 0;
    for (const auto& lane : work_matrix) total += lane.size();
    assert(total == exponents.size() - startIndex);

    // Every item from startIndex onwards must appear exactly once
    std::vector<uint32_t> collected;
    collected.reserve(total);
    for (const auto& lane : work_matrix)
        collected.insert(collected.end(), lane.begin(), lane.end());
    std::sort(collected.begin(), collected.end());
    const std::vector<uint32_t> expected(exponents.begin() + startIndex, exponents.end());
    assert(collected == expected);
}

// ---- discover-mode tests ----

static void test_is_known_mersenne_prime() {
    // Known entries must return true.
    assert(mersenne::is_known_mersenne_prime(2u));
    assert(mersenne::is_known_mersenne_prime(3u));
    assert(mersenne::is_known_mersenne_prime(136279841u));
    // Primes not in the list must return false.
    assert(!mersenne::is_known_mersenne_prime(11u));       // M_11 is composite
    assert(!mersenne::is_known_mersenne_prime(136279843u)); // > last known, not in list
    assert(!mersenne::is_known_mersenne_prime(0u));
    assert(!mersenne::is_known_mersenne_prime(1u));
    // Values above uint32 max can never be known.
    assert(!mersenne::is_known_mersenne_prime(UINT64_C(5000000000)));
}

static void test_generate_post_known_exponents() {
    // Use small range for speed: primes in (10, 30] are 11, 13, 17, 19, 23, 29.
    const auto v = mersenne::generate_post_known_exponents(UINT64_C(10), UINT64_C(30));
    const std::vector<uint64_t> expected = {11, 13, 17, 19, 23, 29};
    assert(v == expected);

    // Empty range.
    assert(mersenne::generate_post_known_exponents(UINT64_C(30), UINT64_C(30)).empty());
    assert(mersenne::generate_post_known_exponents(UINT64_C(30), UINT64_C(10)).empty());

    // Single element: primes in (28, 30] = {29}.
    const auto w = mersenne::generate_post_known_exponents(UINT64_C(28), UINT64_C(30));
    assert(w.size() == 1u && w[0] == 29u);
}

static void test_generate_post_known_exponents_u64_range() {
    // Verify generation works for values well above uint32 max.
    // Primes in (4294967311, 4294967331]: 4294967357 is outside, let's find actual primes.
    // We use a small range just above UINT32_MAX: (UINT32_MAX, UINT32_MAX+100].
    const uint64_t base = static_cast<uint64_t>(UINT32_MAX);
    const auto v = mersenne::generate_post_known_exponents(base, base + 100u);
    // All returned values must be > UINT32_MAX and prime.
    for (uint64_t p : v) {
        assert(p > base);
        assert(p <= base + 100u);
        assert(mersenne::is_prime_exponent(p));
    }
    // is_prime_exponent itself must handle uint64_t inputs.
    assert(mersenne::is_prime_exponent(UINT64_C(4294967311)));  // known prime > UINT32_MAX
    assert(!mersenne::is_prime_exponent(UINT64_C(4294967312))); // even
}

static void test_discover_exponent_list_ordering() {
    // single_exp=13 first, then range (10,30] minus 13.
    // Range primes in (10,30]: 11,13,17,19,23,29  → minus 13 → 11,17,19,23,29
    const auto v = mersenne::discover_exponent_list(UINT64_C(13), UINT64_C(10), UINT64_C(30));
    assert(!v.empty() && v[0] == 13u);
    // 13 must appear exactly once.
    assert(std::count(v.begin(), v.end(), UINT64_C(13)) == 1u);
    // Range part (all but first) must be ascending.
    for (size_t i = 2; i < v.size(); ++i)
        assert(v[i] > v[i - 1]);
}

static void test_discover_exponent_list_dedup() {
    // single_exp falls in the range: must appear only once and be first.
    const auto v = mersenne::discover_exponent_list(UINT64_C(17), UINT64_C(10), UINT64_C(30));
    assert(std::count(v.begin(), v.end(), UINT64_C(17)) == 1u);
    assert(v[0] == 17u);
}

static void test_discover_exponent_list_range_limiting() {
    // max_incl = 20: range primes in (10,20] = 11,13,17,19.
    const auto v = mersenne::discover_exponent_list(UINT64_C(0), UINT64_C(10), UINT64_C(20));
    for (uint64_t p : v) {
        assert(p > 10u && p <= 20u);
        assert(mersenne::is_prime_exponent(p));
    }
    // Ensure 23 and 29 are absent.
    assert(std::find(v.begin(), v.end(), UINT64_C(23)) == v.end());
    assert(std::find(v.begin(), v.end(), UINT64_C(29)) == v.end());
}

static void test_discover_exponent_list_sharding() {
    // Sharding partitions the range part (not single_exp) deterministically.
    // Range primes in (10,30]: 11,13,17,19,23,29 (6 items)
    std::vector<uint64_t> all_range_items;
    for (uint32_t si = 0; si < 3u; ++si) {
        const auto s = mersenne::discover_exponent_list(UINT64_C(0), UINT64_C(10), UINT64_C(30),
                                                        /*reverse=*/false,
                                                        /*shard_count=*/3u,
                                                        /*shard_index=*/si);
        all_range_items.insert(all_range_items.end(), s.begin(), s.end());
    }
    // Total items across all shards == full range size.
    const auto full = mersenne::generate_post_known_exponents(UINT64_C(10), UINT64_C(30));
    assert(all_range_items.size() == full.size());
    // Every item appears exactly once.
    std::sort(all_range_items.begin(), all_range_items.end());
    assert(all_range_items == full);
}

static void test_discover_exponent_list_reverse_order() {
    // Range part must be in descending order when reverse_order=true.
    const auto v = mersenne::discover_exponent_list(UINT64_C(0), UINT64_C(10), UINT64_C(30),
                                                    /*reverse=*/true,
                                                    /*shard_count=*/1u,
                                                    /*shard_index=*/0u);
    // v should be descending.
    for (size_t i = 1; i < v.size(); ++i)
        assert(v[i] < v[i - 1]);
}

static void test_discover_exponent_list_non_prime_single_exp() {
    // single_exp that is not prime must be excluded from the list.
    const auto v = mersenne::discover_exponent_list(UINT64_C(15), UINT64_C(10), UINT64_C(30)); // 15 = 3*5
    assert(std::find(v.begin(), v.end(), UINT64_C(15)) == v.end());
}

static void test_discover_exponent_list_post_known_defaults() {
    // Verify that a small range above 136279841 yields only primes > 136279841
    // and <= max_incl.
    const uint64_t min_excl = 136279841u;
    const uint64_t max_incl = 136279950u;
    const auto v = mersenne::discover_exponent_list(UINT64_C(0), min_excl, max_incl);
    for (uint64_t p : v) {
        assert(p > min_excl);
        assert(p <= max_incl);
        assert(mersenne::is_prime_exponent(p));
    }
    // None of the candidates should be in the known list (they're all > last known).
    for (uint64_t p : v)
        assert(!mersenne::is_known_mersenne_prime(p));
}

static void test_discovery_classification() {
    // Known exponent: is_known_mersenne_prime → true, treated as known.
    assert(mersenne::is_known_mersenne_prime(136279841u));
    // Exponent above the known list: not in list → new-discovery candidate.
    // (136279879 is prime and > 136279841, so it's not in the known list.)
    assert(mersenne::is_prime_exponent(UINT64_C(136279879)));
    assert(!mersenne::is_known_mersenne_prime(136279879u));
}

int main() {
    using namespace mersenne;

    // --- is_prime_exponent ---
    assert(is_prime_exponent(2));
    assert(is_prime_exponent(3));
    assert(!is_prime_exponent(1));
    assert(!is_prime_exponent(9));

    // --- lucas_lehmer: small cases via GenericBackend (p < 128) ---
    assert(lucas_lehmer(2, false));
    assert(lucas_lehmer(3, false));
    assert(lucas_lehmer(5, false));
    assert(lucas_lehmer(11, false) == false);  // M_11 is composite

    // --- lucas_lehmer: medium cases via FftMersenneBackend ---
    assert(lucas_lehmer(521,  false, /*benchmark_mode=*/true));
    assert(lucas_lehmer(607,  false, /*benchmark_mode=*/true));
    assert(lucas_lehmer(1279, false, /*benchmark_mode=*/true));

    // --- lucas_lehmer: large case via FftMersenneBackend (p >= kLimbFftCrossover = 4000) ---
    // Ensures the updated FFT carry path is exercised in CI.
    assert(lucas_lehmer(4253, false, /*benchmark_mode=*/true));

    // --- lucas_lehmer: fast rejection for composite exponent p >= 128 ---
    assert(!lucas_lehmer(129, false));

    // --- runtime ---
    const unsigned cores = runtime::detect_available_cores();
    assert(cores >= 1u);

    // --- backend::choose_fft_length sanity ---
    // For p=9689, the minimum safe FFT length must be >= ceil(9689 / b_hi_max).
    {
        const size_t n = backend::choose_fft_length(9689);
        assert(n >= 1024u && (n & (n - 1)) == 0u);  // power-of-two, ≥ 1024
    }
    test_precharge_distribution();

    // --- discover-mode tests ---
    test_is_known_mersenne_prime();
    test_generate_post_known_exponents();
    test_generate_post_known_exponents_u64_range();
    test_discover_exponent_list_ordering();
    test_discover_exponent_list_dedup();
    test_discover_exponent_list_range_limiting();
    test_discover_exponent_list_sharding();
    test_discover_exponent_list_reverse_order();
    test_discover_exponent_list_non_prime_single_exp();
    test_discover_exponent_list_post_known_defaults();
    test_discovery_classification();

    std::cout << "All tests passed\n";
    return 0;
}
