//v1.1 of hash function .https://github.com/tootless/UNI_BLCH_HF/tree/v.1.1
//library also used for block.hpp
#pragma once
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdint>
#include <iostream>
#include <array>
#include <vector>
#include <chrono>
#include <random>
#include <ctime>

constexpr size_t state_wcount = 8; //8 * 32 bits = 256
constexpr std::array<uint32_t, state_wcount> state_words = { //phi / (prime starting from 3) * 2^32
	2316467687u, 1389880613u, 992771867u, 631763915u,
	534569467u, 408788415u, 365758057u, 302147959u
};
constexpr size_t block_size = 32; //iteration input size
constexpr uint32_t value = 2654435769u; // 2^32/phi, rounded to uneven number cause thats better

inline uint32_t rotate(uint32_t state, unsigned value) {
	return (state << value) | (state >> (32u - value));
}

//32 byte block into state with two words so there is more interaction
inline void mix_block(std::array<uint32_t, state_wcount>& state, const std::vector<uint8_t>& block) {
	size_t i = 0;
	for (unsigned char c : block) {
		size_t firstw = i % state_wcount;
		size_t secondw = (i + 3) % state_wcount;
		state[firstw] += c;
		state[firstw] *= value;
		state[firstw] = rotate(state[firstw], 15);

		state[secondw] ^= state[firstw];
		++i;
	}
}

//uses the MD (merkel damgard) scheme
//begins with a vector of input data, can be any length
//adds a length marker,
//adds zeros until 8 last bytes are left,
//returns 32 bytes of data, padded to a number that is divisible by block_size
inline std::vector <uint8_t> pad(const std::vector<uint8_t>& data) {
	uint64_t byte_len = static_cast<uint64_t>(data.size());
	std::vector<uint8_t> padded(data.begin(), data.end());
	padded.push_back(0b10000000); //length marker
	while (padded.size() % block_size != (block_size - 8)) {
		padded.push_back(0x00); //0b00000000
	}
	//rshift and truncate - (11...u) gets trailing 0s to match length, then last 8bits are 1s
	for (int i = 0; i < 8; ++i) {
		padded.push_back(static_cast<uint8_t>((byte_len >> (8 * i)) & 0b11111111));
	}
	return padded;
}

//hashed result function for bytes
inline std::array<uint32_t, state_wcount> hash_block(const std::vector<uint8_t>& data) {
	std::array<uint32_t, state_wcount> state = state_words;
	std::vector<uint8_t> padded = pad(data);

	for (size_t offset = 0; offset < padded.size(); offset += block_size) {
		std::vector<uint8_t> block(padded.begin() + offset, padded.begin() + offset + block_size);
		mix_block(state, block);
	}

	return state;
}

inline std::string to_hex(const std::array<uint32_t, state_wcount>& state) {
	std::ostringstream oss;
	oss << std::hex << std::nouppercase << std::setfill('0');
	for (uint32_t w : state) {
		oss << std::setw(8) << w;
	}
	return oss.str();
}

//hashed result func for text input
inline std::array<uint32_t, state_wcount> hash_block(const std::string& text) {
	return hash_block(std::vector<uint8_t>(text.begin(), text.end()));
}
//padding func for text input
inline std::vector<uint8_t> pad(const std::string& data) {
	return pad(std::vector<uint8_t>(data.begin(), data.end()));
}

//with salt - only string
inline std::array<uint32_t, state_wcount> hash_block_salted(const std::string& data, const std::string& salt) {
	return hash_block(data + salt);
}

inline std::string salt(size_t num_bytes) {
	std::random_device rd;
	std::mt19937 gen(rd()); //seeded
	std::uniform_int_distribution<int> dist(0, 255);

	std::ostringstream oss;
	for (size_t i = 0; i < num_bytes; ++i) {
		uint8_t byte = static_cast<uint8_t>(dist(gen));
		oss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte);
	}
	return oss.str();
}

////help/other functions
//
////rand ASCII letters
//inline std::string random_text(size_t length) {
//	std::string text;
//	text.reserve(length);
//	for (size_t i = 0; i < length; ++i) {
//		int c = 33 + (rand() % (126 - 33 + 1));  //33-126
//		text += static_cast<char>(c);
//	}
//	return text;
//}
//
////reads from input/ in build files
//inline std::vector<uint8_t> read_data(const std::string& filename) {
//	std::string path = "inputs/" + filename;
//	std::ifstream f(path, std::ios::binary); //ios::binary, tik baitai skaitomi
//	if (!f) {
//		std::cerr << "KLAIDA: nepavyko atidaryti failo " << path << "\n";
//	}
//	//file data into iterator
//	std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
//
//	if (f.bad()) {
//		std::cerr << "KLAIDA: nepavyko nuskaityti failo " << path << "\n";
//	}
//	f.close();
//	return data;
//}
////creates a file with input data (string)
//inline void write_data(const std::string& filename, std::string& input) {
//	std::string path = "inputs/" + filename;
//	std::ofstream of(path, std::ios::binary);
//	of << input;
//	of.close();
//}