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

	//left || right serialisation
	std::vector<uint8_t> serialise() const {
		std::vector<uint8_t> node_bytes = {};

		append_hash(node_bytes, left->get_hash());
		append_hash(node_bytes, right->get_hash());

		return node_bytes;
	}
public:
	//ctor for leaf node
	Node(const Hash& transaction_id)
		: left(nullptr)
		, right(nullptr)
		, hash(transaction_id)
	{
	}

	//ctor for inner node
	Node(std::unique_ptr<Node> left_node, std::unique_ptr<Node> right_node)
		: left(std::move(left_node))
		, right(std::move(right_node))
		, hash({})
	{
		if (!left || !right) throw std::invalid_argument("inner node doesn't have two children");

		hash = calculate_hash();
	}

	Hash calculate_hash() const {
		return hash_block(serialise());
	}

	const Hash& get_hash() const {
		return hash;
	}
};

struct MerkleTree {
private:
	Hash root;
public:
	MerkleTree(const std::vector<Hash>& transaction_ids)
		: root(calculate_root(transaction_ids))
	{
	};

	Hash calculate_root(const std::vector<Hash>& tx_ids) const {
		//genesis root = {}
		if (tx_ids.empty()) {
			return {};
		}

		//add leaf nodes - tx ids
		std::vector<std::unique_ptr<Node>> curr_nodes; //current level of tree from the bottom-up
		curr_nodes.reserve(tx_ids.size());

		for (const auto& id : tx_ids) {
			curr_nodes.push_back(std::make_unique<Node>(id));
		}

		//add parent nodes iteratively
		while (curr_nodes.size() != 1) {
			std::vector<std::unique_ptr<Node>> next_nodes = {}; //next level of nodes
			size_t index = 0;
			next_nodes.reserve((curr_nodes.size() + 1) / 2);

			//add parent nodes
			for (auto& node : curr_nodes) {
				std::unique_ptr<Node> left_node = std::move(curr_nodes[index]);
				std::unique_ptr<Node> right_node = std::move(curr_nodes[index + 1]);

				if (index + 1 >= curr_nodes.size()) { //check if odd number of nodes
					right_node = std::make_unique<Node>(left_node->get_hash());

					next_nodes.push_back(
						std::make_unique<Node>(left_node, right_node);
						);
				}
				else {
					next_nodes.push_back(
						std::make_unique<Node>(left_node, right_node);
						);
					index++;
				}
			}

			//adjust levels; index starts at 0 every iteration
			curr_nodes = std::move(next_nodes);
		}
		return curr_nodes.begin()->get_hash();
	}

	const Hash& get_root() const {
		return root;
	}
};