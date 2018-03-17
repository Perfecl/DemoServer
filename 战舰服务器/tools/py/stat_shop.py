#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime

conn = NewLogDBConn()

day = datetime.now() - timedelta(days=1)
day_str = "%04d-%02d-%02d" % (day.year, day.month, day.day)

item_sql = """
select server, item_id, sum(buy_count)
, sum(delta_1) as d1, sum(delta_2) as d2, sum(delta_5) as d5
, sum(delta_6) as d6, sum(delta_7) as d7, sum(delta_8) as d8
, sum(delta_9) as d9, sum(delta_10) as d10, sum(delta_11) as d11
, sum(delta_12) as d12
from (
select item.server, item.tid, item.item_id, (item.item_count - item.item_old_count) as buy_count,
currency.delta_1, currency.delta_2, currency.delta_5, currency.delta_6, currency.delta_7
, currency.delta_8, currency.delta_9, currency.delta_10, currency.delta_11, currency.delta_12
from `item_%s` as item inner join `currency_%s` as currency
where item.tid = currency.tid and item.msgid=0x2094 and currency.msgid = 0x2094
) as tb
group by server, item_id
order by d1,d2,d5,d6,d7,d8,d9,d10,d11,d12
;
""" % (day_str, day_str)

stat_item = list()

money_type = [(1, 3), (2, 4), (5, 5), (6, 6), (7, 7), (8, 8), (9, 9), (10, 10), (11, 11), (12, 12)]

conn_gm = NewGMDBConn()
cur_gm = conn_gm.cursor()
sql_list = list()
sql_list.append("insert into stat_shop(date, server, item_id, count, money_type, money) values ")

try:
    cur = conn.cursor()
    cur.execute(item_sql)
    for row in cur:
        (Server, ItemID, Count) = (row[0], row[1], int(row[2]))
        for mt in money_type:
            if row[mt[1]] != 0:
                if len(sql_list) > 1:
                    sql_list.append(",")
                sql_list.append("('%s',%s,%s,%s,%s,%s)" % (day_str, Server, ItemID, Count, mt[0], row[mt[1]]))
    sql_list.append("ON DUPLICATE KEY UPDATE count=values(count), money=values(money)")
    cur.close()
except:
    pass

try:
    sql = "".join(sql_list)
    cur_gm.execute(sql)
except:
    pass
finally:
    cur_gm.close()

conn_gm.close()
conn.close()
