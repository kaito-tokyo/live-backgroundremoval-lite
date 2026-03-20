// SPDX-FileCopyrightText: 2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <KaitoTokyo/SimpleJsonReader/SimpleJsonReader.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <variant>

using namespace KaitoTokyo::SimpleJsonReader;

namespace {

struct EventRecord
{
	SimpleJsonReaderEventType type;
	std::string value;
	std::string jsonPath;
};

std::string jsonPathToString(const std::vector<SimpleJsonReaderJsonPath> &jsonPath)
{
	std::string result;

	for (const auto &segment : jsonPath) {
		if (std::holds_alternative<std::string_view>(segment)) {
			result.push_back('/');
			result.append(std::get<std::string_view>(segment));
		} else {
			result.push_back('[');
			result.append(std::to_string(std::get<std::size_t>(segment)));
			result.push_back(']');
		}
	}

	return result;
}

} // namespace

TEST(SimpleJsonReaderTest, ParsesNestedValuesWithJsonPath)
{
	std::vector<EventRecord> records;

	auto error = parseJson(R"(
{
	"root": {
		"items": ["alpha", {"enabled": true}]
	}
}
)",
			 [&](SimpleJsonReaderEvent event) {
				 records.push_back({event.type, std::string(event.value), jsonPathToString(event.jsonPath)});
			 });

	ASSERT_EQ(error, SimpleJsonReaderError::OK);

	const std::vector<EventRecord> expected = {
		{SimpleJsonReaderEventType::StartObject, "{", ""},
		{SimpleJsonReaderEventType::Key, "root", "/root"},
		{SimpleJsonReaderEventType::StartObject, "{", "/root"},
		{SimpleJsonReaderEventType::Key, "items", "/root/items"},
		{SimpleJsonReaderEventType::StartArray, "[", "/root/items"},
		{SimpleJsonReaderEventType::String, "alpha", "/root/items[0]"},
		{SimpleJsonReaderEventType::StartObject, "{", "/root/items[1]"},
		{SimpleJsonReaderEventType::Key, "enabled", "/root/items[1]/enabled"},
		{SimpleJsonReaderEventType::True, "true", "/root/items[1]/enabled"},
		{SimpleJsonReaderEventType::EndObject, "}", "/root/items[1]"},
		{SimpleJsonReaderEventType::EndArray, "]", "/root/items"},
		{SimpleJsonReaderEventType::EndObject, "}", "/root"},
		{SimpleJsonReaderEventType::EndObject, "}", ""},
	};

	ASSERT_EQ(records.size(), expected.size());
	for (std::size_t i = 0; i < expected.size(); ++i) {
		EXPECT_EQ(records[i].type, expected[i].type);
		EXPECT_EQ(records[i].value, expected[i].value);
		EXPECT_EQ(records[i].jsonPath, expected[i].jsonPath);
	}
}

TEST(SimpleJsonReaderTest, PreservesEscapedQuotesAndBackslashes)
{
	std::vector<EventRecord> records;

	auto error = parseJson(R"({"message":"a\"b\\c"})", [&](SimpleJsonReaderEvent event) {
		records.push_back({event.type, std::string(event.value), jsonPathToString(event.jsonPath)});
	});

	ASSERT_EQ(error, SimpleJsonReaderError::OK);

	const std::vector<EventRecord> expected = {
		{SimpleJsonReaderEventType::StartObject, "{", ""},
		{SimpleJsonReaderEventType::Key, "message", "/message"},
		{SimpleJsonReaderEventType::String, R"(a\"b\\c)", "/message"},
		{SimpleJsonReaderEventType::EndObject, "}", ""},
	};

	ASSERT_EQ(records.size(), expected.size());
	for (std::size_t i = 0; i < expected.size(); ++i) {
		EXPECT_EQ(records[i].type, expected[i].type);
		EXPECT_EQ(records[i].value, expected[i].value);
		EXPECT_EQ(records[i].jsonPath, expected[i].jsonPath);
	}
}
