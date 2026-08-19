#pragma once

#include "mx/canvas.h"  // ColorF
#include <string>

namespace mx::ui {

// Returns false on invalid; out unchanged on failure.
// Accepts "#RRGGBB" or "#RRGGBBAA" (alpha last). Leading '#' required.
bool ParseColorHex(const std::string& s, ColorF* out);

}  // namespace mx::ui
