#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime

conn = NewLogDBConn()

day = datetime.now() - timedelta(days=1)
day_str = "%04d-%02d-%02d" % (day.year, day.month, day.day)

item_sql = """
select server, msgid, hero_id, sum(count) from (
select server, tid, msgid, hero_id, 1 as count from `hero_%s` where old_level = 0
) as tb
group by server, msgid, hero_id
""" % day_str

stat_item = list()

try:
    #统计船只获得
    cur = conn.cursor()
    cur.execute(item_sql)
    for row in cur:
        stat_item.append((row[0], row[1], row[2], row[3]))
        #print(row)
    cur.close()
    pass
finally:
    pass

conn_gm = NewGMDBConn()
cur = conn_gm.cursor()

sql_list = list()
sql_list.append("insert into stat_new_ship(date, server, msgid, hero_id, count) values ")

try:
    for (Server, MsgID, HeroID, Count) in stat_item:
        if len(sql_list) > 1:
            sql_list.append(",")
        sql_list.append("('%s',%s,%s,%s,%s)" % (day_str, Server, MsgID, HeroID, Count))
    sql_list.append("ON DUPLICATE KEY UPDATE count=values(count)")
except:
    pass

sql = "".join(sql_list)
try:
    cur.execute(sql)
except:
    pass
finally:
    cur.close()

conn_gm.close()
conn.close()
