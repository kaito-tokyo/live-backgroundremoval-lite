/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: MIT
 *
 * KaitoTokyo CurlHelper Library
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include <curl/curl.h>

namespace KaitoTokyo::CurlHelper {

class CurlHandle {
	struct CurlDeleter {
		void operator()(CURL *ptr) const { curl_easy_cleanup(ptr); }
	};

public:
	CurlHandle() : curl_(curl_easy_init())
	{
		if (!curl_) {
			throw std::runtime_error("CurlEasyInitError(CurlHandle)");
		}
	}

	~CurlHandle() noexcept = default;

	CurlHandle(const CurlHandle &) = delete;
	CurlHandle &operator=(const CurlHandle &) = delete;
	CurlHandle(CurlHandle &&) = delete;
	CurlHandle &operator=(CurlHandle &&) = delete;

	[[nodiscard]]
	CURL *getRaw() const
	{
		return curl_.get();
	}

private:
	const std::unique_ptr<CURL, CurlDeleter> curl_;
};

} // namespace KaitoTokyo::CurlHelper
