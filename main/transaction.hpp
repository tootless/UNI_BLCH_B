#pragma once
#include "lib.hpp"

struct Transaction
{
private:
	std::uint32_t sender_id;
	std::uint32_t receiver_id;
	std::uint64_t amount;
	std::uint64_t nonce; //later on, for id
	std::array<uint32_t, 8> transaction_id;
public:
	Transaction()
		: transaction_id(get_transaction_id())
		, sender_id()
		, receiver_id()
		, amount()
	{
	};

	std::vector<uint8_t> serialise_transaction() const {
		std::vector<uint8_t> transaction_bytes = {}; //1 position 1 byte
		size_t rshift_size = 8;
		uint8_t mask = 0b11111111;

		//push 1 byte of transaction metadata:
		for (size_t i = 0; i < (sizeof(sender_id) * 8); i += rshift_size) { //research CHAR_BITS
			transaction_bytes.push_back(static_cast<uint8_t>(sender_id >> i & mask));
		}
		for (size_t i = 0; i < (sizeof(receiver_id) * 8); i += rshift_size) {
			transaction_bytes.push_back(static_cast<uint8_t>(receiver_id >> i & mask));
		}
		for (size_t i = 0; i < (sizeof(amount) * 8); i += rshift_size) {
			transaction_bytes.push_back(static_cast<uint8_t>(amount >> i & mask));
		}

		return transaction_bytes;
	}

	std::array<uint32_t, 8> get_transaction_id() const{
		auto transaction_bytes = serialise_transaction();
		return hash_block(transaction_bytes);
	}
};