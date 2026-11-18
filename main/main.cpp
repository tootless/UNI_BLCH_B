
#include "blockchain.hpp"
#include "miner.hpp"
#include "transaction.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <iomanip>

using Hash = std::array<uint32_t, 8>;


bool test_candidate_winner_selection()
{
	std::cout << "\ntest of candidate winner\n";

	std::vector<MiningCandidate> candidates(5);

	candidates[0].solved = true;
	candidates[0].attempts = 8500;

	//this one should win
	candidates[1].solved = true;
	candidates[1].attempts = 3200;

	candidates[2].solved = false;
	candidates[2].attempts = 10000;

	candidates[3].solved = true;
	candidates[3].attempts = 5100;

	candidates[4].solved = true;
	candidates[4].attempts = 7400;

	Miner miner;

	size_t winner = miner.select_winner(candidates);

	const bool correct_winner = winner == 1;

	std::cout << "selected candidate: " << winner + 1 << "\nexpected candidate: 2\n";
	std::cout << "winner selection: " << (correct_winner ? "pass" : "fail") << "\n";

	return correct_winner;
}


//different number of transactions
bool test_merkle1()
{
	std::cout << "\nmerkle tree test 1\n";

	std::vector<Transaction> transactions;

	transactions.emplace_back(1, 2, 10, 1);
	transactions.emplace_back(1, 3, 20, 2);
	transactions.emplace_back(2, 3, 30, 3);
	transactions.emplace_back(3, 1, 40, 4);
	transactions.emplace_back(2, 1, 50, 5);

	const std::vector<size_t> test_sizes = { 1, 2, 3, 5 };

	bool all_passed = true;

	for (size_t count : test_sizes) {
		std::vector<Hash> transaction_ids;

		for (size_t i = 0; i < count; ++i) {
			transaction_ids.push_back(transactions[i].get_id());
		}

		MerkleTree tree(transaction_ids);

		const Hash first_root = tree.get_root();

		MerkleTree second_tree(transaction_ids);

		const Hash second_root = second_tree.get_root();

		const bool root_not_empty = first_root != Hash{};
		const bool roots_match = first_root == second_root;

		const bool passed = root_not_empty && roots_match;

		std::cout << count << " transactions: "
			<< (passed ? "pass" : "fail") << "\n";

		if (!root_not_empty) {
			std::cout << "merkle root empty\n";
		}

		if (!roots_match) {
			std::cout << "root is wrong, not matching\n";
		}

		all_passed = all_passed && passed;
	}

	return all_passed;
}

//root avalanche test
bool test_merkle2()
{
	std::cout << "\nmerkle test 2\n";

	Transaction tx1(1, 2, 100, 1);
	Transaction tx2(1, 2, 200, 1);

	std::vector<Hash> original_id = {tx1.get_id()};

	std::vector<Hash> changed_id = {tx2.get_id()};

	MerkleTree original_tree(original_id);
	MerkleTree changed_tree(changed_id);

	const Hash original_root = original_tree.get_root();
	const Hash changed_root = changed_tree.get_root();

	std::cout << "original root: "<< to_hex(original_root) << "\n";

	std::cout << "changed root:  "<< to_hex(changed_root) << "\n";

	const bool id_changed = tx1.get_id() != tx2.get_id();
	const bool root_changed = original_tree.get_root() != changed_tree.get_root();

	const bool passed = id_changed && root_changed;

	std::cout << "transaction ID changed: "
		<< (id_changed ? "pass" : "fail") << "\n";

	std::cout << "merkle root changed: "
		<< (root_changed ? "pass" : "fail") << "\n";

	return passed;
}


void print_balances(const BalanceState& balances)
{
	std::cout << "\nbalances:\n";

	for (const auto& user : balances) {
		std::cout << "user " << user.first
			<< ": " << user.second << '\n';
	}
}

void print_transaction_result(const std::string& name, bool accepted, const std::string& reason = "") {
	std::cout << name << ": "
		<< (accepted ? "accepted" : "rejected");

	if (!accepted && !reason.empty())
		std::cout << " - " << reason;

	std::cout << '\n';
}

