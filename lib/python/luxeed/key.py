from typing import Optional
from dataclasses import dataclass, field
import re
from icecream import ic

LUXEED_NUM_OF_KEYS = 76

@dataclass
class Key:
  id: int = 0              # /* Index into luxeed_device.key_data[* 3]. */
  names: list = field(default_factory=list)        # /* The key name. */
  codes: list = field(default_factory=list)        # /* The ASCII code for this key-press. */
  pos: tuple = (0, 0)             # /* Position relative to the top-leftmost key (~). */
  shift: list = field(default_factory=list)         # /* True if ASCII code is composed with SHIFT and key. */
  mapped: bool = False

key_by_id = [Key(id) for id in range(LUXEED_NUM_OF_KEYS)]
key_by = {}

def key_by_key_id(id: int) -> Optional[Key]:
  if 0 <= id and id < len(key_by_id):
    return key_by_id[id]
  return None

def key_by_code(code: int) -> Optional[Key]:
  return key_by.get(code)

def key_by_position(pos: tuple) -> Optional[Key]:
  return key_by.get(pos)

def key_by_name(name: str) -> Optional[Key]:
  return key_by.get(name)

def key_by_string(s: str) -> Optional[Key]:
  return key_by.get(s)

def key_for(x) -> Optional[Key]:
  if x is None:
    return x
  if isinstance(x, Key):
    return x
  if isinstance(x, int):
    return key_by_id[x]
  return key_by.get(x)

def key_iter():
  return key_by_id

#######################################################

@dataclass
class KeyMap:
  offset: int
  row: int
  col: int
  names: str
  shift: bool = False

key_maps = [
  ( 0 , 0,  0, "`1234567890-=" ),
  ( 0 , 0,  0, "~!@#$%^&*()_+", True ),
  ( 13, 0, 13, ["BACKSPACE=\b"], 0 ),

  ( 14, 1,  0, ["TAB=\t"], 0 ),
  ( 15, 1,  1, "qwertyuiop[]\\", 0 ),
  ( 15, 1,  1, "QWERTYUIOP{}|", True ),

  ( 28, 2,  0, ["CAPS"] ),
  ( 29, 2,  1, "asdfghjkl;'" ),
  ( 29, 2,  1, "ASDFGHJKL:\"", True ),
  ( 40, 2, 12, ["ENTER=\n"] ),

  ( 41, 3,  0, ["LSHIFT"] ),
  ( 42, 3,  1, "zxcvbnm,./" ),
  ( 42, 3,  1, "ZXCVBNM<>?", True ),
  ( 52, 3, 11, ["RSHIFT"] ),

  ( 53, 4,  0, ["LCTRL", "LSTART", "LALT"] ),
  ( 56, 4,  8, ["RCTRL"] ),
  ( 58, 4, 11, ["RALT"] ),
  ( 60, 4,  9, ["RSTART", "MENU"] ),

  ( 67, 0, 15, ["HOME", "PAGE_UP"] ),
  ( 69, 0, 14, ["DEL"]),
  ( 70, 1, 15, ["END", "PAGE_DOWN"] ),
  ( 72, 3, 15, ["UP"] ),
  ( 73, 4, 13, ["LEFT", "DOWN", "RIGHT"]),
  # ( -1, 0 )
]

key_maps = [KeyMap(*vals) for vals in key_maps]
# ic(key_maps)

def key_init():
  for key_map in key_maps:
    # ic(key_map)
    k = key_map.offset
    x = key_map.col
    y = key_map.row
    name_codes = []
    if isinstance(key_map.names, list):
      for name in key_map.names:
        m = re.match(r'([^=]+)(=(.))?', name, re.DOTALL)
        # ic(m)
        # ic(m.groups())
        name_codes.append((m[1], (m[3] and ord(m[3]))))
    else:
      name_codes = [(ch, ord(ch)) for ch in list(key_map.names)]
    # ic(name_codes)
    for name, code in name_codes:
      key = key_by_id[k]
      key.mapped = True
      key.pos = pos = (x, y)
      key.shift.append(key_map.shift)
      key.names.append(name)
      key_by[name] = key_by[pos] = key
      key_by[f'#{k}'] = key_by[f'@{x},{y}'] = key
      if code:
        key.codes.append(code)
        key_by[code] = key_by[f'0x{code:02x}'] = key
      # ic(key)
      x += 1
      k += 1
  # ic(key_by)
  # ic(key_by_id)

key_init()
