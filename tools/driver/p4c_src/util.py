# SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
# Copyright 2013-present Barefoot Networks, Inc.
#
# SPDX-License-Identifier: Apache-2.0
import inspect
import os
import sys
from pathlib import Path
from typing import Optional


# get the directory the python program is running from
def get_script_dir(follow_symlinks: bool = True) -> Path:
    # py2exe, PyInstaller, cx_Freeze
    if getattr(sys, "frozen", False):
        path = (
            Path(sys.executable).resolve()
            if follow_symlinks
            else Path(sys.executable)
        )
    else:
        path = Path(inspect.getabsfile(get_script_dir))
        if follow_symlinks:
            path = path.resolve()
    return path.parent


# recursive find, good for developer
def rec_find_bin(cwd: Path, exe: str) -> Optional[Path]:
    candidate = cwd / exe
    if candidate.is_file():
        return candidate
    parent = cwd.resolve().parent
    if parent != cwd and parent != Path(parent.root):
        return rec_find_bin(parent, exe)
    return None


def use_rec_find(exe: str) -> Optional[Path]:
    cwd = Path.cwd()
    return rec_find_bin(cwd, exe)


def getLocalCfg(config: str) -> Optional[Path]:
    """
    Search bottom-up for p4c.site.cfg
    """
    return use_rec_find(config)


# top-down find, good for deployment
def find_bin(exe: str) -> Optional[Path]:
    for pp in os.environ["PATH"].split(":"):
        pp_path = Path(pp)
        if not pp_path.is_dir():
            continue
        for root, dirs, files in os.walk(pp_path):
            if exe in files:
                return Path(root) / exe
    return None


def find_file(directory: str, filename: str, binary: bool = True) -> Path:
    """
    Searches up the directory hierarchy for filename with prefix directory
    starting from the current working directory.
    If directory is an absolute path, just check for the file.
    If binary == true, then check permissions that the file is executable
    """

    def check_file(f: Path) -> bool:
        if f.is_file():
            if binary:
                return os.access(f, os.X_OK)
            return True
        return False

    directory_path = Path(directory)
    filename_path = Path(filename)

    if directory_path.is_absolute():
        executable = (directory_path / filename_path).resolve()
        if check_file(executable):
            return executable

    # find the file up the hierarchy
    dir = get_script_dir().resolve()
    if filename_path.name != str(filename_path):
        directory_path = directory_path / filename_path.parent
        filename_path = Path(filename_path.name)

    while dir != Path(dir.root):
        path_to_file = dir / directory_path
        if path_to_file.is_dir():
            files = os.listdir(path_to_file)
            if filename_path.name in files:
                executable = path_to_file / filename_path.name
                if check_file(executable):
                    return executable
        dir = dir.parent

    print(f"File {filename_path} not found")
    sys.exit(1)
