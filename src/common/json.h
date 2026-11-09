#pragma once

#include <map>
#include <string>
#include <vector>

// ============================================================
//  极简 JSON（只实现 config.json 需要的子集）
//  支持 null / bool / number / string / array / object
//  字符串统一为 UTF-8
// ============================================================

namespace jsonx {

struct Value {
    enum Type { Null, Bool, Number, String, Array, Object };

    Type type = Null;
    bool boolean = false;
    double number = 0.0;
    std::string str;
    std::vector<Value> array;
    std::map<std::string, Value> object;

    bool is_object() const { return type == Object; }
    bool is_string() const { return type == String; }

    const Value* find(const std::string& key) const {
        if (type != Object) return nullptr;
        auto it = object.find(key);
        return (it == object.end()) ? nullptr : &it->second;
    }

    const Value* at(size_t index) const {
        if (type != Array || index >= array.size()) return nullptr;
        return &array[index];
    }

    std::string as_string() const { return (type == String) ? str : std::string(); }
};

// 解析 UTF-8 JSON 文本
bool parse(const std::string& text, Value& out);

// 把字符串转义成 JSON 字面量（含两侧引号）
std::string quote(const std::string& utf8);

} // namespace jsonx
