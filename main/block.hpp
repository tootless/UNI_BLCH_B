#pragma once
#include "hash.hpp"
#include "transaction.hpp"
#include "serialisation.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <vector>
#include <utility>

constexpr std::uint32_t protocol_version = 1;

struct Block
{
private:
	uint32_t version;
    uint64_t timestamp;
    std::array<uint32_t, 8> prev_hash;
    uint64_t nonce;
    uint64_t difficulty;
    std::vector<Transaction> block_transactions;
    std::array<uint32_t, 8> tx_root;

	static uint64_t curr_timestamp(){
		const auto curr_time = std::chrono::system_clock::now();
		return std::chrono::duration_cast<std::chrono::seconds>(curr_time.time_since_epoch()).count();
	}
public:

	//ctor candidate block, genesis if everything 0/{}
	Block(std::vector<Transaction> transactions, const std::array<uint32_t, 8>& previous_hash, uint64_t diff = 3)
		: version(protocol_version)
		, timestamp(curr_timestamp())
		, prev_hash(previous_hash)
		, nonce(0)
		, difficulty(diff)
		, block_transactions(std::move(transactions))
		, tx_root{}
	{
		tx_root = get_tx_root();
	}

	const std::vector<Transaction>& get_transactions() const{
		return block_transactions;
	}

	void set_nonce(uint64_t value) { 
		nonce = value; 
	}

	uint64_t get_nonce() const { 
		return nonce;
	}

	inline const std::uint64_t get_difficulty() { 
		return difficulty; 
	}

	std::vector<uint8_t> serialise_header(uint64_t candidate_nonce) const{
		std::vector<uint8_t> bytes;

		append_bits(bytes, version);
		append_bits(bytes, timestamp);
		append_hash(bytes, prev_hash);
		append_hash(bytes, tx_root);
		append_bits(bytes, candidate_nonce);
		append_bits(bytes, difficulty);

		return bytes;
	}

	std::array<uint32_t, 8> calculate_hash(uint64_t candidate_nonce) const{
		return hash_block(serialise_header(candidate_nonce));
	}

	std::array<uint32_t, 8> calculate_tx_root() const {
		std::vector<uint8_t> bytes;
		for (const Transaction& transaction : block_transactions) {
			append_hash(bytes, transaction.get_id());
		}
		return hash_block(bytes);
	}

	const std::array<uint32_t, 8>& get_prev_hash() const{
		return prev_hash;
	}

	const std::array<uint32_t, 8>& get_tx_root() const{
		return tx_root;
	}
};