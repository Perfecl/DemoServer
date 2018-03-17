#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime

conn = NewDBConn()

day_count = 3
day3before = datetime.now() - timedelta(days=day_count)
day2before = datetime.now() - timedelta(days=day_count - 1)
today = datetime.now() - timedelta(days=day_count - 3)

day3_str = "%04d-%02d-%02d" % (day3before.year, day3before.month, day3before.day)
day2_str = "%04d-%02d-%02d" % (day2before.year, day2before.month, day2before.day)
today_str = "%04d-%02d-%02d" % (today.year, today.month, today.day)

sql_template = """
select uid, server, progress
from player_%s as p inner join copy_%s as c
on p.uid = c.player_id
and from_unixtime(create_time) >= '%s' and from_unixtime(create_time) < '%s'
and from_unixtime(last_login_time) < '%s'
"""

server_list = dict()

def ParseDeadmanCopy(server, progress):
    if progress is None:
        if 0 not in server_list[server]:
            server_list[server][0] = 0
        server_list[server][0] += 1
        return
    array = progress.split(';')
    for p in array:
        if p.find("1:") >= 0:
            copy = int(p.split(":")[-1])
            if copy not in server_list[server]:
                server_list[server][copy] = 0
            server_list[server][copy] += 1
    pass

try:
    for x in range(8):
        cur = conn.cursor()
        try:
            sql = sql_template % (x, x, day3_str, day2_str, today_str)
            cur.execute(sql)
            for row in cur:
                server = int(row[1])
                if server not in server_list:
                    server_list[server] = dict()
                ParseDeadmanCopy(row[1], row[2])
        except:
            pass
        finally:
            cur.close()
except:
    pass

print(day3_str)
print(server_list)

if len(server_list) <= 0:
    exit()

conn_gm = NewGMDBConn()
cur = conn_gm.cursor()
sql_list = list()
sql_list.append("insert into stat_deadman_copy (date, server, copy_id, count) values ")

for server in server_list:
    copys = sorted(server_list[server].keys())
    for copy in copys:
        if len(sql_list) > 1:
            sql_list.append(',')
        sql_list.append("('%s', %s, %s, %s)" % (day3_str, server, copy, server_list[server][copy]))
    sql_list.append("ON DUPLICATE KEY UPDATE count=values(count)")

try:
    sql = "".join(sql_list)
    cur.execute(sql)
except:
    pass
finally:
    cur.close()

conn.close()
conn_gm.close()