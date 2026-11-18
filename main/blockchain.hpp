#pragma once
#include "block.hpp"
#include "transaction_validate.hpp"
#include <vector>
#include <array>
#include <stdexcept>
#include <set>
#include <cstdint>

constexpr std::size_t max_transactions_per_block = 100;

struct Blockchain
{
private:
    std::vector<Block> blockchain_blocks;

    using Hash = std::array<std::uint32_t, 8>; //transaction id
    std::set<Hash> included_transaction_ids;
    std::set<std::uint64_t> included_sequences;

    std::uint64_t required_difficulty;

public:

    //blockchain ctor with genesis block
    Blockchain(std::uint64_t difficulty = 3)
        : required_difficulty(difficulty)
    {
        if (difficulty > 64)
            throw std::invalid_argument("difficulty cannot exceed 64");

        blockchain_blocks.emplace_back();
    }

    void add_block(const Block& block, BalanceState& balances) {
        // Check block structure and header.
        if (block.get_prev_hash() != get_latest_hash())
            throw std::runtime_error("invalid prev hash");

        if (block.get_version() != protocol_version)
            throw std::runtime_error("unsupported block version");

        if (block.get_difficulty() != required_difficulty)
            throw std::runtime_error("incorrect block difficulty");

        if (block.get_transactions().size() > max_transactions_per_block)
            throw std::runtime_error("too many transactions in block");

        if (!block.is_valid_pow())
            throw std::runtime_error("invalid pow");

        if (block.calculate_tx_root() != block.get_tx_root())
            throw std::runtime_error("invalid tx root");

        BalanceState temp_balances = balances;
        std::set<Hash> candidate_transaction_ids;
        std::set<std::uint64_t> candidate_sequences;

        //validate transactions
        for (const Transaction& tx : block.get_transactions()) {

            const Hash& tx_id = tx.get_id();

            //check if this ID was included in an earlier block
            if (included_transaction_ids.find(tx_id) != included_transaction_ids.end())
                throw std::runtime_error("transaction ID already included in blockchain");

            if (!candidate_transaction_ids.insert(tx_id).second)
                throw std::runtime_error("duplicate transaction ID in block");

            const std::uint64_t sequence = tx.get_sequence();

            if (included_sequences.find(sequence) != included_sequences.end()) {
                throw std::runtime_error("transaction sequence already used in blockchain");
            }

            if (!candidate_sequences.insert(sequence).second) {
                throw std::runtime_error("duplicate transaction sequence in block");
            }

            TxValidation validation = apply_transaction(tx, temp_balances);

            if (!validation.valid)
                throw std::runtime_error("invalid transaction: " + validation.reason);
        }

        //prepare updated ids and sequences, no commit yet
        auto updated_transaction_ids = included_transaction_ids;

        updated_transaction_ids.insert(
            candidate_transaction_ids.begin(),
            candidate_transaction_ids.end()
        );

        auto updated_sequences = included_sequences;

        updated_sequences.insert(candidate_sequences.begin(), candidate_sequences.end());

        //accept block
        blockchain_blocks.push_back(block);

        //commit after approval
        balances.swap(temp_balances);
        included_transaction_ids.swap(updated_transaction_ids);
        included_sequences.swap(updated_sequences);
    }

    std::array<std::uint32_t, 8> get_latest_hash() const {
        return blockchain_blocks.back().get_hash();
    }

    //number of blocks
    std::size_t get_size() const {
        return blockchain_blocks.size();
    }

    const std::vector<Block>& get_blocks() const {
        return blockchain_blocks;
    }

    const std::set<Hash>& get_included_transaction_ids() const {
        return included_transaction_ids;
    }

    const std::set<std::uint64_t>& get_included_sequences() const {
        return included_sequences;
    }
};
