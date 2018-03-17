#!/usr/bin/python3
#coding=utf-8

import re
import pandas
from datetime import timedelta, datetime

time_point = datetime.now() - timedelta(days=1)
today = time_point.strftime("%Y-%m-%d")

path = r"/data/nginx/"
output_path = r"/usr/share/nginx/html/report/"

log_file_name = path + 'access_%s.log' % today
csv_file_name = output_path + 'access_%s.csv' % today
result_file_name = output_path + 'result_%s.csv' % today
other_file_name = output_path + 'other_%s.csv' % today

log_file = open(log_file_name)
csv_file = open(csv_file_name, 'w')
csv_file.write('type,id,level,map_id,create_day,system,device\n')

for line in log_file:
    if line.find("/zhanjian?") == -1:
        continue
    try:
        type_ = int(re.findall(r"type=(.+?)&", line)[0])
        id_ = int(re.findall(r"&id=(.+?)&", line)[0])
        lv_ = int(re.findall(r"lv=(.+?)&", line)[0])
        map_id_ = int(re.findall(r"mapid=(.+?)&", line)[0])
        create_day_ = int(re.findall(r"createday=(.+?) ", line)[0])
        system_ = "Unknown"
        device_ = "Unknown"
        system_ = re.findall(r"\((.+?);", line)[0]
        if system_ == 'iPhone':
            system_ = "IOS " + re.findall(r"iPhone OS (.+?) ", line)[0]
            device_ = 'iPhone'
        elif system_ == 'Linux':
            system_ = 'Android ' + re.findall(r"Android (.+?);", line)[0]
            device_ = re.findall(r"Android[^;]*; (.+?)\)", line)[0]
        elif system_.find("Windows") != -1:
            device_ = 'PC'
        csv_file.write("%d,%d,%d,%d,%d,%s,%s\n" % (type_, id_, lv_, map_id_, create_day_, system_, device_))
    except IndexError:
        continue
    except ValueError:
        continue
    except AttributeError:
        continue

csv_file.close()
log_file.close()

data = pandas.read_csv(csv_file_name)
result = data.groupby(['map_id', 'type']).size()
result_file = open(result_file_name, 'w')
result_file.write('MapID,Step,PlayerCount,ChurnRate\n')
for x in result.iteritems():
    rate = 0
    try:
        rate = (1 - x[1] / result[(x[0][0], 300)]) * 100
    except KeyError:
        continue
    result_file.write("%d,%d,%d,%.2f%%\n" % (x[0][0], x[0][1], x[1], rate))

result_file.close()

data = data.drop_duplicates(['id'])
other_file = open(other_file_name, 'w')
sys_result = data.groupby(['system']).size()
other_file.write('System,Count\n')
for x in sys_result.iteritems():
    other_file.write('%s,%s\n' % (x[0], x[1]))
dev_result = data.groupby(['device']).size()
other_file.write('\nDevice,Count\n')
for x in dev_result.iteritems():
    other_file.write('%s,%s\n' % (x[0], x[1]))
other_file.close()

