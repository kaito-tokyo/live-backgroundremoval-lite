/*
obs-showdraw
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#pragma once

#include <cpr/session.h>
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

inline CURLcode ssl_ctx_callback(CURL *curl, void *ssl_ctx, void *userptr)
{
	(void)curl;
	(void)userptr;

	WOLFSSL_CTX *ctx = (WOLFSSL_CTX *)ssl_ctx;

	if (wolfSSL_CTX_load_system_CA_certs(ctx) != WOLFSSL_SUCCESS) {
		return CURLE_SSL_CACERT_BADFILE;
	}

	return CURLE_OK;
}

class MyCprSession : public ::cpr::Session {
public:
	MyCprSession()
	{
		CURL *curl = GetCurlHolder()->handle;
		curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, ssl_ctx_callback);
	}
};

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
