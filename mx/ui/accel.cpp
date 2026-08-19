#include "mx/ui/types.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace mx::ui {
namespace {

std::string TrimLower(std::string s) {
  auto not_space = [](unsigned char c) { return !std::isspace(c); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
  s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
  for (char& c : s) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return s;
}

bool TokenToVk(const std::string& tok, UINT* vk) {
  if (!vk || tok.empty()) {
    return false;
  }
  if (tok == "esc" || tok == "escape") {
    *vk = VK_ESCAPE;
    return true;
  }
  if (tok == "enter" || tok == "return") {
    *vk = VK_RETURN;
    return true;
  }
  if (tok.size() >= 2 && tok[0] == 'f') {
    char* end = nullptr;
    const long n = std::strtol(tok.c_str() + 1, &end, 10);
    if (end && *end == '\0' && n >= 1 && n <= 24) {
      *vk = static_cast<UINT>(VK_F1 + (n - 1));
      return true;
    }
  }
  if (tok.size() == 1) {
    const unsigned char c = static_cast<unsigned char>(tok[0]);
    if (std::isalnum(c)) {
      *vk = static_cast<UINT>(std::toupper(c));
      return true;
    }
  }
  return false;
}

}  // namespace

bool ParseKeyChord(const std::string& spec, KeyChord* out) {
  if (!out) {
    return false;
  }
  KeyChord chord;
  bool have_vk = false;
  std::string token;
  std::istringstream in(spec);
  while (std::getline(in, token, '+')) {
    const std::string t = TrimLower(std::move(token));
    if (t.empty()) {
      continue;
    }
    if (t == "ctrl" || t == "control") {
      chord.ctrl = true;
      continue;
    }
    if (t == "alt") {
      chord.alt = true;
      continue;
    }
    if (t == "shift") {
      chord.shift = true;
      continue;
    }
    if (have_vk) {
      return false;
    }
    if (!TokenToVk(t, &chord.vk)) {
      return false;
    }
    have_vk = true;
  }
  if (!have_vk || !chord.IsShortcut()) {
    return false;
  }
  *out = chord;
  return true;
}

}  // namespace mx::ui
