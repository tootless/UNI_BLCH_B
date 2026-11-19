#include "blockchain.hpp"
#include "miner.hpp"
#include "transaction.hpp"
#include "transaction_validate.hpp"
#include "user.hpp"
#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <random>

inline std::vector<Transaction> generate_transactions_valid(
    const std::vector<User>& users,
    std::mt19937& rng,
    std::size_t count = 10000)
{
    std::vector<Transaction> transactions;
    transactions.reserve(count);

    std::uniform_int_distribution<std::size_t> user_dist(0, users.size() - 1);
    std::uniform_int_distribution<std::uint64_t> amount_dist(1, 10000);

    std::unordered_map<std::uint32_t, std::uint64_t> sequences;
    for (const User& u : users)
        sequences[u.get_id()] = 1;

    for (std::size_t i = 0; i < count; ++i) {
        std::size_t si = user_dist(rng);
        std::size_t ri = user_dist(rng);
        while (si == ri) ri = user_dist(rng);

        const User& sender = users[si];
        const User& receiver = users[ri];

        std::uint64_t amount = amount_dist(rng);
        std::uint64_t sequence = sequences.at(sender.get_id())++;

        transactions.emplace_back(
            sender.get_id(), receiver.get_id(), amount, sequence);
    }

    return transactions;
}

// Vienos gijos kasimo funkcija – naudojama ir nuosekliai, ir lygiagreciai.
// found = bendras atomic sustabdymo signalas; nullptr reiškia "nera signalo"
// (nuoseklusis kelias).
static void mine_worker(
    MiningCandidate& candidate,
    std::uint64_t batch_size,
    std::atomic<bool>* found)   // nullptr → nuoseklusis kelias, be sustabdymo
{
    if (candidate.solved || candidate.exhausted)
        return;

    const auto start = std::chrono::steady_clock::now();

    for (std::uint64_t i = 0; i < batch_size; ++i) {
        // Tikrina sustabdymo signala tik lygiagreciame kelyje
        if (found && found->load(std::memory_order_relaxed))
            break;

        candidate.block.set_nonce(candidate.next_nonce);
        ++candidate.attempts;

        if (candidate.block.is_valid_pow()) {
            candidate.solved = true;
            if (found) {
                bool expected = false;
                found->compare_exchange_strong(
                    expected, true,
                    std::memory_order_release,
                    std::memory_order_relaxed);
            }
            break;
        }

        if (candidate.next_nonce == std::numeric_limits<std::uint64_t>::max()) {
            candidate.exhausted = true;
            break;
        }

        ++candidate.next_nonce;
    }

    const auto end = std::chrono::steady_clock::now();
    candidate.mining_seconds +=
        std::chrono::duration<double>(end - start).count();
}

// Lygiagretus raundas: kiekvienas kandidatas – atskira gija.
// Grazina tikra praejusi laika (sienos laikas, ne suma).
static double mine_candidates_round_parallel(
    std::vector<MiningCandidate>& candidates,
    std::uint64_t batch_size)
{
    std::atomic<bool> found{ false };

    // Inicijuojame found, jei kas nors jau išsprendė
    for (const MiningCandidate& c : candidates)
        if (c.solved) { found.store(true); break; }

    const auto wall_start = std::chrono::steady_clock::now();

    std::vector<std::thread> threads;
    threads.reserve(candidates.size());

    for (MiningCandidate& c : candidates)
        threads.emplace_back(mine_worker, std::ref(c), batch_size, &found);

    for (std::thread& t : threads)
        t.join();

    const auto wall_end = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(wall_end - wall_start).count();
}

// ---------------------------------------------------------------------------
// Bendras kasimo ciklas – naudojamas abiem variantams
// Gražina sienos laiką (wall time).
// ---------------------------------------------------------------------------
struct RaceResult {
    bool         found = false;
    std::size_t  winner_idx = std::numeric_limits<std::size_t>::max();
    std::uint64_t total_attempts = 0;
    double       wall_seconds = 0.0;   // realus laikas (ne suma per gijas)
    std::uint64_t rounds = 0;
};

