#pragma once
#include "hash.hpp"

constexpr std::uint32_t protocol_version = 1;

struct Block
{
private:
	std::uint32_t version; //protocol version -- 4 bytes
	std::uint64_t timestamp; //8 bytes
	std::array<uint32_t, 8> prev_hash = {}; //0 for genesis -- 32 bytes
	std::uint64_t nonce; //8 bytes
	std::array<uint32_t, 8> merkle_root; //32 bytes -- maybe it can be used to identify the genesis block ctor or something?
	std::array<uint32_t, 8> block_hash; //32 bytes
	std::vector<uint8_t> header;
public:
	//ctor genesis block
	Block()
		: version(get_version())
		, timestamp(get_timestamp())
		, prev_hash({})
		, header(get_serialised_header())
		, block_hash(get_blockhash())
	{
	};

	//ctor general block
	//Block(std::array<uint32_t, 8> hash, std::array<uint32_t, 8> previous_hash) {}

	inline const std::uint32_t get_version() { return protocol_version; }

	//unix epoch
	const std::uint64_t get_timestamp() {
		const auto curr_time = std::chrono::system_clock::now();
		return std::chrono::duration_cast<std::chrono::seconds>(curr_time.time_since_epoch()).count();
	}

	//genesis hash is header hash
	const std::array<uint32_t, 8> get_blockhash() {
		return hash_block(get_serialised_header());
	}

	std::vector<uint8_t> get_serialised_header() {
		std::vector<uint8_t> header_bytes = {}; //1 position 1 byte
		size_t rshift_size = 8;
		uint8_t mask = 0b11111111;

		//push 1 byte of everything onto the header bytes vector
		for (size_t i = 0; i < (sizeof(timestamp) * 8); i += rshift_size) {
			header_bytes.push_back(static_cast<uint8_t>(timestamp >> i & mask));
		}
		for (size_t i = 0; i < (sizeof(version) * 8); i += rshift_size) {
			header_bytes.push_back(static_cast<uint8_t>(version >> i & mask));
		}

		return header_bytes;
	}
};