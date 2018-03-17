#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime
import re

conn = NewDBConn()

day = datetime.now() - timedelta(days=0)
day_str = "%04d-%02d-%02d" % (day.year, day.month, day.day)

sql_template = """
select player.server, copy.player_id, copy.buy_count
from copy_%s as copy inner join player_%s as player
on copy.player_id = player.uid
where from_unixtime(player.last_login_time) >= '%s'
group by player.server, copy.player_id;
"""

regex = re.compile("[:;]")

#完成数
mission_count = dict()
#服务器内的玩家数
server_count = dict()

mission_begin = 6
mission_end = 31

for mession_id in range(16):
    sql = sql_template % (mession_id, mession_id, day_str)
    cur = conn.cursor()
    try:
        cur.execute(sql)
        for row in cur:
            server = int(row[0])
            if server not in server_count:
                server_count[server] = 0
            server_count[server] += 1
            if len(row[2]) <= 2:
                continue

            if server not in mission_count:
                mission_count[server] = [0 for _ in range(mission_end)]
            array = regex.split(row[2])
            #print(server, array)
            if len(array) <= 1:
                continue
            for i in range(0, len(array), 2):
                k = int(array[i])
                v = int(array[i + 1])
                if k >= mission_begin and k < mission_end and v < 0:
                    mission_count[server][k] += 1
    except:
        pass
    finally:
        cur.close()

conn_gm = NewGMDBConn()
cur = conn_gm.cursor()

sql_list = list()
sql_list.append("insert into stat_mission(date, server, mission_id, finished_count, total_count, percent) values ")

for server in mission_count:
    counts = mission_count[server]
    count = server_count[server]
    if count <= 0:
        continue
    for mession_id in range(mission_begin, mission_end):
        if len(sql_list) > 1:
            sql_list.append(",")
        sql_list.append("('%s', %s,%s,%s,%s,%s)" % (day_str, server, mession_id, counts[mession_id], count, counts[mession_id] / count))
sql_list.append("ON DUPLICATE KEY UPDATE finished_count=values(finished_count), total_count=values(total_count), percent=values(percent)")

try:
    sql = "".join(sql_list)
    cur.execute(sql)
except:
    pass
finally:
    cur.close()

conn_gm.close()
conn.close()