static RaceResult run_race(
    std::vector<MiningCandidate>& candidates,
    Miner& miner,
    bool parallel,
    std::uint64_t initial_budget = 10'000,
    std::uint64_t budget_step = 20'000,
    std::uint64_t attempt_limit = 500'000)
{
    RaceResult res;

    auto count_attempts = [&]() {
        std::uint64_t s = 0;
        for (const MiningCandidate& c : candidates) s += c.attempts;
        return s;
        };

    // Pradinis raundas
    ++res.rounds;
    if (parallel)
        res.wall_seconds += mine_candidates_round_parallel(candidates, initial_budget);
    else {
        const auto t0 = std::chrono::steady_clock::now();
        miner.mine_candidates_round(candidates, initial_budget);
        res.wall_seconds +=
            std::chrono::duration<double>(
                std::chrono::steady_clock::now() - t0).count();
    }

    while (true) {
        res.total_attempts = count_attempts();
        res.winner_idx = miner.select_winner(candidates);

        if (res.winner_idx != std::numeric_limits<std::size_t>::max()) {
            res.found = true;
            break;
        }

        if (res.total_attempts >= attempt_limit)
            break;

        const std::uint64_t remaining = attempt_limit - res.total_attempts;
        const std::uint64_t next_budget =
            std::min(budget_step, remaining / static_cast<std::uint64_t>(candidates.size()));

        if (next_budget == 0)
            break;

        ++res.rounds;
        if (parallel)
            res.wall_seconds +=
            mine_candidates_round_parallel(candidates, next_budget);
        else {
            const auto t0 = std::chrono::steady_clock::now();
            miner.mine_candidates_round(candidates, next_budget);
            res.wall_seconds +=
                std::chrono::duration<double>(
                    std::chrono::steady_clock::now() - t0).count();
        }
    }

    res.total_attempts = count_attempts();
    return res;
}

// ---------------------------------------------------------------------------
// Pagalbine: sukuria vienoda aplinka (naudotojai + pool)
// ---------------------------------------------------------------------------
struct TestEnv {
    std::vector<User>    users;
    BalanceState         balances;
    Blockchain           blockchain;
    Miner                miner;
    int                  pool_size = 0;

    TestEnv(std::uint64_t difficulty,
        std::size_t   user_count,
        std::size_t   tx_count,
        std::mt19937& rng)
        : blockchain(difficulty)
    {
        users = generate_users(rng, user_count);
        for (const User& u : users)
            balances[u.get_id()] = u.get_balance();

        const auto all_txs =
            generate_transactions_valid(users, rng, tx_count);

        for (const Transaction& tx : all_txs) {
            std::string reason;
            if (miner.add_transaction(tx, balances,
                blockchain.get_included_transaction_ids(),
                blockchain.get_included_sequences(),
                reason))
                ++pool_size;
        }
    }
};

// ---------------------------------------------------------------------------
// Pagrindiniai testai
// ---------------------------------------------------------------------------

// Testas 1: nuoseklus kasimas (esamas elgesys)
bool test_sequential(std::uint64_t difficulty,
    std::size_t   user_count,
    std::size_t   tx_count)
{
    std::cout << "\n--- Nuoseklus kasimas ---\n";

    std::mt19937 rng{ 3719 };
    TestEnv env(difficulty, user_count, tx_count, rng);

    std::cout << "Pool: " << env.pool_size << " tx\n";
    if (env.pool_size == 0) { std::cout << "PRALEISTA (pool tuščias)\n"; return false; }

    auto candidates = env.miner.create_five_candidates(
        env.balances,
        env.blockchain.get_latest_hash(),
        env.blockchain.get_included_transaction_ids(),
        env.blockchain.get_included_sequences(),
        difficulty);

    RaceResult res = run_race(candidates, env.miner, /*parallel=*/false);

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Rastas: " << (res.found ? "taip" : "ne") << "\n";
    std::cout << "Bandymai: " << res.total_attempts << "\n";
    std::cout << "Raundai:  " << res.rounds << "\n";
    std::cout << "Laikas:   " << res.wall_seconds << " s\n";

    if (!res.found) return false;

    const MiningCandidate& winner = candidates[res.winner_idx];
    BalanceState balances_copy = env.balances;

    try {
        env.blockchain.add_block(winner.block, balances_copy);
    }
    catch (const std::exception& e) {
        std::cout << "Blokas atmestas: " << e.what() << "\n";
        return false;
    }

    env.miner.commit_mined_transactions(winner.included_indices);
    std::cout << "Blokas priimtas. Pool liko: "
        << env.miner.get_transaction_pool().size() << "\n";
    return true;
}

// Testas 2: lygiagretus kasimas
bool test_parallel(std::uint64_t difficulty,
    std::size_t   user_count,
    std::size_t   tx_count)
{
    std::cout << "\n--- Lygiagretus kasimas ---\n";

    std::mt19937 rng{ 3719 };
    TestEnv env(difficulty, user_count, tx_count, rng);

    std::cout << "Pool: " << env.pool_size << " tx\n";
    if (env.pool_size == 0) { std::cout << "PRALEISTA (pool tuščias)\n"; return false; }

    auto candidates = env.miner.create_five_candidates(
        env.balances,
        env.blockchain.get_latest_hash(),
        env.blockchain.get_included_transaction_ids(),
        env.blockchain.get_included_sequences(),
        difficulty);

    RaceResult res = run_race(candidates, env.miner, /*parallel=*/true);

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Rastas: " << (res.found ? "taip" : "ne") << "\n";
    std::cout << "Bandymai: " << res.total_attempts << "\n";
    std::cout << "Raundai:  " << res.rounds << "\n";
    std::cout << "Laikas:   " << res.wall_seconds << " s  (sienos laikas)\n";

    // Patikrinti: nelaimejusių kandidatu attempts nekeicia balansų
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        if (i == res.winner_idx) continue;
        // Jei kandidatas solved bet nera laimėtojas – jo blokas nepateks į grandine, balansai nepakis.
        if (candidates[i].solved)
            std::cout << "  kandidatas " << i + 1
            << " taip pat išsprendė, bet nebus naudojamas\n";
    }

    if (!res.found) return false;

    const MiningCandidate& winner = candidates[res.winner_idx];
    BalanceState balances_copy = env.balances;

    try {
        env.blockchain.add_block(winner.block, balances_copy);
    }
    catch (const std::exception& e) {
        std::cout << "Blokas atmestas: " << e.what() << "\n";
        return false;
    }

    env.miner.commit_mined_transactions(winner.included_indices);
    std::cout << "Blokas priimtas. Pool liko: "
        << env.miner.get_transaction_pool().size() << "\n";
    return true;
}

