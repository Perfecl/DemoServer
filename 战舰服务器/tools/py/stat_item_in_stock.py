#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from collections import *
from datetime import timedelta, datetime

day = datetime.now() - timedelta(days=1)
day_str = "%04d-%02d-%02d" % (day.year, day.month, day.day)
day_end = day + timedelta(days=1)
day_str_end = "%s" % day_end

conn = NewDBConn()

item_sql = """
select server, item_id, sum(item_count) from (
select server, item.item_id as item_id, item.item_count as item_count
from player_%s as player inner join item_%s as item
where player.uid = item.player_id and
from_unixtime(player.last_login_time) >= '%s' and from_unixtime(player.last_login_time) < '%s'
) as tb
group by server, item_id
"""

stat_item = dict()

try:
    for table in range(0, 16):
        cur = conn.cursor()
        try:
            sql = item_sql % (table, table, day_str, day_str_end)
            cur.execute(sql)
            for row in cur:
                k = (int(row[0]), int(row[1]))
                if k not in stat_item:
                    stat_item[k] = 0
                stat_item[k] = stat_item[k] + int(row[2])
                #print(row)
        except:
            pass
        finally:
            cur.close()
            pass
except:
    pass

conn_gm = NewGMDBConn()
cur = conn_gm.cursor()
sql_list = list()
sql_list.append('insert into stat_item_in_stock(date, server, item_id, count) values ')

try:
    d = OrderedDict(sorted(stat_item.items(), key=lambda t: t))
    for (Server, ItemID) in d:
        if len(sql_list) > 1:
            sql_list.append(',')
        Count = d[(Server, ItemID)]
        sql_list.append("('%s', %s, %s, %s)" % (day_str, Server, ItemID, Count))
    sql_list.append("ON DUPLICATE KEY UPDATE count=values(count)")
    sql = "".join(sql_list)
    cur.execute(sql)
except:
    pass
finally:
    cur.close()


conn_gm.close()
conn.close()
