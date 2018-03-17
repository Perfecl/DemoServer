#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime

conn = NewLogDBConn()

day = datetime.now() - timedelta(days=1)
day_str = "%04d-%02d-%02d" % (day.year, day.month, day.day)

sql = """
select server, copy_type, copy_id, copy_order, star, count(*) as count
from `copy_%s`
group by server, copy_id, star
""" % day_str

conn_gm = NewGMDBConn()
cur_gm = conn_gm.cursor()

sql_list = list()
sql_list.append("insert into stat_copy (date, server, copy_type, copy_id, copy_order, copy_star, count) values ")

try:
    cur = conn.cursor()
    cur.execute(sql)
    for row in cur:
        if len(sql_list) > 1:
            sql_list.append(',')
        sql_list.append("('%s', %s, %s, %s, %s, %s, %s)" % (day_str, row[0], row[1], row[2], row[3], row[4], row[5]))
    sql_list.append("ON DUPLICATE KEY UPDATE copy_star=values(copy_star), count=values(count)")
    sql = "".join(sql_list)

    cur_gm.execute(sql)
except:
    pass
finally:
    cur.close()
    cur_gm.close()

conn.close()
conn_gm.close()
