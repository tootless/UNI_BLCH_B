#include "user.hpp"
#include "transaction_generator.hpp"
#include "transaction_validate.hpp"
#include "miner.hpp"
#include "blockchain.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <chrono>

int main()
{
	Miner miner;

	Miner::Balances balances = {
		{1, 100}, // A
		{2, 0},   // B
		{3, 0}    // C
	};

	std::string reason;

	Transaction tx1(1, 2, 70, 1);
	Transaction tx2(1, 3, 60, 2);

	std::set<std::array<std::uint32_t, 8>> included_ids;

	std::cout << "T1: " << miner.add_transaction(tx1, balances, included_ids, reason) << " " << reason << '\n';
	reason.clear();

	std::cout << "T2: " << miner.add_transaction(tx2, balances, included_ids, reason) << " " << reason << '\n';

	//select candidate block
	std::array<std::uint32_t, 8> previous_hash{};

	Block candidate;
	Miner::Balances proposed_balances;
	std::vector<size_t> included_indices;

	bool mined = miner.mine_next_block(
		balances,
		previous_hash,
		included_ids,
		candidate,
		proposed_balances,
		included_indices,
		0
	);

	std::cout << "\ncandidate created: " << mined << '\n';
	std::cout << "transactions included: "
		<< candidate.get_transactions().size() << '\n';

	std::cout << "original A balance: " << balances.at(1) << '\n';

	if (mined) {
		std::cout << "Proposed A balance: "
			<< proposed_balances.at(1) << '\n';
	}

	return 0;
}