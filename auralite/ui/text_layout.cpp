#include "auralite/ui/text_layout.h"

#include "auralite/canvas.h"

namespace auralite::ui {
namespace {

const wchar_t* FontOrDefault(const wchar_t* font_family) {
  return font_family && font_family[0] ? font_family : L"Microsoft YaHei UI";
}

float WidthOf(const std::wstring& text, float font_size,
              const wchar_t* font_family) {
  return auralite::MeasureUiTextWidth(text, font_size, FontOrDefault(font_family));
}

void WrapParagraph(const std::wstring& para, float max_w, float font_size,
                   const wchar_t* font_family,
                   std::vector<std::wstring>* lines) {
  if (para.empty()) {
    lines->push_back(L"");
    return;
  }
  if (max_w <= 1.f) {
    lines->push_back(para);
    return;
  }

  size_t i = 0;
  while (i < para.size()) {
    size_t end = i + 1;
    size_t last_fit = i + 1;
    while (end <= para.size()) {
      const float w =
          WidthOf(para.substr(i, end - i), font_size, font_family);
      if (w <= max_w) {
        last_fit = end;
        if (end == para.size()) {
          break;
        }
        ++end;
      } else {
        break;
      }
    }
    if (last_fit == i) {
      last_fit = i + 1;
    }
    lines->push_back(para.substr(i, last_fit - i));
    i = last_fit;
  }
}

std::wstring EllipsizeEnd(const std::wstring& text, float max_w,
                          float font_size, const wchar_t* font_family) {
  const std::wstring ell(1, kEllipsis);
  if (WidthOf(ell, font_size, font_family) > max_w) {
    return L"";
  }
  size_t lo = 0;
  size_t hi = text.size();
  size_t best = 0;
  while (lo <= hi) {
    const size_t mid = lo + (hi - lo) / 2;
    if (WidthOf(text.substr(0, mid) + ell, font_size, font_family) <= max_w) {
      best = mid;
      if (mid == text.size()) {
        break;
      }
      lo = mid + 1;
    } else {
      if (mid == 0) {
        break;
      }
      hi = mid - 1;
    }
  }
  return text.substr(0, best) + ell;
}

std::wstring EllipsizeStart(const std::wstring& text, float max_w,
                            float font_size, const wchar_t* font_family) {
  const std::wstring ell(1, kEllipsis);
  if (WidthOf(ell, font_size, font_family) > max_w) {
    return L"";
  }
  size_t lo = 0;
  size_t hi = text.size();
  size_t best = 0;
  while (lo <= hi) {
    const size_t mid = lo + (hi - lo) / 2;
    if (WidthOf(ell + text.substr(text.size() - mid), font_size,
                font_family) <= max_w) {
      best = mid;
      if (mid == text.size()) {
        break;
      }
      lo = mid + 1;
    } else {
      if (mid == 0) {
        break;
      }
      hi = mid - 1;
    }
  }
  return ell + text.substr(text.size() - best);
}

std::wstring EllipsizeMiddle(const std::wstring& text, float max_w,
                             float font_size, const wchar_t* font_family) {
  const std::wstring ell(1, kEllipsis);
  if (WidthOf(ell, font_size, font_family) > max_w) {
    return L"";
  }
  size_t left = 0;
  size_t right = 0;
  bool take_left = true;
  const size_t n = text.size();
  while (left + right < n) {
    size_t next_left = left;
    size_t next_right = right;
    if (take_left) {
      ++next_left;
    } else {
      ++next_right;
    }
    if (next_left + next_right > n) {
      break;
    }
    const std::wstring cand = text.substr(0, next_left) + ell +
                              text.substr(n - next_right);
    if (WidthOf(cand, font_size, font_family) <= max_w) {
      left = next_left;
      right = next_right;
      take_left = !take_left;
      continue;
    }
    if (take_left) {
      take_left = false;
      continue;
    }
    break;
  }
  return text.substr(0, left) + ell + text.substr(n - right);
}

}  // namespace

void WrapUiText(const std::wstring& text, float max_w, float font_size,
                const wchar_t* font_family, std::vector<std::wstring>* lines) {
  if (!lines) {
    return;
  }
  lines->clear();
  size_t start = 0;
  for (size_t i = 0; i < text.size(); ++i) {
    if (text[i] == L'\n') {
      WrapParagraph(text.substr(start, i - start), max_w, font_size,
                    font_family, lines);
      start = i + 1;
    }
  }
  WrapParagraph(text.substr(start), max_w, font_size, font_family, lines);
  if (lines->empty()) {
    lines->push_back(L"");
  }
}

std::wstring EllipsizeUiText(const std::wstring& text, float max_w,
                             float font_size, const wchar_t* font_family,
                             TextTrim trim) {
  if (trim == TextTrim::Clip || text.empty()) {
    return text;
  }
  if (max_w <= 0.f) {
    return L"";
  }
  if (WidthOf(text, font_size, font_family) <= max_w) {
    return text;
  }
  switch (trim) {
    case TextTrim::Start:
      return EllipsizeStart(text, max_w, font_size, font_family);
    case TextTrim::Middle:
      return EllipsizeMiddle(text, max_w, font_size, font_family);
    case TextTrim::End:
    default:
      return EllipsizeEnd(text, max_w, font_size, font_family);
  }
}

}  // namespace auralite::ui