// Testas 3: palyginimas vienodomis sąlygomis (tas pats seed, tie patys kandidatai)
void test_comparison(std::uint64_t difficulty,
    std::size_t   user_count,
    std::size_t   tx_count)
{
    std::cout << "\n=== Palyginimas: nuoseklus vs lygiagretus ===\n";
    std::cout << "Difficulty: " << difficulty
        << ", naudotojai: " << user_count
        << ", tx: " << tx_count << "\n";

    // --- Nuoseklus ---
    {
        std::mt19937 rng{ 3719 };
        TestEnv env(difficulty, user_count, tx_count, rng);

        auto candidates = env.miner.create_five_candidates(
            env.balances,
            env.blockchain.get_latest_hash(),
            env.blockchain.get_included_transaction_ids(),
            env.blockchain.get_included_sequences(),
            difficulty);

        RaceResult res = run_race(candidates, env.miner, false);

        std::cout << std::fixed << std::setprecision(6);
        std::cout << "[Nuoseklus]   laikas=" << res.wall_seconds
            << " s, bandymai=" << res.total_attempts
            << ", rastas=" << (res.found ? "taip" : "ne") << "\n";
    }

    // --- Lygiagretus ---
    {
        std::mt19937 rng{ 3719 };
        TestEnv env(difficulty, user_count, tx_count, rng);

        auto candidates = env.miner.create_five_candidates(
            env.balances,
            env.blockchain.get_latest_hash(),
            env.blockchain.get_included_transaction_ids(),
            env.blockchain.get_included_sequences(),
            difficulty);

        RaceResult res = run_race(candidates, env.miner, true);

        std::cout << "[Lygiagretus] laikas=" << res.wall_seconds
            << " s (sienos laikas), bandymai=" << res.total_attempts
            << ", rastas=" << (res.found ? "taip" : "ne") << "\n";
    }
}
int main()
{
    constexpr std::uint64_t DIFFICULTY = 3;
    constexpr std::size_t   USER_COUNT = 20;
    constexpr std::size_t   TX_COUNT = 100;

    std::cout << "Lygiagretus kasimas v0.2\n";
    std::cout << "Thread count: "
        << std::thread::hardware_concurrency() << "\n";

    const bool seq_ok = test_sequential(DIFFICULTY, USER_COUNT, TX_COUNT);
    const bool par_ok = test_parallel(DIFFICULTY, USER_COUNT, TX_COUNT);

    test_comparison(DIFFICULTY, USER_COUNT, TX_COUNT);

    std::cout << "\nnuoseklus vs lygiagretus\n";
    std::cout << "Nuoseklus:    " << (seq_ok ? "praejo" : "nepraejo") << "\n";
    std::cout << "Lygiagretus:  " << (par_ok ? "praejo" : "nepraejo") << "\n";

    return (seq_ok && par_ok) ? 0 : 1;
}