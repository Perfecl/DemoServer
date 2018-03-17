#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from collections import *
from datetime import timedelta, datetime

day = datetime.now() - timedelta(days=0)
day_str = "%04d-%02d-%02d" % (day.year, day.month, day.day)
day_end = day + timedelta(days=1)
day_str_end = "%s" % day_end

conn = NewDBConn()

item_sql = """
select server,
sum(`coin`) as _1,
sum(`money`) as _2,
sum(`oil`) as _3,
sum(`energy`) as _4,
sum(`hero`) as _7,
sum(`plane`) as _8,
sum(`prestige`) as _9,
sum(`muscle`) as _10,
sum(`exploit`) as _11,
sum(`union`) as _12
from player_%d where from_unixtime(fresh_time) >= '%s'
group by server;
"""

stat_item = dict()

try:
    for table in range(0, 16):
        cur = conn.cursor()
        try:
            sql = item_sql % (table, day_str)
            cur.execute(sql)
            for row in cur:
                k = row[0]
                if k not in stat_item:
                    stat_item[k] = [0 for x in range(10)]
                for x in range(10):
                    stat_item[k][x] += row[x + 1]
                #print(row)
        except Exception as e:
            #print(table, e)
            pass
        finally:
            cur.close()
            pass
except:
    pass


conn_gm = NewGMDBConn()
cur = conn_gm.cursor()
sql_list = list()
sql_list.append('insert into stat_currency_in_stock(date, server, _1, _2, _3, _4, _7, _8, _9, _10, _11, _12) values ')

try:
    for Server in stat_item:
        money = stat_item[Server]
        if len(sql_list) > 1:
            sql_list.append(',')
        sql_list.append("\n('%s', %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)" % (day_str, Server, money[0], money[1], money[2], money[3], money[4], money[5], money[6], money[7], money[8], money[9]))
    sql_list.append("\nON DUPLICATE KEY UPDATE _1=values(_1), _2=values(_2), _3=values(_3), _4=values(_4)")
    sql_list.append(", _7=values(_7), _8=values(_8), _9=values(_9), _10=values(_10), _11=values(_11), _12=values(_12)")
    sql = "".join(sql_list)
    print(sql)
    cur.execute(sql)
except:
    pass
finally:
    cur.close()


conn_gm.close()
conn.close()

#print(stat_item)
