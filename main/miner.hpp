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

	std::array<uint32_t, 8> construct_solution(std::uint64_t difficulty) {


		return hash;
	}

	//create candidate (temporary) block, solve it's block hash
	Block solve_puzzle() {
		Block candidate_block = (transaction_pool, {});
		//construct solution
		std::uint64_t diff = candidate_block.get_difficulty();
		std::array<uint32, 8> solution_hash = construct_solution(diff);

		std::uint64_t nonce = candidate_block.get_nonce();
		std::array<uint32_t, 8> candidate_hash = {};
		do {
			candidate_hash = candidate_block.calculate_hash();
			if (candidate_hash == solution_hash) {
				return candidate_block;
			}
			++nonce;
		} while(candidate_hash != solution_hash)
	}

	//void broadcast_block(Block mined_block) const {}
};