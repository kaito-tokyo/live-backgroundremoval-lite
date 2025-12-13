/*
 * Live Background Removal Lite
 * Copyright (c) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT" 
 * in the distribution root.
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
