# -*- coding: utf-8 -*-
"""Regenerate cmake/LegacySources.cmake from Base-related vcxproj files."""
from __future__ import print_function
import re
from pathlib import Path

root = Path(__file__).resolve().parents[1]


def parse(vcx_rel):
    text = (root / vcx_rel).read_text(encoding="utf-8")
    rels = re.findall(r'<ClCompile Include="([^"]+)"', text)
    base_dir = (root / vcx_rel).parent
    out = []
    for rel in rels:
        abs_path = (base_dir / rel).resolve()
        out.append(abs_path.relative_to(root).as_posix())
    return out


seen = set()
paths = []
for vcx in (
    "base/base.vcxproj",
    "message_framework/message_framework.vcxproj",
    "rfc_algorithm/rfc_algorithm.vcxproj",
):
    if not (root / vcx).exists():
        continue
    for p in parse(vcx):
        if p not in seen:
            seen.add(p)
            paths.append(p)

out = root / "cmake" / "LegacySources.cmake"
lines = [
    "# Auto-generated from vcxproj. Regenerate: python scripts/gen_cmake_sources.py\n",
    "set(MXUI_BASE_SOURCES\n",
]
for p in paths:
    lines.append("  ${MXUI_ROOT}/%s\n" % p)
lines.append(")\n")
out.write_text("".join(lines), encoding="utf-8")
print("base", len(paths), "->", out)
