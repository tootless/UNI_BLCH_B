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
	std::vector<Transaction> pending_transactions; //candidate block transactions
	std::vector<Transaction> transaction_pool; //validated transactions

	std::uint32_t transaction_threshold = 100;

	std::mt19937 rng{ std::random_device{}() };

	using SequenceKey = std::pair<std::uint32_t, std::uint64_t>;
	std::set<SequenceKey> used_sequences;

	std::uint64_t last_attempts = 0;
	double last_mining_seconds = 0.0;

public:
	using Balances = std::unordered_map<std::uint32_t, std::uint64_t>;

	//add valid transaction to pool
	bool add_transaction(const Transaction& transaction, const Balances& balances, std::string& reason) {
		Balances temporary_balances = balances;
		TxValidation validation = apply_transaction(transaction, temporary_balances);

		if (!validation.valid) {
			reason = validation.reason;
			return false;
		}

		SequenceKey key{
			transaction.get_sender_id(),
			transaction.get_sequence()
		};

		if (used_sequences.find(key) != used_sequences.end()) {
			reason = "duplicate sequence for sender";
			return false;
		}

		used_sequences.insert(key);
		transaction_pool.push_back(transaction);

		return true;
	}

	//create candidate block and solve its block hash
	Block solve_puzzle(const std::vector<Transaction>& transactions, const std::array<std::uint32_t, 8>& previous_hash,
		std::uint64_t difficulty = 3){
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
	bool mine_next_block(Balances& balances, const std::array<std::uint32_t, 8>& previous_hash, Block& accepted_block, std::uint64_t difficulty = 3) {
		pending_transactions.clear();

		//random selection of transactions, not shuffling transaction_pool 
		std::vector<std::size_t> pool_indices(transaction_pool.size());

		for (std::size_t i = 0; i < pool_indices.size(); ++i) pool_indices[i] = i;

		std::shuffle(pool_indices.begin(), pool_indices.end(), rng);

		//validate temporary candidate transactions
		Balances temporary_balances = balances;

		std::vector<std::size_t> included_indices;

		for (std::size_t index : pool_indices) {
			if (pending_transactions.size() >= transaction_threshold) break;

			TxValidation validation = apply_transaction(transaction_pool[index], temporary_balances);

			if (validation.valid) {
				pending_transactions.push_back(transaction_pool[index]);
				included_indices.push_back(index);
			}

			//Temporarily invalid transactions remain in the pool.
		}

		//return if done
		if (pending_transactions.empty()) {
			std::cerr << "no more valid transactions exist\n";
			return false;
		}

		//mine candidate
		Block mined_block = solve_puzzle(pending_transactions, previous_hash, difficulty);

		//revalidate mined block
		bool valid_block = true;

		if (mined_block.get_prev_hash() != previous_hash) {
			std::cerr << "Mined block rejected: incorrect previous hash\n";
			valid_block = false;
		}

		if (!mined_block.is_valid_pow()) {
			std::cerr << "Mined block rejected: invalid pow\n";
			valid_block = false;
		}

		if (mined_block.calculate_tx_root() != mined_block.get_tx_root()) {
			std::cerr << "Mined block rejected: invalid tx_root \n";
			valid_block = false;
		}

		//re-validate transactions (state has changed)
		Balances verified_balances = balances;

		if (valid_block) {
			for (const Transaction& transaction : mined_block.get_transactions()) {
				TxValidation validation = apply_transaction(transaction, verified_balances);

				if (!validation.valid) {
					std::cerr << "Mined block rejected: " << validation.reason << '\n';
					valid_block = false;
					break;
				}
			}
		}

		if (!valid_block) return false;

		//commit revalidated (verified) transactions
		balances = std::move(verified_balances);
		accepted_block = mined_block;

		//Remove included transactions; keep temporarily invalid ones.
		std::sort(included_indices.begin(), included_indices.end(), std::greater<std::size_t>());

		for (std::size_t index : included_indices)
			transaction_pool.erase(transaction_pool.begin() + index);

		pending_transactions.clear();

		return true;
	}

	//get remaining transaction pool
	const std::vector<Transaction>& get_transaction_pool() const {
		return transaction_pool;
	}

	//laiko matavimai
	std::uint64_t get_last_attempts() const { return last_attempts; }
	double get_last_mining_seconds() const { return last_mining_seconds; }
};
