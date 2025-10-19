/*
Live Background Removal Lite
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <gtest/gtest.h>

#include <UpdateChecker.hpp>

#include <curl/curl.h>

class UpdateCheckerTest : public ::testing::Test {
protected:
	static void SetUpTestSuite() { curl_global_init(CURL_GLOBAL_DEFAULT); }
	static void TearDownTestSuite() { curl_global_cleanup(); }
};

TEST_F(UpdateCheckerTest, FetchLatestVersion_InvalidUrl)
{
	EXPECT_THROW({ KaitoTokyo::UpdateChecker::fetchLatestVersion(""); }, std::invalid_argument);
}

TEST_F(UpdateCheckerTest, FetchLatestVersion_ValidUrl)
{
	std::string url = "https://kaito-tokyo.github.io/live-backgroundremoval-lite/metadata/latest-version.txt";
	std::string result = KaitoTokyo::UpdateChecker::fetchLatestVersion(url);
	EXPECT_FALSE(result.empty());
}
