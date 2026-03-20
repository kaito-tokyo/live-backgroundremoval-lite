// SPDX-FileCopyrightText: 2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <span>
#include <vector>

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/sha256.h>

namespace KaitoTokyo::PluginManager {

class Sha256Calculator {
public:
	Sha256Calculator();
	~Sha256Calculator() noexcept;

	// No copy, no move
	Sha256Calculator(const Sha256Calculator &) = delete;
	Sha256Calculator &operator=(const Sha256Calculator &) = delete;
	Sha256Calculator(Sha256Calculator &&) = delete;
	Sha256Calculator &operator=(Sha256Calculator &&) = delete;

	void update(std::span<const std::uint8_t> data);
	void final();

	std::vector<std::uint8_t> toVector() const;
	std::string toHexString() const;

private:
	std::unique_ptr<wc_Sha256> sha_;
	bool finalized_ = false;
	std::vector<std::uint8_t> hash_;
};

} // namespace KaitoTokyo::PluginManager
