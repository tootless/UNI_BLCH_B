#pragma once
#include "hash.hpp"
#include "transaction.hpp"

constexpr std::uint32_t protocol_version = 1;

struct Block
{
private:
	std::uint32_t version; //protocol version
	std::uint64_t timestamp;
	std::array<uint32_t, 8> prev_hash = {}; //0 for genesis
	std::uint64_t nonce;
	std::uint64_t difficulty; //starting hex 0 count
	std::array<uint32_t, 8> block_hash;
	std::vector<Transaction> block_transactions; //sorted by user id (sender or receiver explicitly) perhaps for quicker search later
	std::array<uint32_t, 8> tx_root; //hash of block's serialised transaction IDs
	std::vector<uint8_t> header;
public:
	Block() //only for genesis block for now
		: version(get_version())
		, timestamp(get_timestamp())
		, prev_hash({})
		, header(serialise_header())
		, block_hash(get_blockhash())
		, tx_root(get_txroot())
	{};

	inline const std::uint32_t get_version() { return protocol_version; }

	//unix epoch
	const std::uint64_t get_timestamp() {
		const auto curr_time = std::chrono::system_clock::now();
		return std::chrono::duration_cast<std::chrono::seconds>(curr_time.time_since_epoch()).count();
	}

	//header bytes serialised in this order, later templates will be used so it's easier
	//version, timestamp, ...
	std::vector<uint8_t> serialise_header() const{
		std::vector<uint8_t> header_bytes = {}; //1 position 1 byte
		size_t rshift_size = 8;
		uint8_t mask = 0b11111111;

		//push 1 byte of header metadata:
		for (size_t i = 0; i < (sizeof(version) * 8); i += rshift_size) {
			header_bytes.push_back(static_cast<uint8_t>(version >> i & mask));
		}
		for (size_t i = 0; i < (sizeof(timestamp) * 8); i += rshift_size) {
			header_bytes.push_back(static_cast<uint8_t>(timestamp >> i & mask));
		}

		return header_bytes;
	}

	std::array<uint32_t, 8> get_txroot() {
		//concat all transactions, return hashed
		std::vector<uint8_t> transactions_bytes = {};
		size_t rshift_size = 8;
		uint8_t mask = 0b11111111;

		for (const Transaction& transaction : block_transactions) {
			std::array<uint32_t, 8> transaction_id = transaction.get_transaction_id();

			for (uint32_t byte_sample : transaction_id) {
				for (size_t i = 0; i < (sizeof(byte_sample) * 8); i += rshift_size) {
					transactions_bytes.push_back(static_cast<uint8_t>(byte_sample >> i & mask));
				}
			}
		}
		return hash_block(transactions_bytes);
	}

	//genesis hash is header hash
	std::array<uint32_t, 8> get_blockhash() const{
		return hash_block(serialise_header());
	}
};