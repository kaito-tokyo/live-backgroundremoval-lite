// SPDX-FileCopyrightText: 2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <KaitoTokyo/SimpleJsonReader/SimpleJsonReader.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace KaitoTokyo::SimpleJsonReader;

namespace {

struct EventRecord
{
	SimpleJsonReaderEvent event;
	std::string value;
};

} // namespace

TEST(SimpleJsonReaderTest, ParsesMixedObject)
{
	std::vector<EventRecord> records;

	parseJson(R"(
{
	"name": "alpha",
	"count": 3,
	"enabled": true,
	"items": [null, false, "x"]
}
)",
		  [&](SimpleJsonReaderEvent event, std::string_view value) { records.push_back({event, std::string(value)}); });

	const std::vector<EventRecord> expected = {
		{SimpleJsonReaderEvent::StartObject, "{"},
		{SimpleJsonReaderEvent::Key, "name"},
		{SimpleJsonReaderEvent::String, "alpha"},
		{SimpleJsonReaderEvent::Key, "count"},
		{SimpleJsonReaderEvent::Number, "3"},
		{SimpleJsonReaderEvent::Key, "enabled"},
		{SimpleJsonReaderEvent::True, "true"},
		{SimpleJsonReaderEvent::Key, "items"},
		{SimpleJsonReaderEvent::StartArray, "["},
		{SimpleJsonReaderEvent::Null, "null"},
		{SimpleJsonReaderEvent::False, "false"},
		{SimpleJsonReaderEvent::String, "x"},
		{SimpleJsonReaderEvent::EndArray, "]"},
		{SimpleJsonReaderEvent::EndObject, "}"},
	};

	ASSERT_EQ(records.size(), expected.size());
	for (std::size_t i = 0; i < expected.size(); ++i) {
		EXPECT_EQ(records[i].event, expected[i].event);
		EXPECT_EQ(records[i].value, expected[i].value);
	}
}

TEST(SimpleJsonReaderTest, PreservesEscapedQuotesAndBackslashes)
{
	std::vector<EventRecord> records;

	parseJson(R"({"message":"a\"b\\c"})", [&](SimpleJsonReaderEvent event, std::string_view value) {
		records.push_back({event, std::string(value)});
	});

	const std::vector<EventRecord> expected = {
		{SimpleJsonReaderEvent::StartObject, "{"},
		{SimpleJsonReaderEvent::Key, "message"},
		{SimpleJsonReaderEvent::String, R"(a\"b\\c)"},
		{SimpleJsonReaderEvent::EndObject, "}"},
	};

	ASSERT_EQ(records.size(), expected.size());
	for (std::size_t i = 0; i < expected.size(); ++i) {
		EXPECT_EQ(records[i].event, expected[i].event);
		EXPECT_EQ(records[i].value, expected[i].value);
	}
}
