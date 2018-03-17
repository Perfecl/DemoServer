#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime
import re

conn = NewDBConn()

day = datetime.now() - timedelta(days=1)
day_str = "%04d-%02d-%02d" % (day.year, day.month, day.day)
day_end = day + timedelta(days=1)
day_end_str = "%s" % day_end

regex = re.compile("[,|]")

item_sql = """
select server, uid, dialog_id from player_%s
where from_unixtime(create_time) >= '%s' and from_unixtime(create_time) < '%s';
"""

server_list = dict()

try:
    for x in range(16):
        cur = conn.cursor()
        try:
            sql = item_sql % (x, day_str, day_end_str)
            cur.execute(sql)
            for row in cur:
                server = int(row[0])
                if server not in server_list:
                    server_list[server] = list()
                server_list[server].append(row[2])
                #print(row)
        except:
            pass
        finally:
            cur.close()

except:
    pass

#玩家数量
player_count = dict()
#新手引导统计, (server, type) => dict()
stat_dict = dict()

for server in server_list:
    if len(server_list[server]) <= 0:
        continue
    player_count[server] = len(server_list[server])
    l = server_list[server]
    for item in l:
        array = regex.split(item)
        if len(array) / 2 <= 0.5:
            continue
        for index in range(0, len(array), 2):
            k = int(array[index])
            v = int(array[index+1])
            if (server, k) not in stat_dict:
                stat_dict[(server, k)] = dict()
            if v != 0 and v not in stat_dict[(server, k)]:
                stat_dict[(server, k)][v] = 0
            if v != 0:
                stat_dict[(server, k)][v] += 1


conn_gm = NewGMDBConn()
cur = conn_gm.cursor()

sql_list = list()
sql_list.append("insert into stat_guide (date, server, guide_type, id, count, percent) values ")

for (server, guide_type) in stat_dict:
    l = stat_dict[(server, guide_type)]
    if len(l) <= 0:
        continue
    l1 = [(k, l[k]) for k in sorted(l.keys())]
    total_count = player_count[server]
    zero_count = total_count - sum(l.values())
    if len(sql_list) > 1:
        sql_list.append(',')
    sql_list.append("('%s', %s,%s,%s,%s,%s)" % (day_str, server, guide_type, 0, zero_count, zero_count / total_count))
    for (guide_id, count) in l1:
        if len(sql_list) > 1:
            sql_list.append(',')
        sql_list.append("('%s', %s,%s,%s,%s,%s)" % (day_str, server, guide_type, guide_id, count, count / total_count))

sql_list.append("ON DUPLICATE KEY UPDATE count=values(count), percent=values(percent)")

sql = "".join(sql_list)
try:
    cur.execute(sql)
except:
    pass
finally:
    cur.close()

conn_gm.close()
conn.close()
