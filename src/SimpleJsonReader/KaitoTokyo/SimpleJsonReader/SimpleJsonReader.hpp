// SPDX-FileCopyrightText: 2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <optional>
#include <string_view>
#include <string>
#include <variant>
#include <vector>

namespace KaitoTokyo::SimpleJsonReader {

enum class SimpleJsonReaderEventType {
	StartObject,
	EndObject,
	StartArray,
	EndArray,
	Key,
	String,
	Number,
	True,
	False,
	Null
};

using SimpleJsonReaderJsonPath = std::variant<std::string_view, std::size_t>;

struct SimpleJsonReaderEvent {
	SimpleJsonReaderEventType type;
	std::string_view value;
	const std::vector<SimpleJsonReaderJsonPath> &jsonPath;
};

enum class SimpleJsonReaderError {
	OK,
	EmptyJSONError,
	UnexpectedEndWhileParsingObjectError,
	UnexpectedEndWhileParsingArrayError,
	UnexpectedEndWhileParsingStringError,
	FieldDelimiterMissingError,
	InvalidTokenWhileParsingValueError,
	InvalidTokenWhileParsingObjectError,
	InvalidTokenWhileParsingArrayError,
	InvalidTokenLikelyTrueError,
	InvalidTokenLikelyFalseError,
	InvalidTokenLikelyNullError,
	InvalidTokenLikelyLiteralError,
};

using SimpleJsonReaderHandler = std::function<void(SimpleJsonReaderEvent)>;

namespace Detail {

using namespace std::string_view_literals;

SimpleJsonReaderError parseValue(std::string_view *json, SimpleJsonReaderHandler handler, std::vector<std::string_view> *jsonPath);

void skipWhitespaces(std::string_view *json) noexcept
{
	std::size_t first = json->find_first_not_of(" \t\n\r"sv);
	json->remove_prefix((first == std::string_view::npos) ? json->size() : first);
}

SimpleJsonReaderError parseArray(std::string_view *json, SimpleJsonReaderHandler handler, std::vector<SimpleJsonReaderJsonPath> *jsonPath)
{
	std::size_t index = 0;

	json->remove_prefix(1);
	handler(SimpleJsonReaderEvent{SimpleJsonReaderEventType::StartArray, "["sv, *jsonPath});
	jsonPath->push_back(index);

	while (true) {
		skipWhitespaces(json);
		if (json->empty())
			return SimpleJsonReaderError::UnexpectedEndWhileParsingArrayError;

		if (json->front() == ']') {
			jsonPath->pop_back();
			json->remove_prefix(1);
			handler(SimpleJsonReaderEvent{SimpleJsonReaderEventType::EndArray, "]"sv, *jsonPath});
			return SimpleJsonReaderError::OK;
		}

		SimpleJsonReaderError err = parseValue(json, handler, jsonPath);
		std::get<std::size_t>(jsonPath->back()) = index;
		if (err != SimpleJsonReaderError::OK)
			return err;

		skipWhitespaces(json);
		if (json->empty())
			return SimpleJsonReaderError::UnexpectedEndWhileParsingArrayError;

		switch (json->front()) {
		case ',':
			index += 1;
			json->remove_prefix(1);
			continue;
		case ']':
			jsonPath->pop_back();
			json->remove_prefix(1);
			handler(SimpleJsonReaderEvent{SimpleJsonReaderEventType::EndArray, "]"sv, *jsonPath});
			return SimpleJsonReaderError::OK;
		default:
			return SimpleJsonReaderError::InvalidTokenWhileParsingArrayError;
		}
	}
}

SimpleJsonReaderError parseString(std::string_view *json, SimpleJsonReaderHandler handler, const std::vector<SimpleJsonReaderJsonPath> &jsonPath)
{
	std::size_t pos = json->find('\x01', 1); // Sanitized structural '"'
	if (pos == std::string_view::npos) {
		return SimpleJsonReaderError::UnexpectedEndWhileParsingStringError;
	} else {
		handler(SimpleJsonReaderEvent{SimpleJsonReaderEventType::String, json->substr(1, pos - 1), jsonPath});
		json->remove_prefix(pos + 1);
		return SimpleJsonReaderError::OK;
	}
}


SimpleJsonReaderError parseKey(std::string_view *json, SimpleJsonReaderHandler handler, std::vector<SimpleJsonReaderJsonPath> *jsonPath)
{
	std::size_t pos = json->find('\x01', 1); // Sanitized structural '"'
	if (pos == std::string_view::npos) {
		return SimpleJsonReaderError::UnexpectedEndWhileParsingStringError;
	} else {
		std::string_view key = json->substr(1, pos - 1);
		jsonPath->push_back(key);
		handler(SimpleJsonReaderEvent{SimpleJsonReaderEventType::Key, key, *jsonPath});
		json->remove_prefix(pos + 1);
		return SimpleJsonReaderError::OK;
	}
}

SimpleJsonReaderError parseNumber(std::string_view *json, SimpleJsonReaderHandler handler, const std::vector<SimpleJsonReaderJsonPath> &jsonPath)
{
	std::size_t end = json->find_first_of(",]} \t\n\r"sv);
	std::size_t length = (end == std::string_view::npos) ? json->size() : end;
	handler(SimpleJsonReaderEvent{SimpleJsonReaderEventType::Number, json->substr(0, length), jsonPath});
	json->remove_prefix(length);
	return SimpleJsonReaderError::OK;
}

SimpleJsonReaderError parseLiteral(std::string_view *json, SimpleJsonReaderHandler handler, const std::vector<SimpleJsonReaderJsonPath> &jsonPath)
{
	switch (json->front()) {
	case 't':
		if (json->starts_with("true"sv)) {
			json->remove_prefix(4);
			handler(SimpleJsonReaderEvent{SimpleJsonReaderEventType::True, "true"sv, jsonPath});
			return SimpleJsonReaderError::OK;
		} else {
			return SimpleJsonReaderError::InvalidTokenLikelyTrueError;
		}
	case 'f':
		if (json->starts_with("false"sv)) {
			json->remove_prefix(5);
			handler(SimpleJsonReaderEvent{SimpleJsonReaderEventType::False, "false"sv, jsonPath});
			return SimpleJsonReaderError::OK;
		} else {
			return SimpleJsonReaderError::InvalidTokenLikelyFalseError;
		}
	case 'n':
		if (json->starts_with("null"sv)) {
			json->remove_prefix(4);
			handler(SimpleJsonReaderEvent{SimpleJsonReaderEventType::Null, "null"sv, jsonPath});
			return SimpleJsonReaderError::OK;
		} else {
			return SimpleJsonReaderError::InvalidTokenLikelyNullError;
		}
	default:
		return SimpleJsonReaderError::InvalidTokenLikelyLiteralError;
	}
}

SimpleJsonReaderError parseObject(std::string_view *json, SimpleJsonReaderHandler handler, std::vector<SimpleJsonReaderJsonPath> *jsonPath)
{
	json->remove_prefix(1);
	handler(SimpleJsonReaderEvent{SimpleJsonReaderEventType::StartObject, "{"sv, *jsonPath});

	while (true) {
		skipWhitespaces(json);
		if (json->empty())
			return SimpleJsonReaderError::UnexpectedEndWhileParsingObjectError;

		if (json->front() == '}') {
			json->remove_prefix(1);
			handler(SimpleJsonReaderEvent{SimpleJsonReaderEventType::EndObject, "}"sv, *jsonPath});
			return SimpleJsonReaderError::OK;
		}

		if (SimpleJsonReaderError err = parseKey(json, handler, jsonPath); err != SimpleJsonReaderError::OK)
			return err;

		skipWhitespaces(json);
		if (json->empty())
			return SimpleJsonReaderError::UnexpectedEndWhileParsingObjectError;

		if (json->front() != ':')
			return SimpleJsonReaderError::FieldDelimiterMissingError;
		json->remove_prefix(1);

		SimpleJsonReaderError err = parseValue(json, handler, jsonPath);
		jsonPath->pop_back();
		if (err != SimpleJsonReaderError::OK)
			return err;

		skipWhitespaces(json);
		if (json->empty()) {
			return SimpleJsonReaderError::UnexpectedEndWhileParsingObjectError;
		}

		switch (json->front()) {
		case ',':
			json->remove_prefix(1);
			continue;
		case '}':
			jsonPath->pop_back();
			json->remove_prefix(1);
			handler(SimpleJsonReaderEvent{SimpleJsonReaderEventType::EndObject, "}"sv, *jsonPath});
			return SimpleJsonReaderError::OK;
		default:
			return SimpleJsonReaderError::InvalidTokenWhileParsingObjectError;
		}
	}
}

SimpleJsonReaderError parseValue(std::string_view *json, SimpleJsonReaderHandler handler, std::vector<SimpleJsonReaderJsonPath> *jsonPath)
{
	skipWhitespaces(json);

	if (json->empty())
		return SimpleJsonReaderError::EmptyJSONError;

	switch (json->front()) {
	case '{':
		return parseObject(json, handler, jsonPath);
	case '[':
		return parseArray(json, handler, jsonPath);
	case '\x01': // Sanitized structural '"'
		return parseString(json, handler, *jsonPath);
	case 't':
	case 'f':
	case 'n':
		return parseLiteral(json, handler, *jsonPath);
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '-':
		return parseNumber(json, handler, *jsonPath);
	default:
		return SimpleJsonReaderError::InvalidTokenWhileParsingValueError;
	}
}

} // namespace Detail

SimpleJsonReaderError parseJson(std::string jsonString, SimpleJsonReaderHandler handler)
{
	std::size_t pos = 0;
	bool inString = false;

	while (true) {
		pos = jsonString.find('"', pos);
		if (pos == std::string::npos)
			break;

		if (inString) {
			std::size_t rpos = pos;

			while (rpos > 0 && jsonString[rpos - 1] == '\\') {
				rpos -= 1;
			}

			if ((pos - rpos) % 2 == 0) {
				jsonString[pos] = '\x01';
				inString = false;
			}
		} else {
			jsonString[pos] = '\x01';
			inString = true;
		}

		pos += 1;
	}

	std::string_view json(jsonString);
	std::vector<SimpleJsonReaderJsonPath> jsonPath;
	return Detail::parseValue(&json, handler, &jsonPath);
}

} // namespace KaitoTokyo::SimpleJsonReader
