#include "parser.h"
#include <array>
#include <functional>
#include <fstream>
#include <sstream>
#include <utility>

#define ERR(pos)  "At " << line_pos << ":" << pos << ". "

namespace {
constexpr static auto HexToDec = ([] {
  std::array<uint8_t, omfl::SIZE> res{};

  for (int c = 0; c < omfl::SIZE; ++c) {
    res[c] = omfl::SIZE - 1;
  }

  for (int c = '0'; c <= '9'; ++c) {
    res[c] = c - '0';
  }

  for (int c = 'A'; c <= 'Z'; ++c) {
    res[c] = c - 'A';
    res[c | omfl::LOW_TO_UP_CASE] = c - 'A';
  }

  return res;
})();

constexpr static auto CorrectName = ([] {
  std::array<bool, omfl::SIZE> res{};

  for (size_t c = 0; c < omfl::SIZE; ++c) {
    res[c] = ('a' <= (c | omfl::LOW_TO_UP_CASE) && (c | omfl::LOW_TO_UP_CASE) <= 'z')
        || ('0' <= c && c <= '9');
  }

  res['-'] = true;
  res['_'] = true;

  return res;
})();

constexpr static auto CorrectSection = ([] {
  auto res = CorrectName;
  res['.'] = 1;
  return res;
})();

std::vector<std::string> split_path(const std::string_view &s) {
  std::vector<std::string> res;
  size_t prev_pos = 0;

  for (size_t pos = 0; pos < s.size(); ++pos) {
    if (s[pos] == '.') {
      res.emplace_back(s.substr(prev_pos, pos - prev_pos));
      prev_pos = pos + 1;
    }
  }

  res.emplace_back(s.substr(prev_pos));

  return res;
}

auto lower_bnd =
    [](std::function<bool(int)> pred, const std::string_view &sw, size_t pos, size_t right_border) {
  while (pos < right_border && pred(sw[pos])) {
    ++pos;
  }

  return pos;
};
}

