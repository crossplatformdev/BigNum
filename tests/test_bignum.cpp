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
    assert(mersenne::is_known_mersenne_prime(2));
    assert(mersenne::is_known_mersenne_prime(3));
    assert(mersenne::is_known_mersenne_prime(136279841));
    // Primes not in the list must return false.
    assert(!mersenne::is_known_mersenne_prime(11));      // M_11 is composite
    assert(!mersenne::is_known_mersenne_prime(136279843)); // > last known, not in list
    assert(!mersenne::is_known_mersenne_prime(0));
    assert(!mersenne::is_known_mersenne_prime(1));
}

static void test_generate_post_known_exponents() {
    // Use small range for speed: primes in (10, 30] are 11, 13, 17, 19, 23, 29.
    const auto v = mersenne::generate_post_known_exponents(10u, 30u);
    const std::vector<uint32_t> expected = {11, 13, 17, 19, 23, 29};
    assert(v == expected);

    // Empty range.
    assert(mersenne::generate_post_known_exponents(30u, 30u).empty());
    assert(mersenne::generate_post_known_exponents(30u, 10u).empty());

    // Single element: primes in (28, 30] = {29}.
    const auto w = mersenne::generate_post_known_exponents(28u, 30u);
    assert(w.size() == 1u && w[0] == 29u);
}

static void test_discover_exponent_list_ordering() {
    // single_exp=13 first, then range (10,30] minus 13.
    // Range primes in (10,30]: 11,13,17,19,23,29  → minus 13 → 11,17,19,23,29
    const auto v = mersenne::discover_exponent_list(13u, 10u, 30u);
    assert(!v.empty() && v[0] == 13u);
    // 13 must appear exactly once.
    assert(std::count(v.begin(), v.end(), 13u) == 1u);
    // Range part (all but first) must be ascending.
    for (size_t i = 2; i < v.size(); ++i)
        assert(v[i] > v[i - 1]);
}

static void test_discover_exponent_list_dedup() {
    // single_exp falls in the range: must appear only once and be first.
    const auto v = mersenne::discover_exponent_list(17u, 10u, 30u);
    assert(std::count(v.begin(), v.end(), 17u) == 1u);
    assert(v[0] == 17u);
}

static void test_discover_exponent_list_range_limiting() {
    // max_incl = 20: range primes in (10,20] = 11,13,17,19.
    const auto v = mersenne::discover_exponent_list(0u, 10u, 20u);
    for (uint32_t p : v) {
        assert(p > 10u && p <= 20u);
        assert(mersenne::is_prime_exponent(p));
    }
    // Ensure 23 and 29 are absent.
    assert(std::find(v.begin(), v.end(), 23u) == v.end());
    assert(std::find(v.begin(), v.end(), 29u) == v.end());
}

static void test_discover_exponent_list_sharding() {
    // Sharding partitions the range part (not single_exp) deterministically.
    // Range primes in (10,30]: 11,13,17,19,23,29 (6 items)
    std::vector<uint32_t> all_range_items;
    for (uint32_t si = 0; si < 3u; ++si) {
        const auto s = mersenne::discover_exponent_list(0u, 10u, 30u,
                                                        /*reverse=*/false,
                                                        /*shard_count=*/3u,
                                                        /*shard_index=*/si);
        all_range_items.insert(all_range_items.end(), s.begin(), s.end());
    }
    // Total items across all shards == full range size.
    const auto full = mersenne::generate_post_known_exponents(10u, 30u);
    assert(all_range_items.size() == full.size());
    // Every item appears exactly once.
    std::sort(all_range_items.begin(), all_range_items.end());
    assert(all_range_items == full);
}

static void test_discover_exponent_list_reverse_order() {
    // Range part must be in descending order when reverse_order=true.
    const auto v = mersenne::discover_exponent_list(0u, 10u, 30u,
                                                    /*reverse=*/true,
                                                    /*shard_count=*/1u,
                                                    /*shard_index=*/0u);
    // v should be descending.
    for (size_t i = 1; i < v.size(); ++i)
        assert(v[i] < v[i - 1]);
}

static void test_discover_exponent_list_non_prime_single_exp() {
    // single_exp that is not prime must be excluded from the list.
    const auto v = mersenne::discover_exponent_list(15u, 10u, 30u); // 15 = 3*5
    assert(std::find(v.begin(), v.end(), 15u) == v.end());
}

static void test_discover_exponent_list_post_known_defaults() {
    // Verify that a small range above 136279841 yields only primes > 136279841
    // and <= max_incl.
    const uint32_t min_excl = 136279841u;
    const uint32_t max_incl = 136279950u;
    const auto v = mersenne::discover_exponent_list(0u, min_excl, max_incl);
    for (uint32_t p : v) {
        assert(p > min_excl);
        assert(p <= max_incl);
        assert(mersenne::is_prime_exponent(p));
    }
    // None of the candidates should be in the known list (they're all > last known).
    for (uint32_t p : v)
        assert(!mersenne::is_known_mersenne_prime(p));
}

static void test_discovery_classification() {
    // Known exponent: is_known_mersenne_prime → true, treated as known.
    assert(mersenne::is_known_mersenne_prime(136279841u));
    // Exponent above the known list: not in list → new-discovery candidate.
    // (136279879 is prime and > 136279841, so it's not in the known list.)
    assert(mersenne::is_prime_exponent(136279879u));
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
