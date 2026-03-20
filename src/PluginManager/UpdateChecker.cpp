// SPDX-FileCopyrightText: 2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

#include "KaitoTokyo/PluginManager/UpdateChecker.hpp"

#include <KaitoTokyo/CurlHelper/CurlSlistHandle.hpp>
#include <KaitoTokyo/CurlHelper/CurlWriteCallback.hpp>

#include "KaitoTokyo/PluginManager/Sha256Calculator.hpp"

namespace {

struct HTTPResponse {
	CURLcode curlCode;
	long statusCode = 0;
	std::vector<std::uint8_t> body;
};

HTTPResponse doGet(CURL *curl, const char *url, struct curl_slist *headers)
{
	HTTPResponse response;

	curl_easy_reset(curl);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 2L);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, KaitoTokyo::CurlHelper::CurlUint8VectorWriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);

	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

	response.curlCode = curl_easy_perform(curl);

	if (response.curlCode == CURLE_OK) {
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.statusCode);
	}

	return response;
}

HTTPResponse doHead(CURL *curl, const char *url, struct curl_slist *headers)
{
	HTTPResponse response;

	curl_easy_reset(curl);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 2L);

	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

	response.curlCode = curl_easy_perform(curl);

	if (response.curlCode == CURLE_OK) {
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.statusCode);
	}

	return response;
}

} // anonymous namespace

namespace KaitoTokyo::PluginManager {

std::optional<std::vector<std::uint8_t>> UpdateChecker::getLatestMetadata()
{
	CurlHelper::CurlSlistHandle headers;
	headers.append("User-Agent: LiveBackgroundRemovalLite/PluginManager");

	HTTPResponse response = doGet(curl_.getRaw(), getRemoteMetadataURL().c_str(), headers.getRaw());

	if (response.curlCode == CURLE_OK && response.statusCode == 200) {
		return std::move(response.body);
	} else {
		return std::nullopt;
	}
}

bool UpdateChecker::verifyMetadata(std::span<const std::uint8_t> metadata)
{
	Sha256Calculator sha;
	sha.update(metadata);
	sha.final();

	CurlHelper::CurlSlistHandle headers;
	headers.append("Accept: application/vnd.github+json");
	headers.append("X-GitHub-Api-Version: 2026-03-10");
	headers.append("User-Agent: LiveBackgroundRemovalLite/PluginManager");

	HTTPResponse response = doHead(curl_.getRaw(), getAttestationURL(sha.toHexString()).c_str(), headers.getRaw());

	if (response.curlCode == CURLE_OK && response.statusCode == 200) {
		return true;
	} else {
		return false;
	}
}

} // namespace KaitoTokyo::PluginManager
