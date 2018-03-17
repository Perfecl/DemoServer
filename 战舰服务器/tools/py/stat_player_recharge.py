#!/usr/bin/python3
# coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime

game_db_conn = NewDBConn()
gm_db_conn = NewGMDBConn()
game_cur = game_db_conn.cursor()
gm_cur = gm_db_conn.cursor()

time_point = datetime.now() - timedelta(days=1)
today = time_point.strftime("%Y-%m-%d")
rank = 50

recharge_table = dict()

sql_pre = "set @num := 0, @type := ''"
sql = "select `uid`,`server`,`total_recharge` from (select `uid`,`server`,`total_recharge`," \
      "@num:= if(@type = `server`, @num + 1, 1) as row_number,@type:= `server`from player_%d " \
      "order by `server`,`total_recharge` desc) as x where x.row_number <= %d"
x = 0
while True:
    try:
        game_cur.execute(sql_pre)
        game_cur.execute(sql % (x, rank))
        for row in game_cur:
            if row[1] not in recharge_table:
                recharge_table[row[1]] = []
            if row[2] is not 0:
                recharge_table[row[1]].append((row[0], row[2]))
    except pymysql.err.IntegrityError as e:
        print(e)
    except pymysql.err.ProgrammingError:
        break
    x += 1

sql_list = list()
sql_list.append("insert into stat_player_recharge (`server`,`player_id`,`total_recharge`) values")
for key in recharge_table:
    recharge_table[key].sort(key=lambda item: item[1], reverse=True)
    x = 0
    for row in recharge_table[key]:
        if x >= rank:
            break
        if len(sql_list) > 1:
            sql_list.append(', ')
        sql_list.append("(%d, %ld, %d)" % (key, row[0], row[1]))
        x += 1

sql_list.append(" ON DUPLICATE KEY UPDATE total_recharge=values(total_recharge)")
sql = "".join(sql_list)
try:
    gm_cur.execute(sql)
except pymysql.err.IntegrityError as e:
    print(e)

game_cur.close()
gm_cur.close()
game_db_conn.close()
gm_db_conn.close()


