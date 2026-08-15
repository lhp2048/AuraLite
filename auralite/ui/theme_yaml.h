#pragma once

#include "auralite/canvas.h"  // ColorF
#include <string>

namespace auralite::ui {

// Returns false on invalid; out unchanged on failure.
// Accepts "#RRGGBB" or "#RRGGBBAA" (alpha last). Leading '#' required.
bool ParseColorHex(const std::string& s, ColorF* out);

}  // namespace auralite::ui
