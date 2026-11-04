#pragma once
#include "transaction.hpp"
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>

using BalanceState = std::unordered_map<uint32_t, uint64_t>;

struct TxValidation
{
    bool valid;
    std::string reason;
};

inline TxValidation apply_transaction(const Transaction& tx, BalanceState& state)
{
    const uint32_t sender = tx.get_sender_id();
    const uint32_t receiver = tx.get_receiver_id();
    const uint64_t amount = tx.get_amount();

    if (sender == receiver) {
        return { false, "sender and receiver cant be the same" };
    }

    if (amount == 0) {
        return { false, "amount has to be more than 0" };
    }

    if (!tx.is_id_valid()) {
        return { false, "transaction ID is invalid" };
    }

    auto sender_it = state.find(sender);
    auto receiver_it = state.find(receiver);

    if (sender_it == state.end()) {
        return { false, "sender doesnt exist" };
    }

    if (receiver_it == state.end()) {
        return { false, "receiver doesnt exist" };
    }

    if (sender_it->second < amount) {
        return { false, "not enough balance" };
    }

    if (receiver_it->second > std::numeric_limits<uint64_t>::max() - amount) {
        return { false, "receiver balance overflow" };
    }

    //update balances
    sender_it->second -= amount;
    receiver_it->second += amount;

    return { true, "" };
}
