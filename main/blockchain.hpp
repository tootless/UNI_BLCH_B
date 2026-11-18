#pragma once
#include "block.hpp"
#include <vector>
#include <array>
#include <stdexcept>
#include <set>
#include <cstdint>

struct Blockchain
{
private:
    using Hash = std::array<std::uint32_t, 8>;

    std::vector<Block> blockchain_blocks;
    std::set<Hash> included_transaction_ids; //here, hash = transaction id
    std::uint64_t required_difficulty;
public:

    //blockchain ctor with genesis block
    //when miner is called, use the same constexpr difficulty for miner as you did for blockchain
    Blockchain(std::uint64_t difficulty = 3)
        : required_difficulty(difficulty)
    {
        if (difficulty > 64)
            throw std::invalid_argument("difficulty cannot be more than 64");

        blockchain_blocks.emplace_back();
    }

    void add_block(const Block& block) {
        if (block.get_version() != protocol_version)
            throw std::runtime_error("unsupported block version");

        if (block.get_difficulty() != required_difficulty)
            throw std::runtime_error("incorrect block difficulty");

        if (block.get_prev_hash() != get_latest_hash())
            throw std::runtime_error("invalid prev hash");

        if (!block.is_valid_pow())
            throw std::runtime_error("invalid pow");

        if (block.calculate_tx_root() != block.get_tx_root())
            throw std::runtime_error("invalid tx root");

        const auto& transactions = block.get_transactions();

        //check transaction ids before accepting into block
        std::set<Hash> candidate_transaction_ids;

        for (const Transaction& tx : transactions) {
            if (!tx.is_id_valid())
                throw std::runtime_error("invalid transaction ID");

            const Hash& tx_id = tx.get_id();

            if (included_transaction_ids.find(tx_id) != included_transaction_ids.end())
                throw std::runtime_error("transaction ID already included in blockchain");

            if (!candidate_transaction_ids.insert(tx_id).second)
                throw std::runtime_error("duplicate transaction ID in block");
        }

        //updated transaction set before modifying the blockchain
        auto updated_transaction_ids = included_transaction_ids;

        updated_transaction_ids.insert(
            candidate_transaction_ids.begin(),
            candidate_transaction_ids.end()
        );

        //block accepted, then transaction ids are fully valid
        blockchain_blocks.push_back(block);
        included_transaction_ids.swap(updated_transaction_ids);
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
};
