#pragma once

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

#include "./common.hpp"

using KeyValueMap = std::unordered_map<std::string, std::string>;

constexpr char COMMENT_CHAR = '#';
constexpr char QUOTE_CHAR = '"';
constexpr char ASSIGN_CHAR = '=';
constexpr std::string_view HOME_VAR = "$HOME";
constexpr std::string_view TRIM_CHARS = " \t\r\n";

static std::string_view trim(std::string_view sv) {
    const std::size_t start = sv.find_first_not_of(TRIM_CHARS);
    if (start == std::string_view::npos) {
        return {};
    }

    const std::size_t end = sv.find_last_not_of(TRIM_CHARS);
    return sv.substr(start, end - start + 1);
}

static bool try_parse_line(std::string_view line, std::string_view& key, std::string_view& value) {
    line = trim(line);
    if (line.empty() || line.front() == COMMENT_CHAR) {
        return false;
    }

    const std::size_t assign_pos = line.find(ASSIGN_CHAR);
    if (assign_pos == std::string_view::npos) {
        return false;
    }

    key = trim(line.substr(0, assign_pos));
    const std::string_view raw_value = trim(line.substr(assign_pos + 1));

    // value must be wrapped in double quotes
    if (raw_value.size() < 2 || raw_value.front() != QUOTE_CHAR || raw_value.back() != QUOTE_CHAR) {
        return false;
    }

    value = raw_value.substr(1, raw_value.size() - 2);
    return !key.empty();
}

static bool is_var_start_char(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_';
}

static bool is_var_char(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
}

namespace kv_parser {
    inline KeyValueMap parse(std::string_view content) {
        KeyValueMap result;

        std::size_t line_start = 0;
        while (line_start <= content.size()) {
            const std::size_t line_end = content.find('\n', line_start);
            const std::size_t line_stop = (line_end == std::string_view::npos) ? content.size() : line_end;

            std::string_view key;
            std::string_view value;
            if (try_parse_line(content.substr(line_start, line_stop - line_start), key, value)) {
                result[std::string(key)] = std::string(value);
            }

            if (line_end == std::string_view::npos) {
                break;
            }

            line_start = line_end + 1;
        }

        return result;
    }

    inline KeyValueMap parse_file(const fs::path& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return {};
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();
        return parse(buffer.str());
    }

    [[nodiscard]] inline std::string resolve_path(std::string_view path) {
        std::string result;
        result.reserve(path.size());

        std::size_t i = 0;
        while (i < path.size()) {
            if (path[i] != '$') {
                result += path[i];
                ++i;
                continue;
            }

            const bool braced = i + 1 < path.size() && path[i + 1] == '{';
            std::size_t name_start = i + 1 + (braced ? 1 : 0);

            std::size_t name_end = name_start;
            if (name_end < path.size() && is_var_start_char(path[name_end])) {
                ++name_end;
                while (name_end < path.size() && is_var_char(path[name_end])) {
                    ++name_end;
                }
            }

            const bool has_name = name_end > name_start;
            const bool brace_closed = !braced || (name_end < path.size() && path[name_end] == '}');

            if (!has_name || !brace_closed) {
                result += path[i];
                ++i;
                continue;
            }

            const std::size_t reference_end = braced ? name_end + 1 : name_end;
            const std::string var_name(path.substr(name_start, name_end - name_start));
            const char* env_value = std::getenv(var_name.c_str());

            result += (env_value != nullptr) ? std::string_view(env_value) : path.substr(i, reference_end - i);
            i = reference_end;
        }

        return result;
    }
} // namespace kv_parser
