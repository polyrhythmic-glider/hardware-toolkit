from collections import deque
from pathlib import Path
import re
from PIL import Image
ROOT=Path(__file__).resolve().parent.parent
header=(ROOT/'NekoClockAssets.h').read_text()
for value in range(10):
    body=re.search(rf'uint8_t Large{value}\[\] PROGMEM = \{{(.*?)\}};',header,re.S).group(1)
    data=bytes(int(x,16) for x in re.findall(r'0x([0-9a-f]{2})',body))
    pixels={(x,y) for y in range(172) for x in range(80) if data[y*10+x//8]&(1<<(x%8))}
    assert 1000<len(pixels)<10000
    # Digits must be one connected outline, not a set of disjoint LED segments.
    seen=set();queue=deque([next(iter(pixels))]);seen.add(queue[0])
    while queue:
        x,y=queue.popleft()
        for dx,dy in ((-1,0),(1,0),(0,-1),(0,1),(-1,-1),(1,1),(-1,1),(1,-1)):
            p=x+dx,y+dy
            if p in pixels and p not in seen:seen.add(p);queue.append(p)
    assert seen==pixels, f'digit {value} is disconnected'
base=Image.open(ROOT/'assets/pixel-neko/pet-atlas.png')
assert base.size==(160,160) and base.mode=='1'
pixels=list(base.get_flattened_data() if hasattr(base,'get_flattened_data') else base.getdata())
assert set(pixels)=={0,255}
assert 2000<sum(p==0 for p in pixels)<14000
print('Assets: ten connected Orbitron glyphs and ONE immutable 1-bit dithered pet base.')