namespace omfl {
namespace {
TParser::TValue deflt;
TParser defltp;
}
TParser::TValue::exception::exception(const std::string &msg, size_t pos) :
std::runtime_error(msg), pos(pos) {}

TParser::TValue::TValue() = default;

const TParser::TValue &TParser::TValue::operator[](size_t pos) const {
  if (!IsArray()) {
    throw std::runtime_error("Value is not an array");
  }
  auto arr = std::get<arr_val>(value);
  if (pos < arr->size()) {
    return arr->at(pos);
  }

  return deflt;
}

bool TParser::TValue::IsNothing() const {
  return value_type == VNothing;
}

bool TParser::TValue::IsInt() const {
  return value_type == VInt;
}

bool TParser::TValue::IsArray() const {
  return value_type == VArray;
}

bool TParser::TValue::IsString() const {
  return value_type == VString;
}

bool TParser::TValue::IsFloat() const {
  return value_type == VFloating;
}

bool TParser::TValue::IsBool() const {
  return value_type == VBoolean;
}

bool TParser::TValue::AsBool() const {
  if (!IsBool()) {
    throw std::runtime_error("Value is not a bool");
  }

  return std::get<bool>(value);
}

bool TParser::TValue::AsBoolOrDefault(bool b) const {
  if (!IsBool()) {
    return b;
  }

  return std::get<bool>(value);
}

const std::string &TParser::TValue::AsString() const {
  if (!IsString()) {
    throw std::runtime_error("Value is not a string");
  }

  return *std::get<str_ptr>(value);
}

const std::string &TParser::TValue::AsStringOrDefault(const std::string &s) const {
  if (!IsString()) {
    return s;
  }

  return *std::get<str_ptr>(value);
}

int TParser::TValue::AsInt() const {
  if (!IsInt()) {
    throw std::runtime_error("Value is not an integer");
  }

  return std::get<int>(value);
}

int TParser::TValue::AsIntOrDefault(int i) const {
  if (!IsInt()) {
    return i;
  }

  return std::get<int>(value);
}

float TParser::TValue::AsFloat() const {
  if (!IsFloat()) {
    throw std::runtime_error("Value is not an integer");
  }

  return std::get<float>(value);
}

float TParser::TValue::AsFloatOrDefault(float f) const {
  if (!IsFloat()) {
    return f;
  }

  return std::get<float>(value);
}

void TParser::TValue::parseIntOrFloat(const std::string_view &sw, size_t &pos) {
  int sign = 1;

  while (pos < sw.size() && (sw[pos] == '+' || sw[pos] == '-')) {
    if (sw[pos++] == '-') {
      sign = -sign;
    }
  }

  int val = 0;
  if (pos == sw.size() || !isdigit(sw[pos])) {
    throw exception("Excepted digit, but found EOL/incorrect char", pos);
  }

  while (pos < sw.size() && isdigit(sw[pos])) {
    val = val * 10 + sw[pos++] - '0';
  }

  bool is_float = 0;
  float q = 0, pow = 0.1f;

  if (pos < sw.size() && sw[pos] == '.') {
    is_float = 1;
    ++pos;
  }

  if (is_float) {
    if (pos == sw.size() || !isdigit(sw[pos])) {
      throw exception("Excepted digit", pos);
    }

    while (pos < sw.size() && isdigit(sw[pos])) {
      q += pow * (sw[pos++] - '0');
      pow *= 0.1f;
    }
    value = sign * (q + val);
    value_type = VFloating;
  } else {
    value = sign * val;
    value_type = VInt;
  }

}

TParser::TValue::TValue(std::string_view &line) {
  size_t pos = 0;
  while (pos < line.size() && isspace(line[pos])) {
    ++pos;
  }

  if (pos == line.size()) {
    throw exception(R"(Excepted ["\[+\-0-9], but found EOL)", pos);
  }

  switch (line[pos]) {
    case '[': {
      arr_val res = std::make_shared<std::vector<TValue>>();
      size_t last_pos = pos + 1;
      auto sw = line.substr(last_pos);

      auto skip_spaces = [&]() {
        last_pos = line.size() - sw.size();
        while (last_pos < line.size() && isspace(line[last_pos])) {
          ++last_pos;
        }
        sw = line.substr(last_pos);
      };

      while (true) {
        skip_spaces();
        if (sw.empty()) {
          throw exception("Excepted value but found EOL", last_pos);
        }

        if (sw[0] == ']') {
          break;
        }

        try {
          res->emplace_back(sw);
        }
        catch (const exception &ex) {
          throw exception(ex.what(), ex.pos + last_pos);
        }

        skip_spaces();

        if (sw.empty()) {
          throw exception("Excepted , or ], but found EOL", last_pos);
        }

        if (sw[0] == ']') {
          break;
        }

        if (sw[0] == ',') {
          sw = sw.substr(1);
        }
      }

      line = line.substr(last_pos + 1);
      value_type = VArray;
      value = res;
      break;
    }
    case '"': {
      auto ptr = std::make_shared<std::string>();
      ++pos;

      while (pos < line.size()) {
        if (line[pos] == '\\') {
          if (++pos == line.size()) {
            throw exception("Excepted escaping char, but found EOL", pos + 1);
          }
          ptr->push_back(escape(line, pos));
          continue;
        } else if (line[pos] == '\"') {
          value = ptr;
          value_type = VString;
          line = line.substr(pos + 1);
          break;
        }
        ptr->push_back(line[pos++]);
      }
    }
      break;
    case 't':
      if (line.substr(pos, 4) != "true") {
        throw exception("Excepted right value ", pos + 1);
      }
      value = true;
      value_type = VBoolean;
      line = line.substr(pos + 4);
      return;
    case 'f':
      if (line.substr(pos, 5) != "false") {
        throw exception("Excepted right value ", pos + 1);
      }
      value = false;
      value_type = VBoolean;
      line = line.substr(pos + 5);
      return;
    case '+':
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':parseIntOrFloat(line, pos);
      line = line.substr(pos);
      break;
    default:throw exception(R"(Excepted ["\[+\-0-9], but found )" + line[pos], pos);
  }
}

char TParser::TValue::escape(const std::string_view &sw, size_t &at) {
  switch (sw[at]) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7': {
      if (at == sw.size()) {
        throw exception("Excepted char to escape, but found EOL", at);
      }

      uint8_t val = sw[at++] - '0';
      uint8_t cnt = 1;

      while (cnt < 3 && at < sw.size() && '0' <= sw[at] && sw[at] <= '7') {
        val = (val << 3) | (sw[at] - '0');
        ++at;
        ++cnt;
      }

      return val;
    }
    case 'x': {
      ++at;
      if (at == sw.size()) {
        throw exception("Excepted char to escape, but found EOL", at);
      }

      uint8_t val = HexToDec[sw[at]];

      if (val == 0xFF) {
        throw exception("Excepted [0-9A-Fa-f] to escape, but found " + sw[at], at);
      }

      ++at;
      uint8_t cnt = 1;

      while (cnt < 2 && at < sw.size() && HexToDec[sw[at]] != 0xFF) {
        val = (val << 4) | HexToDec[sw[at]];
        ++cnt;
        ++at;
      }

      return val;
    }
    case '\'':
    case '\"':
    case '\\':
    case '?':return sw[at++];
    case 'a':++at;
      return '\a';
    case 't':++at;
      return '\t';
    case 'b':++at;
      return '\b';
    case 'f':++at;
      return '\f';
    case 'n':++at;
      return '\n';
    case 'r':return '\r';
    case 'v':++at;
      return '\v';
    default:throw exception("Excepted char to escape, but found " + sw[at], at);

  }
}

