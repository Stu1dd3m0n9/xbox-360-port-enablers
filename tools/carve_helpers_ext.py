#!/usr/bin/env python3
"""Extend helpers.a with PPC float/int conversion helpers (lua needs __fixdfdi).
Reuses tri/coff2elf archive flow; outputs E:\\360ports\\build\\helpers_float.a.
WANT set covers savres family (kept for completeness) + fix/float/div helpers.
Usage: py -3 carve_helpers_ext.py
"""
import struct, sys
sys.path.insert(0, r"E:\sm64build\tri")
from coff2elf import (read_archive, CoffObject, strong_noncomdat_globals,
                      coff_to_elf, defined_globals, write_archive, member_name,
                      IMAGE_FILE_MACHINE_POWERPCBE)
WANT_PREFIX = ("__savegpr", "__restgpr", "__savefpr", "__restfpr",
               "__savevmx", "__restvmx",
               "__fix", "__float", "__div", "__mod", "__udiv", "__umod",
               "__extend", "__trunc", "__adddf", "__subdf", "__muldf",
               "__divdf", "__addsf", "__subsf", "__mulsf", "__divsf")
WANT_EXACT = {"__u64tod", "_RtlCheckStack12"}
def wanted(syms):
    return [s for s in syms if s in WANT_EXACT or s.startswith(WANT_PREFIX)]
def main():
    import os
    src = r"C:\Program Files\RXDK-360\legacy\lib\libcMT.lib"
    out_a = r"E:\360ports\build\helpers_float.a"
    os.makedirs(os.path.dirname(out_a), exist_ok=True)
    blob = open(src, "rb").read()
    coff_members, noncomdat_strong = [], set()
    for member, longnames in read_archive(blob):
        if member.name in ("/", "//"): continue
        if member.data[:2] != struct.pack("<H", IMAGE_FILE_MACHINE_POWERPCBE): continue
        obj = CoffObject(member.data)
        noncomdat_strong |= strong_noncomdat_globals(obj)
        coff_members.append((member, longnames, obj))
    members, seen, kept = [], {}, []
    for member, longnames, obj in coff_members:
        defs = defined_globals(obj)
        if not wanted(defs): continue
        # skip members already in base helpers.a savres-only set? keep all; link --allow-multiple-definition tolerates dupes
        elf = coff_to_elf(obj, warn=lambda m: None, noncomdat_strong=noncomdat_strong)
        base = member_name(member.name, longnames).replace("\\", "/").split("/")[-1]
        base = base[:-4] if base.endswith(".obj") else base
        n = seen.get(base, 0); seen[base] = n + 1
        uniq = f"{base}.{n}.o" if n else f"{base}.o"
        members.append((uniq, elf, defs)); kept.extend(wanted(defs))
    nsyms = write_archive(members, out_a)
    print(f"{out_a}: {len(members)} objects, {nsyms} indexed symbols")
    for s in sorted(set(kept)): print(f"  provides {s}")
if __name__ == "__main__": main()
