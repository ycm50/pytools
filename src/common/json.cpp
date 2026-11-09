#include "common/json.h"

#include <cstdlib>
#include <cstring>

namespace jsonx {

namespace {

void append_utf8(std::string& out, unsigned int cp) {
    if (cp <= 0x7F) {
        out += static_cast<char>(cp);
    } else if (cp <= 0x7FF) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp <= 0xFFFF) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

class Parser {
public:
    explicit Parser(const std::string& text) : s_(text) {}

    bool parse_value(Value& out) {
        skip_ws();
        if (pos_ >= s_.size()) return false;
        switch (s_[pos_]) {
            case '{': return parse_object(out);
            case '[': return parse_array(out);
            case '"':
                out.type = Value::String;
                return parse_string(out.str);
            case 't':
            case 'f': return parse_bool(out);
            case 'n': return parse_null(out);
            default: return parse_number(out);
        }
    }

    size_t position() const { return pos_; }

private:
    const std::string& s_;
    size_t pos_ = 0;

    void skip_ws() {
        while (pos_ < s_.size()) {
            char c = s_[pos_];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                ++pos_;
            } else {
                break;
            }
        }
    }

    bool expect(char c) {
        skip_ws();
        if (pos_ < s_.size() && s_[pos_] == c) {
            ++pos_;
            return true;
        }
        return false;
    }

    bool parse_null(Value& out) {
        if (s_.compare(pos_, 4, "null") != 0) return false;
        pos_ += 4;
        out.type = Value::Null;
        return true;
    }

    bool parse_bool(Value& out) {
        if (s_.compare(pos_, 4, "true") == 0) {
            pos_ += 4;
            out.type = Value::Bool;
            out.boolean = true;
            return true;
        }
        if (s_.compare(pos_, 5, "false") == 0) {
            pos_ += 5;
            out.type = Value::Bool;
            out.boolean = false;
            return true;
        }
        return false;
    }

    bool parse_number(Value& out) {
        const char* begin = s_.c_str() + pos_;
        char* end = nullptr;
        double v = std::strtod(begin, &end);
        if (end == begin) return false;
        pos_ += static_cast<size_t>(end - begin);
        out.type = Value::Number;
        out.number = v;
        return true;
    }

    bool parse_hex4(unsigned int& value) {
        if (pos_ + 4 > s_.size()) return false;
        unsigned int v = 0;
        for (int i = 0; i < 4; ++i) {
            int h = hex_value(s_[pos_ + static_cast<size_t>(i)]);
            if (h < 0) return false;
            v = (v << 4) | static_cast<unsigned int>(h);
        }
        pos_ += 4;
        value = v;
        return true;
    }

    bool parse_string(std::string& out) {
        if (!expect('"')) return false;
        out.clear();
        while (pos_ < s_.size()) {
            char c = s_[pos_++];
            if (c == '"') return true;
            if (c != '\\') {
                out += c;
                continue;
            }
            if (pos_ >= s_.size()) return false;
            char esc = s_[pos_++];
            switch (esc) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    unsigned int cp = 0;
                    if (!parse_hex4(cp)) return false;
                    if (cp >= 0xD800 && cp <= 0xDBFF) {
                        if (pos_ + 1 < s_.size() && s_[pos_] == '\\' && s_[pos_ + 1] == 'u') {
                            pos_ += 2;
                            unsigned int low = 0;
                            if (!parse_hex4(low)) return false;
                            if (low >= 0xDC00 && low <= 0xDFFF) {
                                cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                            } else {
                                append_utf8(out, cp);
                                cp = low;
                            }
                        }
                    }
                    append_utf8(out, cp);
                    break;
                }
                default: return false;
            }
        }
        return false;
    }

    bool parse_array(Value& out) {
        if (!expect('[')) return false;
        out.type = Value::Array;
        out.array.clear();
        skip_ws();
        if (pos_ < s_.size() && s_[pos_] == ']') {
            ++pos_;
            return true;
        }
        for (;;) {
            Value item;
            if (!parse_value(item)) return false;
            out.array.push_back(std::move(item));
            skip_ws();
            if (pos_ < s_.size() && s_[pos_] == ',') {
                ++pos_;
                continue;
            }
            return expect(']');
        }
    }

    bool parse_object(Value& out) {
        if (!expect('{')) return false;
        out.type = Value::Object;
        out.object.clear();
        skip_ws();
        if (pos_ < s_.size() && s_[pos_] == '}') {
            ++pos_;
            return true;
        }
        for (;;) {
            skip_ws();
            std::string key;
            if (!parse_string(key)) return false;
            if (!expect(':')) return false;
            Value item;
            if (!parse_value(item)) return false;
            out.object[key] = std::move(item);
            skip_ws();
            if (pos_ < s_.size() && s_[pos_] == ',') {
                ++pos_;
                continue;
            }
            return expect('}');
        }
    }
};

} // namespace

bool parse(const std::string& text, Value& out) {
    Parser p(text);
    if (!p.parse_value(out)) return false;
    return true;
}

std::string quote(const std::string& utf8) {
    std::string out;
    out.reserve(utf8.size() + 2);
    out += '"';
    for (unsigned char c : utf8) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    static const char* hex = "0123456789abcdef";
                    out += "\\u00";
                    out += hex[(c >> 4) & 0x0F];
                    out += hex[c & 0x0F];
                } else {
                    out += static_cast<char>(c);
                }
                break;
        }
    }
    out += '"';
    return out;
}

} // namespace jsonx
