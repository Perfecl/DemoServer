#!/usr/bin/python3
# coding:utf-8

import json
import os.path
import sys

if len(sys.argv) < 2:
    raise Exception("未传入参数")

path = r'./'
file_name = sys.argv[1]
dir_name = file_name[:file_name.rfind('.')]
file = open(path + file_name)

all_data = {}

for line in file:
    index = line.find(":[")
    if index < 0:
        continue
    end_index = line.find(":[")
    start_index = line[:end_index].rfind(" ")
    datetime_x = 'xx:xx:xx'
    if start_index >= 0:
        start_index += 1
        datetime_x = line[11:19]
    else:
        start_index = 0
    api = line[start_index:end_index]
    try:
        data = json.loads(line[index + 1:])
    except ValueError:
        continue
    if api not in all_data:
        all_data[api] = []
    for row in data:
        str_key = 'datetime'
        str_val = datetime_x
        for key in sorted(row.keys()):
            str_key += ',' + str(key)
            str_val += ',' + str(row[key])
        if len(all_data[api]) <= 0:
            all_data[api].append(str_key + '\n')
        all_data[api].append(str_val + '\n')

for key in all_data:
    if not os.path.isdir(path + dir_name):
        os.mkdir(path + dir_name)
    csv_file = path + dir_name + '/' + dir_name + '_' + key + '.csv'
    with open(csv_file, 'w', encoding='gbk') as csv_file:
        csv_file.writelines(all_data[key])

file.close()
