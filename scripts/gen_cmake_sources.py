# -*- coding: utf-8 -*-
from __future__ import print_function
import re
from pathlib import Path

root = Path(r'd:\Users\mx\Desktop\smart-family\family_win_desktop\3rd-party\AuraLite')


def parse(vcx_rel):
    text = (root / vcx_rel).read_text(encoding='utf-8')
    rels = re.findall(r'<ClCompile Include="([^"]+)"', text)
    base_dir = (root / vcx_rel).parent
    out = []
    for rel in rels:
        abs_path = (base_dir / rel).resolve()
        out.append(abs_path.relative_to(root).as_posix())
    return out


base = parse('AuraLite.Base/AuraLite.Base.vcxproj')
ui = parse('AuraLite.UI/AuraLite.UI.vcxproj')

out = root / 'cmake' / 'LegacySources.cmake'
out.parent.mkdir(parents=True, exist_ok=True)
lines = [
    '# Auto-generated from vcxproj. Regenerate: python scripts/gen_cmake_sources.py\n',
    'set(AURALITE_BASE_SOURCES\n',
]
for p in base:
    lines.append('  ${AURALITE_ROOT}/%s\n' % p)
lines.append(')\n\nset(AURALITE_UI_SOURCES\n')
for p in ui:
    lines.append('  ${AURALITE_ROOT}/%s\n' % p)
lines.append(')\n')
out.write_text(''.join(lines), encoding='utf-8')
print('base', len(base), 'ui', len(ui), '->', out)
