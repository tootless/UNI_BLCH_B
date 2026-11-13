#include "hash.hpp"
#include "serialisation.hpp"
#include "transaction.hpp"
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>
#include <array>
#include <stdexcept>

using Hash = std::array<std::uint32_t, 8>; //will have to refactor rest of code with this later

struct Node {
private:
	//left right children
	std::unique_ptr<Node> left;
	std::unique_ptr<Node> right;
	Hash hash;
public:
	//ctor for leaf node
	Node(const Hash& transaction_id)
		: left(nullptr)
		, right(nullptr)
		, hash(transaction_id)
	{}

	//ctor for inner node
	Node(std::unique_ptr<Node> left_node, std::unique_ptr<Node> right_node)
		: left(std::move(left_node))
		, right(std::move(right_node))
		, hash({})
	{
		if (!left || !right) throw std::invalid_argument("inner node doesn't have two children");

		hash = calculate_hash();
	}

	//left || right serialisation
	std::vector<uint8_t> serialise() const {
		std::vector<uint8_t> node_bytes = {};

		append_hash(node_bytes, left->get_hash());
		append_hash(node_bytes, right->get_hash());

		return node_bytes;
	}

	Hash calculate_hash() const {
		return hash_block(serialise());
	}

	const Hash& get_hash() const {
		return hash;
	}

	bool is_leaf() const {
		if (!left && !right) { //! is equivalent to = nullptr (std::unique_ptr)
			return true;
		}
		else return false;
	}
};

struct MerkleTree {
private:
	std::unique_ptr<Node> root; //if node is_leaf and height = 1, that node is root;
	std::vector<Transaction> transactions; //valid transactions of the block
	//height
public:
	MerkleTree(const std::vector<std::array<std::uint32_t, 8>>& tx_ids)
		:
	{};
};