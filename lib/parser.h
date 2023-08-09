#pragma once

#include <filesystem>
#include <istream>
#include <map>
#include <sstream>
#include <variant>
#include <vector>

namespace omfl {

const int SIZE = 256;

// 0x20 is bit to convert lowercase to uppercase ASCII letter
const int LOW_TO_UP_CASE = 0x20;

class TParser {
 public:
  class TValue {
    enum type { VNothing, VString, VInt, VFloating, VArray, VBoolean };
    using str_ptr = std::shared_ptr<std::string>;
    using arr_val = std::shared_ptr<std::vector<TValue>>;
   public:
    class exception : public std::runtime_error {
     public:
      exception(const std::string& msg, size_t pos);
      size_t pos;
    };
    TValue();
    TValue(std::string_view& line);
    TValue(const TValue&) noexcept;
    TValue(TValue&&) noexcept;
    TValue& operator=(const TValue&) noexcept;
    TValue& operator=(TValue&&) noexcept;
    void swap(TValue&) noexcept;
    bool IsNothing() const;
    bool IsString() const;
    bool IsArray() const;
    bool IsFloat() const;
    bool IsBool() const;
    bool IsInt() const;
    bool AsBool() const;
    bool AsBoolOrDefault(bool b) const;
    const std::string& AsString() const;
    const std::string& AsStringOrDefault(const std::string& s) const;
    float AsFloat() const;
    float AsFloatOrDefault(float f) const;
    int AsInt() const;
    int AsIntOrDefault(int x) const;
    const TValue& operator[](size_t pos) const;
   private:
    static char escape(const std::string_view& sw, size_t& at);
    void parseIntOrFloat(const std::string_view& sw, size_t& pos);
    std::variant<str_ptr, int, float, arr_val, bool> value;
    type value_type = VNothing;
  };
  TParser(std::istream& is);
  TParser(const TParser& p) noexcept;
  TParser(TParser&& p) noexcept;
  TParser& operator=(const TParser& other) noexcept;
  TParser& operator=(TParser&& other) noexcept;
  ~TParser() noexcept;
  const TParser& Get(const std::string& path) const;
  void swap(TParser& p) noexcept;
  bool valid() const;
  const std::string& get_errors() const;

  bool IsNothing() const;
  bool IsString() const;
  bool IsArray() const;
  bool IsFloat() const;
  bool IsBool() const;
  bool IsInt() const;
  bool AsBool() const;
  bool AsBoolOrDefault(bool b) const;
  const std::string& AsString() const;
  const std::string& AsStringOrDefault(const std::string& s) const;
  float AsFloat() const;
  float AsFloatOrDefault(float f) const;
  int AsInt() const;
  int AsIntOrDefault(int x) const;
  const TValue& operator[](size_t pos) const;
  TParser() noexcept;
 private:
  TParser(const TParser& other, TValue  val);
  std::vector<std::string> parse_section(size_t line_pos, const std::string_view& line);
  std::pair<std::string, TValue> parse_key_value(size_t line_pos, const std::string& line);
  std::stringstream errors;
  std::map<std::string, TParser> subpaths;
  TValue property;
  bool is_valid = true;
  bool is_value = false;
};

TParser parse(const std::filesystem::path& path);
TParser parse(const std::string& str);
TParser parse(std::istream& is);

}// namespace