TParser::TValue::TValue(const TParser::TValue &other) noexcept {
  value_type = other.value_type;
  value = other.value;
}

TParser::TValue::TValue(TParser::TValue &&other) noexcept {
  swap(other);
}

TParser::TValue &TParser::TValue::operator=(const TParser::TValue &other) noexcept {
  TParser::TValue tmp(other);
  swap(tmp);

  return *this;
}

void TParser::TValue::swap(TParser::TValue &other) noexcept {
  std::swap(value, other.value);
  std::swap(value_type, other.value_type);
}

TParser::TValue &TParser::TValue::operator=(TParser::TValue &&other) noexcept {
  swap(other);

  return *this;
}

TParser::TParser() noexcept = default;

TParser::TParser(std::istream &is) {
  TParser *current_parser = this;
  std::string tmp;

  size_t line_pos = 0;

  while (is_valid && std::getline(is, tmp)) {
    ++line_pos;

    size_t pos = lower_bnd(isspace, tmp, 0, tmp.size());

    if (pos == tmp.size()) {
      continue;
    }

    if (tmp[pos] == '[') {
      TParser *new_ptr = this;

      for (const auto &sub : parse_section(line_pos, tmp.substr(pos))) {
        if (sub.empty()) {
          is_valid = false;
          break;
        }

        auto it = new_ptr->subpaths.find(sub);
        if (it == new_ptr->subpaths.end()) {
          new_ptr = &(new_ptr->subpaths[sub] = TParser());
          continue;
        }
        new_ptr = &it->second;
      }
      current_parser = new_ptr;
      continue;
    }

    auto [key, value] = parse_key_value(line_pos, tmp);

    if (value.IsNothing()) {
      continue;
    }

    if (!is_valid) {
      break;
    }

    if (current_parser->subpaths.count(key)) {
      is_valid = false;
      break;
    }

    current_parser->subpaths[key] = TParser(*this, value);
  }
}

std::vector<std::string> TParser::parse_section(size_t line_pos, const std::string_view &line) {
  if (line.back() != ']') {
    is_valid = false;
    errors << ERR(line.size()) << "Excepted ], but found" << line.back() << std::endl;

    return {};
  }

  for (size_t pos = 1; pos + 1 < line.size(); ++pos) {
    if (!CorrectSection[line[pos]]) {
      is_valid = false;
      errors << ERR(pos + 1) << "Excepted [A-z0-9_-.], but found" << line[pos] << std::endl;
      return {};
    }
  }

  if (line[line.size() - 2] == '.') {
    is_valid = false;
    errors << ERR(line.size() - 1) << "Excepted subname, but found ]" << std::endl;

    return {};
  }

  return split_path(line.substr(1, line.size() - 2));
}

std::pair<std::string, TParser::TValue> TParser::parse_key_value(size_t line_pos, const std::string &line) {
  size_t pos = 0;

  // Skip spaces, check for comment
  pos = lower_bnd(isspace, line, pos, line.size());

  if (pos == line.size()) {
    return {};
  }

  if (line[pos] == '#') {
    return {};
  }

  size_t eq_pos = line.find('=');

  if (eq_pos == std::string::npos) {
    is_valid = false;
    errors << ERR(1) << "Excepted = in line, but not found. " << std::endl;

    return {};
  }

  // Skip spaces
  pos = lower_bnd(isspace, line, pos, eq_pos);

  size_t start_pos = pos;
  // Parse name
  pos = lower_bnd([](int x) { return CorrectName[x]; }, line, pos, eq_pos);

  std::string name = line.substr(start_pos, pos - start_pos);

  if (name.empty()) {
    is_valid = false;
    errors << ERR(pos) << "Excepted name, but found nothing " << std::endl;
  }

  // Skip spaces
  pos = lower_bnd(isspace, line, pos, eq_pos);

  // Now pos must be equal to eq_pos
  if (pos != eq_pos) {
    is_valid = false;
    errors << ERR(pos + 1) << "Unexcepted character " << line[pos] << std::endl;

    return {};
  }

  try {
    std::string right_part = line.substr(eq_pos + 1);
    std::string_view sw = right_part;
    TValue value(sw);
    sw = sw.substr(lower_bnd(isspace, sw, 0, sw.size()));

    if (!sw.empty() && sw[0] == '#') {
      sw = sw.substr(sw.size());
    }

    if (!sw.empty()) {
      is_valid = false;
      errors << ERR(line.size() - sw.size()) << "Excepted EOL, but found " << sw[0] << std::endl;

      return {};
    }

    return {name, value};
  }
  catch (const TValue::exception &err) {
    is_valid = false;
    errors << ERR(err.pos + 1 + eq_pos + 1) << err.what() << std::endl;

    return {};
  }
}

