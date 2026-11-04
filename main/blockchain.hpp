#pragma once
#include "block.hpp"
#include <vector>
#include <array>
#include <stdexcept>

struct Blockchain
{
private:
    std::vector<Block> blockchain_blocks;

public:

    //blockchain ctor with genesis block
    Blockchain() {
        blockchain_blocks.emplace_back();
    }

    void add_block(const Block& block) {
        if (block.get_prev_hash() != get_latest_hash())
            throw std::runtime_error("invalid prev hash");

        if (!block.is_valid_pow())
            throw std::runtime_error("invalid pow");

        if (block.calculate_tx_root() != block.get_tx_root())
            throw std::runtime_error("invalid tx root");

        blockchain_blocks.push_back(block);
    }

    std::array<std::uint32_t, 8> get_latest_hash() const { return blockchain_blocks.back().get_hash();}

    //number of blocks
    std::size_t get_size() const { return blockchain_blocks.size();}

    const std::vector<Block>& get_blocks() const { return blockchain_blocks; }
};
