#pragma once

#include <string_view>
#include <functional>
#include <vector>
#include <stdexcept>
#include <cctype>

namespace KaitoTokyo::PluginManager {

enum class SimpleJsonReaderEvent {
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

using SimpleJsonReaderHandler = std::function<void(SimpleJsonReaderEvent, std::string_view)>;

namespace SimpleJsonReader {

namespace Detail {

using namespace std::string_view_literals;

SimpleJsonReaderError parseValue(std::string_view *json, SimpleJsonReaderHandler handler)
{
	skipWhitespaces(json);

	if (json->empty())
		return SimpleJsonReaderError::EmptyJSONError;

	switch (json->front()) {
	case '{':
		return parseObject(json, handler);
	case '[':
		return parseArray(json, handler);
	case '\x01': // Sanitized structural '"'
		return parseString(json, handler);
	case 't':
	case 'f':
	case 'n':
		return parseLiteral(json, handler);
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
		return parseNumber(json, handler);
	default:
		return SimpleJsonReaderError::InvalidTokenWhileParsingValueError;
	}
}

SimpleJsonReaderError parseObject(std::string_view *json, SimpleJsonReaderHandler handler)
{
	json->remove_prefix(1);
	handler(SimpleJsonReaderEvent::StartObject, "{"sv);

	while (true) {
		skipWhitespaces(json);
		if (json->empty())
			return SimpleJsonReaderError::UnexpectedEndWhileParsingObjectError;

		if (json->front() == '}') {
			json->remove_prefix(1);
			handler(SimpleJsonReaderEvent::EndObject, "}"sv);
			return SimpleJsonReaderError::OK;
		}

		if (SimpleJsonReaderError err = parseString<true>(json, handler); err != SimpleJsonReaderError::OK)
			return err;

		skipWhitespaces(json);
		if (json->empty())
			return SimpleJsonReaderError::UnexpectedEndWhileParsingObjectError;

		if (json->front() != ':')
			return SimpleJsonReaderError::FieldDelimiterMissingError;
		json->remove_prefix(1);

		if (SimpleJsonReaderError err = parseValue(json, handler); err != SimpleJsonReaderError::OK)
			return err;

		switch (json->front()) {
		case ',':
			json->remove_prefix(1);
			continue;
		case '}':
			json->remove_prefix(1);
			handler(SimpleJsonReaderEvent::EndObject, "}"sv);
			return SimpleJsonReaderError::OK;
		default:
			return SimpleJsonReaderError::InvalidTokenWhileParsingObjectError;
		}
	}
}

SimpleJsonReaderError parseArray(std::string_view *json, SimpleJsonReaderHandler handler)
{
	json->remove_prefix(1);
	handler(SimpleJsonReaderEvent::StartArray, "["sv);

	while (true) {
		skipWhitespaces(json);
		if (json->empty())
			return SimpleJsonReaderError::UnexpectedEndWhileParsingArrayError;

		if (json->front() == ']') {
			json->remove_prefix(1);
			handler(SimpleJsonReaderEvent::EndArray, "]"sv);
			return SimpleJsonReaderError::OK;
		}

		if (SimpleJsonReaderError err = parseValue(json, handler); err != SimpleJsonReaderError::OK)
			return err;

		skipWhitespaces(json);
		if (json->empty())
			return SimpleJsonReaderError::UnexpectedEndWhileParsingArrayError;

		switch (json->front()) {
		case ',':
			json->remove_prefix(1);
			continue;
		case ']':
			json->remove_prefix(1);
			handler(SimpleJsonReaderEvent::EndArray, "]"sv);
			return SimpleJsonReaderError::OK;
		default:
			return SimpleJsonReaderError::InvalidTokenWhileParsingArrayError;
		}
	}
}

template <bool isKey = false>
SimpleJsonReaderError parseString(std::string_view *json, SimpleJsonReaderHandler handler)
{
	std::size_t pos = json->find('\x01', 1); // Sanitized structural '"'
	if (pos == std::string_view::npos) {
		return SimpleJsonReaderError::UnexpectedEndWhileParsingStringError;
	} else {
		handler(isKey ? SimpleJsonReaderEvent::Key : SimpleJsonReaderEvent::String, json->substr(1, pos - 1));
		json->remove_prefix(pos + 1);
		return SimpleJsonReaderError::OK;
	}
}

SimpleJsonReaderError parseNumber(std::string_view *json, SimpleJsonReaderHandler handler)
{
	std::size_t end = json->find_first_of(",]} \t\n\r"sv);
	std::size_t length = (end == std::string_view::npos) ? json->size() : end;
	handler(SimpleJsonReaderEvent::Number, json->substr(0, length));
	json->remove_prefix(length);
	return SimpleJsonReaderError::OK;
}

SimpleJsonReaderError parseLiteral(std::string_view *json, SimpleJsonReaderHandler handler) noexcept
{
	switch (json->front()) {
	case 't':
		if (json->starts_with("true"sv)) {
			json->remove_prefix(4);
			handler(SimpleJsonReaderEvent::True, "true"sv);
			return SimpleJsonReaderError::OK;
		} else {
			return SimpleJsonReaderError::InvalidTokenLikelyTrueError;
		}
	case 'f':
		if (json->starts_with("false"sv)) {
			json->remove_prefix(5);
			handler(SimpleJsonReaderEvent::False, "false"sv);
			return SimpleJsonReaderError::OK;
		} else {
			return SimpleJsonReaderError::InvalidTokenLikelyFalseError;
		}
	case 'n':
		if (json->starts_with("null"sv)) {
			json->remove_prefix(4);
			handler(SimpleJsonReaderEvent::Null, "null"sv);
			return SimpleJsonReaderError::OK;
		} else {
			return SimpleJsonReaderError::InvalidTokenLikelyNullError;
		}
	default:
		return SimpleJsonReaderError::InvalidTokenLikelyLiteralError;
	}
}

void skipWhitespaces(std::string_view *json)
{
	std::size_t first = json->find_first_not_of(" \t\n\r"sv);
	json->remove_prefix((first == std::string_view::npos) ? json->size() : first);
}

} // namespace Detail

void parseJson(std::string jsonString, SimpleJsonReaderHandler handler)
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
	Detail::parseValue(&json, handler);
}

} // namespace SimpleJsonReader

} // namespace KaitoTokyo::PluginManager