//testing block rejection
bool test1()
{
	std::cout << "test 1\n";

	Blockchain blockchain(3);

	BalanceState balances = {
		{ 1, 100 },
		{ 2, 0 },
		{ 3, 0 }
	};

	const BalanceState original_balances = balances;
	const std::size_t original_chain_size = blockchain.get_size();

	const auto original_ids =
		blockchain.get_included_transaction_ids();

	const auto original_sequences =
		blockchain.get_included_sequences();

	Transaction tx1(1, 2, 70, 1001);
	Transaction tx2(1, 3, 60, 1002);

	std::vector<Transaction> invalid_transactions = { tx1,tx2 };

	Miner miner;

	Block invalid_block = miner.solve_puzzle(invalid_transactions, blockchain.get_latest_hash(), 3);

	bool block_rejected = false;

	try {
		blockchain.add_block(invalid_block, balances);
	}
	catch (const std::exception& e) {
		block_rejected = true;
		std::cout << "block rejected: " << e.what() << '\n';
	}

	const bool balances_unchanged = balances == original_balances;

	const bool chain_unchanged = blockchain.get_size() == original_chain_size;

	const bool ids_unchanged = blockchain.get_included_transaction_ids() == original_ids;

	const bool sequences_unchanged = blockchain.get_included_sequences() == original_sequences;

	std::cout << "Block rejected: " << (block_rejected ? "pass" : "fail") << '\n';

	std::cout << "Balances unchanged: " << (balances_unchanged ? "pass" : "fail") << '\n';

	std::cout << "Blockchain unchanged: " << (chain_unchanged ? "pass" : "fail") << '\n';

	std::cout << "Transaction IDs unchanged: " << (ids_unchanged ? "pass" : "fail") << '\n';

	std::cout << "Sequences unchanged: " << (sequences_unchanged ? "pass" : "fail") << '\n';

	print_balances(balances);

	return block_rejected &&
		balances_unchanged &&
		chain_unchanged &&
		ids_unchanged &&
		sequences_unchanged;
}

//testing tx pool and mining
bool test2()
{
	std::cout << "test 2\n";

	constexpr std::uint64_t difficulty = 3;

	Blockchain blockchain(difficulty);
	Miner miner;

	BalanceState balances = {
		{ 1, 100 },
		{ 2, 0 },
		{ 3, 0 }
	};

	print_balances(balances);

	//cant be in same block cause of amount
	Transaction tx1(1, 2, 70, 1);
	Transaction tx2(1, 3, 60, 2);

	std::string reason;

	bool tx1_accepted = miner.add_transaction(
		tx1,
		balances,
		blockchain.get_included_transaction_ids(),
		blockchain.get_included_sequences(),
		reason
	);

	print_transaction_result("T1: A to B, 70", tx1_accepted, reason);

	reason.clear();

	bool tx2_accepted = miner.add_transaction(
		tx2,
		balances,
		blockchain.get_included_transaction_ids(),
		blockchain.get_included_sequences(),
		reason
	);

	print_transaction_result("T2: A to C, 60", tx2_accepted, reason);

	if (!tx1_accepted || !tx2_accepted)
		return false;

	std::cout << "\ntransactions in pool: " << miner.get_transaction_pool().size() << '\n';

	//create and mine candidate block
	Block accepted_block;
	Miner::Balances proposed_balances;
	std::vector<std::size_t> included_indices;

	const bool candidate_created = miner.mine_next_block(
		balances,
		blockchain.get_latest_hash(),
		blockchain.get_included_transaction_ids(),
		blockchain.get_included_sequences(),
		accepted_block,
		proposed_balances,
		included_indices,
		difficulty
	);

	std::cout << "\ncandidate created: "
		<< (candidate_created ? "yes" : "no") << '\n';

	if (!candidate_created)
		return false;

	std::cout << "transactions included: "
		<< accepted_block.get_transactions().size() << '\n';

	std::cout << "mining attempts: "
		<< miner.get_last_attempts() << '\n';

	std::cout << "mining time: "
		<< std::fixed << std::setprecision(6)
		<< miner.get_last_mining_seconds()
		<< " seconds\n";

	//blockchain verification
	try {
		blockchain.add_block(accepted_block, balances);

		//remove transactions after success
		miner.commit_mined_transactions(included_indices);

		std::cout << "\nblock accepted\n";
	}
	catch (const std::exception& e) {
		std::cout << "\nblock rejected: " << e.what() << '\n';
		return false;
	}

	std::cout << "\nremaining transactions in pool: "
		<< miner.get_transaction_pool().size() << '\n';

	std::cout << "blockchain size: "
		<< blockchain.get_size() << '\n';

	print_balances(balances);

	return true;
}


int main()
{

	bool winner_test_passed = test_candidate_winner_selection();

	//bool merkle_tests_passed = test_merkle1() && test_merkle2();

	/*bool test_1 = test1();

	bool test_2 = test2();

	std::cout << "results\n";

	std::cout << "block rejection: "
		<< (test_1 ? "pass" : "fail") << '\n';

	std::cout << "Transaction pool and mining: "
		<< (test_2 ? "pass" : "fail") << '\n';

	if (test_1 && test_2) {
		std::cout << "\neverything passed yippee\n";
		return 0;
	}

	std::cout << "\nsomething failed\n";*/
	return 1;
}
