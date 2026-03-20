// SPDX-FileCopyrightText: 2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <span>
#include <string>
#include <vector>

#include <curl/curl.h>

#include <KaitoTokyo/CurlHelper/CurlHandle.hpp>

namespace KaitoTokyo::PluginManager {

class UpdateChecker {
public:
	UpdateChecker() = default;
	~UpdateChecker() noexcept = default;

	// No copy, no move
	UpdateChecker(const UpdateChecker &) = delete;
	UpdateChecker &operator=(const UpdateChecker &) = delete;
	UpdateChecker(UpdateChecker &&) = delete;
	UpdateChecker &operator=(UpdateChecker &&) = delete;

	std::string getRemoteMetadataURL()
	{
		return "https://kaito-tokyo.github.io/live-backgroundremoval-lite/metadata/latest-metadata.json";
	}

	std::string getAttestationURL(const std::string &subjectDigest)
	{
		std::string url = "https://api.github.com/repos/kaito-tokyo/live-backgroundremoval-lite/attestations/sha256:";
		url.reserve(url.size() + subjectDigest.size());
		url.append(subjectDigest);
		return url;
	}

	std::optional<std::vector<std::uint8_t>> getLatestMetadata();
	bool verifyMetadata(std::span<const std::uint8_t> metadata);

private:
	CurlHelper::CurlHandle curl_;
};

} // namespace KaitoTokyo::PluginManager
