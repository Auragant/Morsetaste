// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <string>
#include <string_view>

namespace morse {
struct Version { uint32_t major = 0, minor = 0, patch = 0; };
inline int compareVersions(const Version& a, const Version& b) {
    if (a.major != b.major) return a.major < b.major ? -1 : 1;
    if (a.minor != b.minor) return a.minor < b.minor ? -1 : 1;
    if (a.patch != b.patch) return a.patch < b.patch ? -1 : 1;
    return 0;
}
inline std::string versionString(const Version& value) {
    return std::to_string(value.major) + "." + std::to_string(value.minor) + "." + std::to_string(value.patch);
}
inline bool parseVersion(std::string_view text, Version& version) {
    if (text.empty() || text.size() > 64) return false;
    size_t at = text.front() == 'v' ? 1 : 0;
    Version parsed;
    for (uint32_t* part : {&parsed.major, &parsed.minor, &parsed.patch}) {
        const size_t first = at;
        while (at < text.size() && text[at] >= '0' && text[at] <= '9') {
            const uint32_t digit = uint32_t(text[at++] - '0');
            if (*part > (std::numeric_limits<uint32_t>::max() - digit) / 10) return false;
            *part = *part * 10 + digit;
        }
        if (at == first || (at - first > 1 && text[first] == '0')) return false;
        if (part != &parsed.patch && (at == text.size() || text[at++] != '.')) return false;
    }
    if (at < text.size() && text[at] == 'p') ++at; // Historical suffix meant Public.
    if (at != text.size()) return false;
    version = parsed;
    return true;
}

constexpr size_t updateMaxResponseBytes = 512 * 1024;
constexpr unsigned updateMaxJsonDepth = 32;
struct ReleaseInfo {
    std::string tag;
    Version version;
    bool draft = false, prerelease = false;
};
enum class UpdateState { Idle, Checking, Current, Available, Failed };
struct UpdateResult {
    UpdateState state = UpdateState::Idle;
    std::wstring message;
    std::string latestVersion;
};

namespace update_detail {
// Read only the release fields we use; validate and skip the rest without a DOM.
class ReleaseReader {
public:
    explicit ReleaseReader(std::string_view input) : input_(input) {}
    bool parse(ReleaseInfo& result) {
        if (input_.empty() || input_.size() > updateMaxResponseBytes || !take('{')) return false;
        ReleaseInfo parsed;
        bool tagSeen = false, draftSeen = false, prereleaseSeen = false;
        if (!take('}')) {
            do {
                std::string key;
                bool longKey = false;
                if (!string(&key, 64, &longKey) || !take(':')) return false;
                if (!longKey && key == "tag_name") {
                    bool longTag = false;
                    if (tagSeen || !string(&parsed.tag, 64, &longTag) || longTag) return false;
                    tagSeen = true;
                } else if (!longKey && key == "draft") {
                    if (draftSeen || !boolean(parsed.draft)) return false;
                    draftSeen = true;
                } else if (!longKey && key == "prerelease") {
                    if (prereleaseSeen || !boolean(parsed.prerelease)) return false;
                    prereleaseSeen = true;
                } else if (!value(1)) return false;
                if (take('}')) break;
                if (!take(',')) return false;
            } while (true);
        }
        whitespace();
        if (at_ != input_.size() || !tagSeen || !draftSeen || !prereleaseSeen ||
                !parseVersion(parsed.tag, parsed.version)) return false;
        result = parsed;
        return true;
    }
private:
    std::string_view input_;
    size_t at_ = 0;
    void whitespace() {
        while (at_ < input_.size() && (input_[at_] == ' ' || input_[at_] == '\t' ||
                input_[at_] == '\r' || input_[at_] == '\n')) ++at_;
    }
    bool take(char wanted) {
        whitespace();
        if (at_ == input_.size() || input_[at_] != wanted) return false;
        ++at_; return true;
    }
    bool literal(std::string_view wanted) {
        whitespace();
        if (input_.substr(at_, wanted.size()) != wanted) return false;
        at_ += wanted.size(); return true;
    }
    bool boolean(bool& output) {
        if (literal("true")) { output = true; return true; }
        if (literal("false")) { output = false; return true; }
        return false;
    }
    bool hex4(uint32_t& codepoint) {
        codepoint = 0;
        for (int i = 0; i < 4; ++i) {
            if (at_ == input_.size()) return false;
            const unsigned char c = static_cast<unsigned char>(input_[at_++]);
            unsigned digit = 16;
            if (c >= '0' && c <= '9') digit = c - '0';
            else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
            if (digit == 16) return false;
            codepoint = codepoint * 16 + digit;
        }
        return true;
    }
    static void emit(std::string* output, size_t limit, bool* exceeded, uint32_t codepoint) {
        if (!output) return;
        char bytes[4]{};
        size_t count = 0;
        if (codepoint < 0x80) { bytes[0] = char(codepoint); count = 1; }
        else if (codepoint < 0x800) {
            bytes[0] = char(0xc0 | (codepoint >> 6)); bytes[1] = char(0x80 | (codepoint & 63)); count = 2;
        } else if (codepoint < 0x10000) {
            bytes[0] = char(0xe0 | (codepoint >> 12)); bytes[1] = char(0x80 | ((codepoint >> 6) & 63));
            bytes[2] = char(0x80 | (codepoint & 63)); count = 3;
        } else {
            bytes[0] = char(0xf0 | (codepoint >> 18)); bytes[1] = char(0x80 | ((codepoint >> 12) & 63));
            bytes[2] = char(0x80 | ((codepoint >> 6) & 63)); bytes[3] = char(0x80 | (codepoint & 63)); count = 4;
        }
        if (output->size() + count <= limit && (!exceeded || !*exceeded)) output->append(bytes, count);
        else if (exceeded) *exceeded = true;
    }
    bool string(std::string* output = nullptr, size_t limit = 0, bool* exceeded = nullptr) {
        if (!take('"')) return false;
        while (at_ < input_.size()) {
            const unsigned char first = static_cast<unsigned char>(input_[at_++]);
            if (first == '"') return true;
            if (first < 0x20) return false;
            uint32_t codepoint = first;
            if (first == '\\') {
                if (at_ == input_.size()) return false;
                const char escape = input_[at_++];
                switch (escape) {
                case '"': case '\\': case '/': codepoint = static_cast<unsigned char>(escape); break;
                case 'b': codepoint = 8; break;
                case 'f': codepoint = 12; break;
                case 'n': codepoint = 10; break;
                case 'r': codepoint = 13; break;
                case 't': codepoint = 9; break;
                case 'u': {
                    if (!hex4(codepoint)) return false;
                    if (codepoint >= 0xd800 && codepoint <= 0xdbff) {
                        if (input_.substr(at_, 2) != "\\u") return false;
                        at_ += 2;
                        uint32_t low = 0;
                        if (!hex4(low) || low < 0xdc00 || low > 0xdfff) return false;
                        codepoint = 0x10000 + ((codepoint - 0xd800) << 10) + low - 0xdc00;
                    } else if (codepoint >= 0xdc00 && codepoint <= 0xdfff) return false;
                    break;
                }
                default: return false;
                }
            } else if (first >= 0x80) {
                unsigned count = 0;
                uint32_t minimum = 0;
                if (first >= 0xc2 && first <= 0xdf) { count = 1; minimum = 0x80; codepoint = first & 31; }
                else if (first >= 0xe0 && first <= 0xef) { count = 2; minimum = 0x800; codepoint = first & 15; }
                else if (first >= 0xf0 && first <= 0xf4) { count = 3; minimum = 0x10000; codepoint = first & 7; }
                else return false;
                for (unsigned i = 0; i < count; ++i) {
                    if (at_ == input_.size()) return false;
                    const unsigned char next = static_cast<unsigned char>(input_[at_++]);
                    if ((next & 0xc0) != 0x80) return false;
                    codepoint = (codepoint << 6) | (next & 63);
                }
                if (codepoint < minimum || codepoint > 0x10ffff ||
                        (codepoint >= 0xd800 && codepoint <= 0xdfff)) return false;
            }
            emit(output, limit, exceeded, codepoint);
        }
        return false;
    }
    bool digits() {
        const size_t begin = at_;
        while (at_ < input_.size() && input_[at_] >= '0' && input_[at_] <= '9') ++at_;
        return at_ != begin;
    }
    bool number() {
        if (at_ < input_.size() && input_[at_] == '-') ++at_;
        if (at_ == input_.size()) return false;
        if (input_[at_] == '0') ++at_;
        else if (!digits()) return false;
        if (at_ < input_.size() && input_[at_] == '.') { ++at_; if (!digits()) return false; }
        if (at_ < input_.size() && (input_[at_] == 'e' || input_[at_] == 'E')) {
            ++at_;
            if (at_ < input_.size() && (input_[at_] == '+' || input_[at_] == '-')) ++at_;
            if (!digits()) return false;
        }
        return true;
    }
    bool value(unsigned depth) {
        whitespace();
        if (at_ == input_.size()) return false;
        if (input_[at_] == '"') return string();
        if (input_[at_] == '{' || input_[at_] == '[') {
            if (depth >= updateMaxJsonDepth) return false;
            const bool object = input_[at_++] == '{';
            const char close = object ? '}' : ']';
            if (take(close)) return true;
            do {
                if (object && (!string() || !take(':'))) return false;
                if (!value(depth + 1)) return false;
                if (take(close)) return true;
                if (!take(',')) return false;
            } while (true);
        }
        if (input_[at_] == 't') return literal("true");
        if (input_[at_] == 'f') return literal("false");
        if (input_[at_] == 'n') return literal("null");
        return number();
    }
};
} // namespace update_detail
inline bool parseLatestRelease(std::string_view json, ReleaseInfo& release) {
    return update_detail::ReleaseReader(json).parse(release);
}
} // namespace morse
