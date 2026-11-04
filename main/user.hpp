#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <random>
#include <unordered_set>
#include <stdexcept>

struct User
{
private:
	std::uint32_t user_id;
	std::uint64_t balance;
	std::string name;
	std::string public_key;

public:
	User(std::uint32_t id, const std::string& user_name, const std::string& key, std::uint64_t initial_balance)
		: user_id(id),
		name(user_name),
		public_key(key),
		balance(initial_balance)
	{}

	std::uint32_t get_id() const { return user_id; }

	const std::string& get_name() const { return name; }

	const std::string& get_public_key() const { return public_key; }

	std::uint64_t get_balance() const { return balance; }
};

inline std::vector<User> generate_users(std::size_t count = 1000)
{
	std::vector<User> users;
	users.reserve(count);

	std::mt19937 rand(std::random_device{}());

	std::uniform_int_distribution<std::uint64_t> balance_distribution(100, 1000000);

	const std::vector<std::string> first_names = { "Jonas", "Petras", "Antanas", "Lukas", "Mantas", "Tomas", "Darius", "Andrius", "Paulius", "Karolis","Ona", "Ieva", "Greta", "Laura", "Egle","Austeja", "Gabija", "Monika", "Rasa", "Juste" };

	std::uniform_int_distribution<std::size_t> name_distribution(0, first_names.size() - 1);

	std::unordered_set<std::string> generated_keys;

	for (std::size_t i = 0; i < count; ++i) {

		std::string public_key;

		do {
			public_key = "public_key_" + std::to_string(i) + "_" + std::to_string(rand());
		} while (!generated_keys.insert(public_key).second);

		std::string name = first_names[name_distribution(rand)] + "_" + std::to_string(i + 1);

		std::uint64_t balance = balance_distribution(rand);

		users.emplace_back(static_cast<std::uint32_t>(i), name, public_key, balance);
	}

	return users;
}
