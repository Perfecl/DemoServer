#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime


def add_value(container, key_a, key_b, val):
    if key_a not in container:
        container[key_a] = dict()
    if key_b not in container[key_a]:
        container[key_a][key_b] = 0
    container[key_a][key_b] += val

game_db_conn = NewDBConn()
game_cur = game_db_conn.cursor()

time_point = datetime.now()
today = time_point.strftime("%Y-%m-%d")

level_table = dict()
sql = "select `server`,`level`,count(*) from player_%d" + " where fresh_time >= unix_timestamp('%s') "\
      " group by `server`,`level`" % today
x = 0
while True:
    try:
        game_cur.execute(sql % x)
        for row in game_cur:
            add_value(level_table, row[0], row[1], row[2])
    except pymysql.err.IntegrityError as e:
        print(e)
    except pymysql.err.ProgrammingError:
        break
    x += 1

conn_gm = NewGMDBConn()

cur = conn_gm.cursor()
try:
    sql_list = list()
    sql_list.append("insert into stat_level(date, server, level, count) values ")
    for server_id in sorted(level_table.keys()):
        for level in sorted(level_table[server_id].keys()):
            if len(sql_list) > 1:
                sql_list.append(', ')
            sql_list.append("('%s', %s, %s, %s)" % (today, server_id, level, level_table[server_id][level]))
    sql_list.append("ON DUPLICATE KEY UPDATE count=values(count)")
    sql = "".join(sql_list)
    cur.execute(sql)
except:
    pass
finally:
    cur.close()
conn_gm.close()

game_cur.close()
game_db_conn.close()



