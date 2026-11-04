#pragma once
#include "user.hpp"
#include "transaction.hpp"
#include <cstdint>
#include <vector>
#include <random>
#include <stdexcept>
#include <unordered_map>

inline std::vector<Transaction> generate_transactions(const std::vector<User>& users, std::size_t count = 10000) {
	std::vector<Transaction> transactions;
	transactions.reserve(count);

	std::mt19937 rand(std::random_device{}());

	//randomly select users
	std::uniform_int_distribution<std::size_t> user_distribution(0, users.size() - 1);

	//generate transactions
	std::uniform_int_distribution<std::uint64_t> amount_distribution(1, 10000);

	std::unordered_map<std::uint32_t, std::uint64_t> sequences;

	for (const User& user : users)
		sequences[user.get_id()] = 0;

	for (std::size_t i = 0; i < count; ++i) {

		std::size_t sender_index = user_distribution(rand);
		std::size_t receiver_index = user_distribution(rand);

		//ensure sender != receiver
		while (sender_index == receiver_index) { receiver_index = user_distribution(rand); }

		const User& sender = users[sender_index];
		const User& receiver = users[receiver_index];

		std::uint64_t amount = amount_distribution(rand);

		std::uint64_t sequence = sequences.at(sender.get_id())++;

		transactions.emplace_back(sender.get_id(), receiver.get_id(), amount, sequence);
	}

	return transactions;
}
