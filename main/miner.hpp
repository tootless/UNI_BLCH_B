#include "hash.hpp"
#include "transaction.hpp"
#include "block.hpp"
#include "lib.hpp"

struct Miner
{
private:
	std::vector<Transaction> pending_transactions; //unvalidated transactions of future candidate block
	std::vector<Transaction> transaction_pool; //validated transactions 
	//some kind of "miner in use" marker? if its even needed - i think it might be node related instead
	std::uint32_t transaction_threshold;
	std::uint64_t time_passed; //inactivity timer
public:
	//validate every pending transaction
	//bool validate_transaction() {}

	//revalidate transaction pool (because of time passed and such), mine, construct block, initiate connection w blockchain

	//extract one bit from position index
	unsigned extract_bit(const std::array<uint32_t, 8>& hash, size_t index) {
		size_t pos = 31 - (index % 32);
		return (hash[index/32] >> pos) & 1u;
	}

	//create candidate (temporary) block, solve it's block hash
	Block solve_puzzle() {
		Block candidate_block(transaction_pool);
		auto diff = candidate_block.get_difficulty();
		auto candidate_nonce = candidate_block.get_nonce(); //0 at first

		do {
			bool valid = true;
			std::array<uint32_t, 8> candidate_hash = candidate_block.calculate_hash(candidate_nonce);

			for (size_t i = 0; i < diff; ++i) {
				if (extract_bit(candidate_hash, i) != 0) {
					valid = false;
					break;
				}
			}

			if (valid) {
				candidate_block.set_nonce(candidate_nonce);
				return candidate_block;
			}
			if (candidate_nonce == UINT64_MAX) break;

			++candidate_nonce;
		} while (true);

		throw std::runtime_error("no more nonce space, puzzle not solved");
		//can add other condition after
	}

	//void broadcast_block(Block mined_block) const {}
};