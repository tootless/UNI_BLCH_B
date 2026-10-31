#pragma once
#include "hash.hpp"
#include "serialisation.hpp"
#include <array>
#include <cstdint>
#include <vector>

struct Transaction
{
private:
    uint32_t sender_id;
    uint32_t receiver_id;
    uint64_t amount;
    uint64_t sequence;
    std::array<uint32_t, 8> transaction_id;

public:
    Transaction(
        uint32_t sender,
        uint32_t receiver,
        uint64_t transaction_amount,
        uint64_t sequence_number)
        : sender_id(sender)
        , receiver_id(receiver)
        , amount(transaction_amount)
        , sequence(sequence_number)
        , transaction_id(get_transaction_id())
    {
    }

    std::vector<uint8_t> serialize_transaction() const
    {
        std::vector<uint8_t> bytes;
        append_bits(bytes, sender_id);
        append_bits(bytes, receiver_id);
        append_bits(bytes, amount);
        append_bits(bytes, sequence);
        return bytes;
    }

    std::array<uint32_t, 8> get_transaction_id() const
    {
        return hash_block(serialize_transaction());
    }

    uint32_t get_sender_id() const { return sender_id; }
    uint32_t get_receiver_id() const { return receiver_id; }
    uint64_t get_amount() const { return amount; }
    uint64_t get_sequence() const { return sequence; }
    const std::array<uint32_t, 8>& get_id() const { return transaction_id; }
};