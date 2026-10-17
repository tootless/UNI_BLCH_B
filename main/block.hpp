#pragma once
#include "hash.hpp"

struct Block
{
private:
	std::string header; //header will not be a string later on
	std::int64_t timestamp;
	std::array<uint32_t, 8> block_hash;
	std::array<uint32_t, 8> prev_hash;

public:
	//ctor genesis block
	Block(std::string header, std::string timestamp, std::array<uint32_t, 8> genesis_hash, std::array<uint32_t, 8> prev_hash = {})
	{
		set_header(header);
		set_timestamp(timestamp);
		set_genesishash(genesis_hash);
		prev_hash = {};
	};

	//ctor general block
	Block(std::string text, std::string timestamp, std::array<uint32_t, 8> block_hash, std::array<uint32_t, 8> prev_hash) {
		set_header(text);
		set_timestamp(timestamp);
		/*set_blockhash(block_hash);
		prev_hash = ?*/
	}

	inline void set_header(std::string text) { header = text;}

	//unix 
	void set_timestamp(){
		const auto curr_time = std::chrono::system_clock::now();
		timestamp = std::chrono::duration_cast<std::chrono::seconds>(curr_time.time_since_epoch()).count();
	}

	//genesis hash is header hash
	void set_genesishash(std::string header) {
		block_hash = hash_block(header);
	}

	/*void set_blockhash() {
		//uses every piece of data in the blockchain
	}*/
};