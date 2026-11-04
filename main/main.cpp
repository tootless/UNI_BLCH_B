#include "user.hpp"
#include "transaction_generator.hpp"
#include "transaction_validate.hpp"
#include "miner.hpp"
#include "blockchain.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <chrono>

int main()
{
    const auto program_start = std::chrono::steady_clock::now();
    const unsigned int seed = 3719;
    std::mt19937 rand(seed);

    std::vector<User> users = generate_users(1000,rand);
    //initialise
    Miner::Balances balances;
    for (const User& user : users)
        balances[user.get_id()] = user.get_balance();

    std::uint64_t initial_total_balance = 0; //all balance amount
    for (const auto& account : balances)
        initial_total_balance += account.second;

    std::vector<Transaction> transactions = generate_transactions(users, 10000,rand);

    //accept or reject from mining transaction pool
    Miner miner;
    std::size_t rejected_transactions = 0;
    std::unordered_map<std::string, std::size_t> rejected_reasons;

    for (const Transaction& transaction : transactions) {
        //count rejection amount
        std::string reason;
        if (!miner.add_transaction(transaction, balances, reason)) {
            ++rejected_transactions;
            ++rejected_reasons[reason];
        }
    }

    const std::size_t accepted_to_pool =  miner.get_transaction_pool().size();

    Blockchain blockchain;
    std::size_t mined_blocks = 0;
    std::size_t included_transactions = 0;

    std::cout << "sukurtas Genesis blokas\n";

    //mining
    while (!miner.get_transaction_pool().empty()) {
        Block accepted_block;

        const auto previous_hash = blockchain.get_latest_hash();

        if (!miner.mine_next_block( balances, previous_hash, accepted_block, 3)) {
            std::cout << "kasimas baigesi, nebera valid transakciju\n";
            break;
        }

        blockchain.add_block(accepted_block);
        ++mined_blocks;
        included_transactions += accepted_block.get_transactions().size();

        std::cout << "\n--- Priimtas blokas ---\n";
        std::cout << "Height: " << blockchain.get_size() - 1 << '\n';
        std::cout << "Transakciju sk.: " << accepted_block.get_transactions().size() << '\n';
        std::cout << "prev_hash: " << to_hex(accepted_block.get_prev_hash()) << '\n';
        std::cout << "tx_root: " << to_hex(accepted_block.get_tx_root()) << '\n';
        std::cout << "Bloko hash: "  << to_hex(accepted_block.get_hash()) << '\n';
        std::cout << "Difficulty: " << accepted_block.get_difficulty() << '\n';
        std::cout << "Nonce: " << accepted_block.get_nonce() << '\n';
        std::cout << "Bandymu sk.: " << miner.get_last_attempts() << '\n';
        std::cout << "Kasimo laikas: " << std::fixed << std::setprecision(6) << miner.get_last_mining_seconds() << " s\n";
    }

    //output remaining transaction reasons
    std::unordered_map<std::string, std::size_t> remaining_reasons;

    for (const Transaction& transaction : miner.get_transaction_pool()) {
        Miner::Balances temporary_balances = balances;

        //validate copy to find reasons
        TxValidation validation = apply_transaction(transaction, temporary_balances);
        const std::string reason = validation.valid ? "tiesiog nepasirinkta (shufflinta per toli; 100 tran. limitas, t.t.)" : validation.reason;
        ++remaining_reasons[reason];    
    }

    std::uint64_t final_total_balance = 0; //all balance amount
    for (const auto& account : balances) final_total_balance += account.second;

    const auto program_end = std::chrono::steady_clock::now();
    const double total_seconds = std::chrono::duration<double>(program_end - program_start).count();

    std::cout << "\nRezultatai:\n\n";
    std::cout << "Vartotoju sk.: " << users.size() << '\n';
    std::cout << "Transakciju sk.: " << transactions.size() << '\n';
    std::cout << "Priimti blokai, be genesis: " << mined_blocks << '\n';
    std::cout << "Transakcijos baseine: " << accepted_to_pool << '\n';
    std::cout << "Transakcijos blokuose: " << included_transactions << '\n';
    std::cout << "Atmestos transakcijos: " << rejected_transactions << '\n';
    std::cout << "Transakcijos vis dar baseinuose: " << miner.get_transaction_pool().size() << '\n';
    std::cout << "Pirmutine total balance " << initial_total_balance << '\n';
    std::cout << "Finaline total balance: " << final_total_balance << '\n';
    std::cout << "Programos laikas: " << std::fixed << std::setprecision(3) << total_seconds << " s\n";

    std::cout << "\nAtmestu tran. priezastys:\n";
    for (const auto& t : rejected_reasons)
        std::cout << "  " << t.first << ": " << t.second << '\n';

    std::cout << "\nPasilikusiu tran. priezastys:\n";
    for (const auto& t : remaining_reasons)
        std::cout << "  " << t.first << ": " << t.second << '\n';

    if (initial_total_balance == final_total_balance)
        std::cout << "\ntotal balance check praejo\n";
    else
        std::cout << "\nkazkas negerai su total balance\n";

    return 0;
}