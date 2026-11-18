#pragma once
#include "transaction.hpp"
#include "block.hpp"
#include "transaction_validate.hpp"
#include <set>
#include <utility>
#include <algorithm>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <functional>
#include <chrono>

struct Miner
{
private:
	using Hash = std::array<std::uint32_t, 8>;

	std::vector<Transaction> pending_transactions; //candidate block transactions
	std::vector<Transaction> transaction_pool; //validated transactions

	std::uint32_t transaction_threshold = 100;

	std::mt19937 rng{ std::random_device{}() };

	std::set<std::uint64_t> used_sequences;

	std::uint64_t last_attempts = 0;
	double last_mining_seconds = 0.0;

public:
	using Balances = std::unordered_map<std::uint32_t, std::uint64_t>;

	//add valid transaction to pool
	bool add_transaction(const Transaction& transaction, const Balances& balances,
		const std::set<Hash>& included_transaction_ids,
		const std::set<std::uint64_t>& included_sequences, std::string& reason)
	{
		if (included_transaction_ids.find(transaction.get_id()) != included_transaction_ids.end()) {
			reason = "transaction ID already included in blockchain";
			return false;
		}

		for (const Transaction& pooled_tx : transaction_pool) {
			if (pooled_tx.get_id() == transaction.get_id()) {
				reason = "duplicate transaction ID in pool";
				return false;
			}
		}

		const std::uint64_t sequence = transaction.get_sequence();

		if (included_sequences.find(sequence) != included_sequences.end()) {
			reason = "transaction sequence already included in blockchain";
			return false;
		}

		if (used_sequences.find(sequence) != used_sequences.end()) {
			reason = "duplicate transaction sequence in pool";
			return false;
		}

		Balances temporary_balances = balances;
		TxValidation validation = apply_transaction(transaction, temporary_balances);

		if (!validation.valid) {
			reason = validation.reason;
			return false;
		}

		used_sequences.insert(sequence);
		transaction_pool.push_back(transaction);

		return true;
	}

	//create candidate block and solve its block hash
	Block solve_puzzle(const std::vector<Transaction>& transactions, const std::array<std::uint32_t, 8>& previous_hash,
		std::uint64_t difficulty = 3) {
		Block candidate_block(transactions, previous_hash, difficulty);

		if (difficulty > 64)
			throw std::runtime_error("difficulty bigger than 256bit hash");

		last_attempts = 0;
		const auto start = std::chrono::steady_clock::now();
		auto candidate_nonce = candidate_block.get_nonce();

		do {
			candidate_block.set_nonce(candidate_nonce);
			++last_attempts;

			//block pow
			if (candidate_block.is_valid_pow()) {
				const auto end = std::chrono::steady_clock::now();
				last_mining_seconds = std::chrono::duration<double>(end - start).count();

				return candidate_block;
			}

			if (candidate_nonce == std::numeric_limits<std::uint64_t>::max())
				break;

			++candidate_nonce;

		} while (true);

		const auto end = std::chrono::steady_clock::now();
		last_mining_seconds = std::chrono::duration<double>(end - start).count();

		throw std::runtime_error("no more nonce space, puzzle not solved");
	}

	//up to 100 transactions, validate in order, mine, revalidate and commit

	bool mine_next_block(
		const Balances& balances, const std::array<std::uint32_t, 8>& previous_hash,
		const std::set<Hash>& included_transaction_ids, const std::set<std::uint64_t>& included_sequences,
		Block& accepted_block, Balances& proposed_balances, 
		std::vector<size_t>& included_indices, std::uint64_t difficulty = 3)
	{
		pending_transactions.clear();
		included_indices.clear();

		std::vector<size_t> pool_indices(transaction_pool.size());

		for (size_t i = 0; i < pool_indices.size(); ++i)
			pool_indices[i] = i;

		std::shuffle(pool_indices.begin(), pool_indices.end(), rng);

		Balances temporary_balances = balances;
		std::set<Hash> candidate_transaction_ids;
		std::set<std::uint64_t> candidate_sequences;

		for (size_t index : pool_indices) {
			if (pending_transactions.size() >= transaction_threshold)
				break;

			const Transaction& tx = transaction_pool[index];
			const Hash& tx_id = tx.get_id();

			if (!tx.is_id_valid())
				continue;

			if (included_transaction_ids.find(tx_id) != included_transaction_ids.end())
				continue;

			if (!candidate_transaction_ids.insert(tx_id).second)
				continue;

			const std::uint64_t sequence = tx.get_sequence();

			if (included_sequences.find(sequence) != included_sequences.end()) {
				candidate_transaction_ids.erase(tx_id);
				continue;
			}

			if (!candidate_sequences.insert(sequence).second) {
				candidate_transaction_ids.erase(tx_id);
				continue;
			}

			TxValidation validation = apply_transaction(tx, temporary_balances);

			if (validation.valid) {
				pending_transactions.push_back(tx);
				included_indices.push_back(index);
			}
			else {
				candidate_transaction_ids.erase(tx_id);
				candidate_sequences.erase(sequence);
			}
		}

		if (pending_transactions.empty())
			return false;

		Block mined_block = solve_puzzle(
			pending_transactions,
			previous_hash,
			difficulty
		);

		bool valid_block = true;

		if (mined_block.get_prev_hash() != previous_hash)
			valid_block = false;

		if (!mined_block.is_valid_pow())
			valid_block = false;

		if (mined_block.calculate_tx_root() != mined_block.get_tx_root())
			valid_block = false;

		Balances verified_balances = balances;

		if (valid_block) {
			for (const Transaction& tx : mined_block.get_transactions()) {
				TxValidation validation = apply_transaction(tx, verified_balances);

				if (!validation.valid) {
					valid_block = false;
					break;
				}
			}
		}

		if (!valid_block)
			return false;

		proposed_balances = std::move(verified_balances);
		accepted_block = std::move(mined_block);

		return true;
	}

	void commit_mined_transactions(const std::vector<size_t>& included_indices)
	{
		std::vector<size_t> sorted_indices = included_indices;

		std::sort(sorted_indices.begin(), sorted_indices.end(), std::greater<size_t>());

		for (size_t index : sorted_indices) {
			if (index < transaction_pool.size()) {
				const std::uint64_t sequence = transaction_pool[index].get_sequence();

				used_sequences.erase(sequence);

				transaction_pool.erase(transaction_pool.begin() + index);
			}
		}

		pending_transactions.clear();
	}


	//get remaining transaction pool
	const std::vector<Transaction>& get_transaction_pool() const {
		return transaction_pool;
	}

	//laiko matavimai
	std::uint64_t get_last_attempts() const { return last_attempts; }
	double get_last_mining_seconds() const { return last_mining_seconds; }
};
