#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from collections import *
from datetime import timedelta, datetime

conn = NewLogDBConn()

day = datetime.now() - timedelta(days=1)
day_str = "%04d-%02d-%02d" % (day.year, day.month, day.day)


add_item_sql = """
select server, item_id, sum(delta_count)
from (
  select server, item_id, item_count - item_old_count as delta_count
  from `item_%s` where item_count > item_old_count
) as tb
group by server, item_id;
""" % day_str

reduce_item_sql = """
select server, item_id, sum(delta_count)
from (
  select server, item_id, item_old_count - item_count as delta_count
  from `item_%s` where item_count < item_old_count
) as tb
group by server, item_id;
""" % day_str

delete_item_sql = """
select server, item_id, sum(delta_count)
from (
  select server, item_id, item_count as delta_count
  from `item_delete_%s`
) as tb
group by server, item_id
""" % day_str

#(ServerID,ItemID) => (add,reduce)
stat_item = dict()

try:
    #统计获得
    cur = conn.cursor()
    cur.execute(add_item_sql)
    for row in cur:
        k = (int(row[0]), int(row[1]))
        if k not in stat_item:
            stat_item[k] = (0, 0)
        (add, reduce) = stat_item[k]
        add = add + int(row[2])
        stat_item[k] = (add, reduce)
        #print(row)
    cur.close()
    #统计消耗
    cur = conn.cursor()
    cur.execute(reduce_item_sql)
    for row in cur:
        k = (int(row[0]), int(row[1]))
        if k not in stat_item:
            stat_item[k] = (0, 0)
        (add, reduce) = stat_item[k]
        reduce = reduce + int(row[2])
        stat_item[k] = (add, reduce)
        #print(row)
    cur.close()
    #删除统计
    cur = conn.cursor()
    cur.execute(delete_item_sql)
    for row in cur:
        k = (int(row[0]), int(row[1]))
        if k not in stat_item:
            stat_item[k] = (0, 0)
        (add, reduce) = stat_item[k]
        reduce = reduce + int(row[2])
        stat_item[k] = (add, reduce)
        #print(row)
    cur.close()
    pass
finally:
    pass

conn_gm = NewGMDBConn()
cur = conn_gm.cursor()
sql_list = list()
sql_list.append('insert into stat_item(date, server, item_id, add_count, reduce_count) values ')

try:
    d = OrderedDict(sorted(stat_item.items(), key=lambda t: t))
    for (Server, ItemID) in d:
        if len(sql_list) > 1:
            sql_list.append(',')
        (Add, Reduce) = d[(Server, ItemID)]
        sql_list.append("('%s', %s, %s, %s, %s)" % (day_str, Server, ItemID, Add, Reduce))
    sql_list.append("ON DUPLICATE KEY UPDATE add_count=values(add_count), reduce_count=values(reduce_count)")
    sql = "".join(sql_list)
    cur.execute(sql)
except:
    pass
finally:
    cur.close()


conn_gm.close()
conn.close()