// SPDX-FileCopyrightText: 2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

#include "KaitoTokyo/PluginManager/Sha256Calculator.hpp"

#include <limits>
#include <stdexcept>

namespace KaitoTokyo::PluginManager {

Sha256Calculator::Sha256Calculator() : sha_(std::make_unique<wc_Sha256>()), hash_(WC_SHA256_DIGEST_SIZE)
{
	if (wc_InitSha256(sha_.get()) != 0) {
		throw std::runtime_error("InitError(Sha256Calculator::Sha256Calculator)");
	}
}

Sha256Calculator::~Sha256Calculator() noexcept
{
	if (sha_) {
		wc_Sha256Free(sha_.get());
	}
}

void Sha256Calculator::update(std::span<const std::uint8_t> data)
{
	if (data.size() > std::numeric_limits<word32>::max()) {
		throw std::runtime_error("DataTooLargeError(Sha256Calculator::update)");
	}

	if (finalized_) {
		throw std::runtime_error("AlreadyFinalizedError(Sha256Calculator::update)");
	}

	if (wc_Sha256Update(sha_.get(), reinterpret_cast<const byte *>(data.data()),
			    static_cast<word32>(data.size())) != 0) {
		throw std::runtime_error("UpdateError(Sha256Calculator::update)");
	}
}

void Sha256Calculator::final()
{
	if (finalized_) {
		throw std::runtime_error("AlreadyFinalizedError(Sha256Calculator::final)");
	}

	if (wc_Sha256Final(sha_.get(), reinterpret_cast<byte *>(hash_.data())) != 0) {
		throw std::runtime_error("FinalError(Sha256Calculator::final)");
	}

	finalized_ = true;
}

std::vector<std::uint8_t> Sha256Calculator::toVector() const
{
	if (!finalized_) {
		throw std::runtime_error("NotFinalizedError(Sha256Calculator::toVector)");
	}

	return hash_;
}

std::string Sha256Calculator::toHexString() const
{
	if (!finalized_) {
		throw std::runtime_error("NotFinalizedError(Sha256Calculator::toHexString)");
	}

	static constexpr char hexDigits[] = "0123456789abcdef";
	std::string hexString(hash_.size() * 2, '\0');

	for (std::size_t i = 0; i < hash_.size(); i += 1) {
		hexString[i * 2] = hexDigits[hash_[i] >> 4];
		hexString[i * 2 + 1] = hexDigits[hash_[i] & 0x0f];
	}

	return hexString;
}

} // namespace KaitoTokyo::PluginManager
