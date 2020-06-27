#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import re
import shutil

error_files = []
pos_file = []
other = []


def readfile(filename):
    file = open(filename, 'r')
    item = ''
    for line in file.readlines():
        if line[0] == '-':
            item = re.split('\s+', line.strip())[1]
        if '[ERROR]' in line:
            # print(item)
            error_files.append(item)


def check():
    for file in error_files:
        f = open(file, 'r')
        for line in f.readlines():
            if '_atom_site_fract_x' in line:
                pos_file.append(file)
                break
        if file not in pos_file:
            other.append(file)


def main():
    parser = argparse.ArgumentParser(description='csd solvent')
    parser.add_argument('filename',
                        help='log file')

    args = parser.parse_args()
    filename = args.filename

    readfile(filename)

    print("Total Error file:", len(error_files))
    check()
    print("Position Error file:", len(pos_file))
    print("Other Error file:", len(other))

    for item in other:
        print(item)


if __name__ == "__main__":
    main()
