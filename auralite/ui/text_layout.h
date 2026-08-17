#pragma once

#include "auralite/ui/types.h"

#include <string>
#include <vector>

namespace auralite::ui {

// U+2026. Used by Label trim start/middle/end.
inline constexpr wchar_t kEllipsis = L'\u2026';

// Soft-wrap |text| to |max_w|. Splits on hard newlines first. Empty paragraphs kept.
void WrapUiText(const std::wstring& text, float max_w, float font_size,
                const wchar_t* font_family, std::vector<std::wstring>* lines);

// Single-line fit. Clip returns |text| unchanged. Start/Middle/End insert
// kEllipsis. If |max_w| cannot fit the ellipsis, returns empty.
std::wstring EllipsizeUiText(const std::wstring& text, float max_w,
                             float font_size, const wchar_t* font_family,
                             TextTrim trim);

}  // namespace auralite::ui