TParser::TParser(const TParser &p, TParser::TValue val) : is_value(true), property(std::move(val)) {
}

TParser::TParser(const TParser &p) noexcept {
  is_value = p.is_value;
  subpaths = p.subpaths;
  property = p.property;
  is_valid = p.is_valid;
  errors << p.errors.str();
}

TParser::TParser(TParser &&p) noexcept {
  swap(p);
}

TParser &TParser::operator=(const TParser &other) noexcept {
  TParser tmp(other);
  tmp.swap(*this);

  return *this;
}

TParser &TParser::operator=(TParser &&other) noexcept {
  swap(other);

  return *this;
}

TParser::~TParser() noexcept = default;

const TParser &TParser::Get(const std::string &path) const {
  const TParser *ptr = this;

  for (const auto &sub : split_path(path)) {
    auto it = ptr->subpaths.find(sub);
    if (it == ptr->subpaths.end()) {
      throw defltp;
    }
    ptr = &it->second;
  }

  return *ptr;
}

void TParser::swap(TParser &p) noexcept {
  std::swap(subpaths, p.subpaths);
  std::swap(is_valid, p.is_valid);
  std::swap(is_value, p.is_value);
  std::swap(property, p.property);
  std::swap(errors, p.errors);
}

bool TParser::valid() const {
  return is_valid;
}

bool TParser::IsNothing() const {
  if (!is_value) {
    throw std::runtime_error("It isn't a property.");
  }

  return property.IsNothing();
}

bool TParser::IsInt() const {
  if (!is_value) {
    throw std::runtime_error("It isn't a property.");
  }

  return property.IsInt();
}

bool TParser::IsArray() const {
  if (!is_value) {
    throw std::runtime_error("It isn't a property.");
  }

  return property.IsArray();
}

bool TParser::IsString() const {
  if (!is_value) {
    throw std::runtime_error("It isn't a property.");
  }

  return property.IsString();
}

bool TParser::IsFloat() const {
  if (!is_value) {
    throw std::runtime_error("It isn't a property.");
  }

  return property.IsFloat();
}

bool TParser::AsBool() const {
  if (!is_value) {
    throw std::runtime_error("It isn't a property.");
  }

  return property.AsBool();
}

bool TParser::AsBoolOrDefault(bool b) const {
  if (!is_value) {
    return b;
  }

  return property.AsBoolOrDefault(b);
}

const std::string &TParser::AsString() const {
  if (!is_value) {
    throw std::runtime_error("It isn't a property.");
  }

  return property.AsString();
}

const std::string &TParser::AsStringOrDefault(const std::string &s) const {
  if (!is_value) {
    return s;
  }

  return property.AsStringOrDefault(s);
}

int TParser::AsInt() const {
  if (!is_value) {
    throw std::runtime_error("It isn't a property.");
  }

  return property.AsInt();
}

int TParser::AsIntOrDefault(int i) const {
  if (!is_value) {
    return i;
  }

  return property.AsIntOrDefault(i);
}

float TParser::AsFloat() const {
  if (!is_value) {
    throw std::runtime_error("It isn't a property.");
  }

  return property.AsFloat();
}

float TParser::AsFloatOrDefault(float f) const {
  if (!is_value) {
    return f;
  }

  return property.AsFloatOrDefault(f);
}

const TParser::TValue &TParser::operator[](size_t pos) const {
  if (!is_value) {
    throw std::runtime_error("It isn't a property.");
  }

  return property[pos];
}

TParser parse(std::istream &is) {
  return TParser(is);
}

TParser parse(const std::filesystem::path &path) {
  std::ifstream file(path);

  return parse(file);
}

TParser parse(const std::string &str) {
  std::stringstream ss;
  ss << str;

  return parse(ss);
}
} // namespace
