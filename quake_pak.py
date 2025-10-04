#!/usr/bin/env python3
"""
quake_pak.py

Usage:
  # list contents
  python quake_pak.py list pak0.pak

  # extract all (default outdir = pak0/)
  python quake_pak.py extract pak0.pak
  python quake_pak.py extract pak0.pak output_dir/

  # extract a single named file (default outdir = pak0/)
  python quake_pak.py extract-one pak0.pak "gfx/menu/title.lmp"
  python quake_pak.py extract-one pak0.pak "gfx/menu/title.lmp" output_dir/

  # create pak from a folder
  python quake_pak.py create newpak.pak folder_with_files/
"""
import argparse
import os
import struct
from pathlib import Path

PACK_IDENT = b'PACK'
DIR_ENTRY_STRUCT = struct.Struct('<56sii')  # name (56 bytes), filepos (int32), filelen (int32)
HEADER_STRUCT = struct.Struct('<4sii')      # ident, dir_offset, dir_length
DIR_ENTRY_SIZE = DIR_ENTRY_STRUCT.size
NAME_FIELD_LEN = 56

def read_pak_index(fp):
    fp.seek(0)
    header = fp.read(HEADER_STRUCT.size)
    if len(header) != HEADER_STRUCT.size:
        raise ValueError("File too small to be a pak")
    ident, dir_offset, dir_length = HEADER_STRUCT.unpack(header)
    if ident != PACK_IDENT:
        raise ValueError("Not a PACK file (missing 'PACK' ident)")
    if dir_length % DIR_ENTRY_SIZE != 0:
        raise ValueError("Directory length is not a multiple of directory entry size")
    count = dir_length // DIR_ENTRY_SIZE
    entries = []
    fp.seek(dir_offset)
    for _ in range(count):
        raw = fp.read(DIR_ENTRY_SIZE)
        if len(raw) != DIR_ENTRY_SIZE:
            raise ValueError("Unexpected EOF when reading directory entries")
        name_bytes, filepos, filelen = DIR_ENTRY_STRUCT.unpack(raw)
        name = name_bytes.split(b'\x00', 1)[0].decode('ascii', errors='ignore')
        entries.append((name, filepos, filelen))
    return entries

def list_pak(path):
    with open(path, 'rb') as fp:
        entries = read_pak_index(fp)
        print(f"{len(entries)} files in {path}:")
        for name, pos, length in entries:
            print(f"{name:60s}  {length:8d} bytes  @ {pos}")

def extract_all(pak_path, out_dir=None):
    if out_dir is None:
        out_dir = Path(pak_path).with_suffix('')  # pak0.pak → pak0
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    with open(pak_path, 'rb') as fp:
        entries = read_pak_index(fp)
        for name, pos, length in entries:
            fp.seek(pos)
            data = fp.read(length) if length else b''
            out_path = out_dir / Path(name)
            out_path.parent.mkdir(parents=True, exist_ok=True)
            with open(out_path, 'wb') as out_f:
                out_f.write(data)
            print(f"Extracted: {name} -> {out_path}")

def extract_one(pak_path, entry_name, out_dir=None):
    if out_dir is None:
        out_dir = Path(pak_path).with_suffix('')
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    with open(pak_path, 'rb') as fp:
        entries = read_pak_index(fp)
        for name, pos, length in entries:
            if name == entry_name:
                fp.seek(pos)
                data = fp.read(length) if length else b''
                out_path = out_dir / Path(name)
                out_path.parent.mkdir(parents=True, exist_ok=True)
                with open(out_path, 'wb') as out_f:
                    out_f.write(data)
                print(f"Extracted: {name} -> {out_path}")
                return
        raise KeyError(f"Entry not found: {entry_name}")

def _norm_name_for_pak(path: Path, root: Path):
    return path.relative_to(root).as_posix()

def create_pak(pak_path, source_dir):
    pak_path = Path(pak_path)
    source_dir = Path(source_dir)
    if not source_dir.is_dir():
        raise NotADirectoryError(f"{source_dir} is not a directory")

    files = []
    for p in sorted(source_dir.rglob('*')):
        if p.is_file():
            name = _norm_name_for_pak(p, source_dir)
            files.append((p, name))

    with open(pak_path, 'wb') as out:
        out.write(HEADER_STRUCT.pack(PACK_IDENT, 0, 0))  # placeholder header

        dir_entries = []
        for src_path, name in files:
            filepos = out.tell()
            with open(src_path, 'rb') as f:
                data = f.read()
            out.write(data)
            filelen = len(data)
            name_bytes = name.encode('ascii', errors='ignore')
            if len(name_bytes) > NAME_FIELD_LEN:
                print(f"Warning: name too long, truncating: {name}")
                name_bytes = name_bytes[:NAME_FIELD_LEN]
            dir_entries.append((name_bytes, filepos, filelen))

        dir_offset = out.tell()
        for name_bytes, filepos, filelen in dir_entries:
            name_field = name_bytes.ljust(NAME_FIELD_LEN, b'\x00')
            out.write(DIR_ENTRY_STRUCT.pack(name_field, filepos, filelen))

        dir_length = len(dir_entries) * DIR_ENTRY_SIZE

        out.seek(0)
        out.write(HEADER_STRUCT.pack(PACK_IDENT, dir_offset, dir_length))

    print(f"Created {pak_path} with {len(dir_entries)} files (dir @ {dir_offset}, {dir_length} bytes)")

def main():
    ap = argparse.ArgumentParser(prog='quake_pak.py', description='List/extract/create Quake (PACK) .pak files')
    sub = ap.add_subparsers(dest='cmd', required=True)

    p_list = sub.add_parser('list', help='List contents of a pak')
    p_list.add_argument('pak', help='pak file path')

    p_extract = sub.add_parser('extract', help='Extract all files from a pak')
    p_extract.add_argument('pak', help='pak file path')
    p_extract.add_argument('outdir', nargs='?', help='output directory (default: pak filename)')

    p_extract_one = sub.add_parser('extract-one', help='Extract a single entry from pak by name')
    p_extract_one.add_argument('pak', help='pak file path')
    p_extract_one.add_argument('entry', help='entry name inside pak (use forward slashes)')
    p_extract_one.add_argument('outdir', nargs='?', help='output directory (default: pak filename)')

    p_create = sub.add_parser('create', help='Create a pak from a folder (recurses into subdirs)')
    p_create.add_argument('pak', help='output pak path')
    p_create.add_argument('sourcedir', help='source directory to pack')

    args = ap.parse_args()

    if args.cmd == 'list':
        list_pak(args.pak)
    elif args.cmd == 'extract':
        extract_all(args.pak, args.outdir)
    elif args.cmd == 'extract-one':
        extract_one(args.pak, args.entry, args.outdir)
    elif args.cmd == 'create':
        create_pak(args.pak, args.sourcedir)
    else:
        ap.print_help()

if __name__ == '__main__':
    main()
