#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import re
import os


threshold = 0.1

origin_log = "./origin_log_classification.txt"
log = "./log_classification.txt"
log_dir = "./log/cutoff-5.00/"

# 获取单一的文件
items = {}
cnt = 0

def readfile(filename):
    file = open(filename, 'r')
    for line in file.readlines():
        tmp = re.split(':', line.strip())
        if len(re.split('\s+', tmp[1])) == 1:
            items[tmp[0].strip()] = [tmp[1].strip()]


def getFather(relation, x):
    if relation[x] == x:
        return x
    return getFather(relation, relation[x])

def read_dir(file_dir):
    paths = []
    for root, _, files in os.walk(file_dir):
        for file in files:
            if os.path.splitext(file)[1] == '.txt':
                paths.append(os.path.join(root, file))
    for path in paths:
        print(path)
        citems = set()
        eles = []
        relation = {}

        i = 0
        file = open(path, 'r')
        for line in file.readlines():
            if i == 0:
                cur = line.strip()
                items[cur] = []
                i = 1
            else:
                tmp = re.split('\s+', line.strip())
                eles.append(tmp)
                relation[tmp[0]] = tmp[0]
                relation[tmp[1]] = tmp[1]
        file.close()

        for ele in eles:
            if float(ele[2]) < threshold and float(ele[2]) > 0:
                x = getFather(relation, ele[0])
                y = getFather(relation, ele[1])
                if x == y:
                    continue
                else:
                    relation[y] = x
        for item in relation:
            if getFather(relation, item) == item:
                citems.add(item)
        for item in citems:
            items[cur].append(item)
        print(items[cur])
                

def writefile(filename):
    file = open(filename, 'w')
    cnt = 0
    for key in items:
        cnt += len(items[key])
        file.write(key + ":" + ' '.join(items[key])+"\n")
    print(cnt)

def main():
    readfile(origin_log)
    writefile("./cod_unique_" + str(threshold) + ".txt")

    read_dir(log_dir)
    writefile("./cod_unique_" + str(threshold) + "_all.txt")

if __name__ == "__main__":
    main()

        


