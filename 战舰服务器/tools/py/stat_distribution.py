#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime

time_point = datetime.now()
today = time_point.strftime("%Y-%m-%d")
tomorrow = (time_point + timedelta(days=1)).strftime("%Y-%m-%d")

gm_db_conn = NewGMDBConn()
game_db_conn = NewDBConn()

gm_cur = gm_db_conn.cursor()
game_cur = game_db_conn.cursor()


def add_value(container, key_a, key_b, val):
    if key_a not in container:
        container[key_a] = dict()
    if key_b not in container[key_a]:
        container[key_a][key_b] = 0
    container[key_a][key_b] += val


def insert_sql_value(sql_q, container):
    x = 0
    while True:
        try:
            game_cur.execute(sql_q % x)
            for row in game_cur:
                add_value(container, row[0], row[1], row[2])
        except pymysql.err.IntegrityError as e:
            print(e)
        except pymysql.err.ProgrammingError:
            break
        x += 1

level_table = dict()
sql = "select `server`,`level`,count(*) from player_%d" + " where fresh_time >= unix_timestamp('%s') "\
      " group by `server`,`level`" % today
insert_sql_value(sql, level_table)

vip_table = dict()
sql = "select `server`,`vip_level`,count(*) from player_%d" + " where vip_level > 0 and " \
      " fresh_time >= unix_timestamp('%s') group by `server`,`vip_level`" % today
insert_sql_value(sql, vip_table)

first_recharge_table = dict()
sql = "select a.`server`,a.`level`,count(*) from player_%d" + " as a inner join recharge_uid as b " \
      "on a.uid = b.player_id where b.recharge_time > unix_timestamp('%s') group by a.`server`,a.`level`" % today
#insert_sql_value(sql, first_recharge_table)

recharge_table = dict()
sql = "select `server`,`level`,count(*) from player_%d" + " where uid in (select distinct role_id from " \
      "recharge_details where `timestamp` > unix_timestamp('%s')) group by `server`,`level`" % today
insert_sql_value(sql, recharge_table)

level_avg_table = dict()
sql = "select `server`,`level`,avg(money) from player_%d" + " where fresh_time > unix_timestamp('%s') " \
      "group by `server`,`level`" % today
insert_sql_value(sql, level_avg_table)

vip_avg_table = dict()
sql = "select `server`,`vip_level`,avg(money) from player_%d" + " where fresh_time > unix_timestamp('%s') " \
      "group by `server`,`vip_level`" % today
insert_sql_value(sql, vip_avg_table)

list_dict = [level_table, vip_table, first_recharge_table, recharge_table, level_avg_table, vip_avg_table]

index = 0
while index < len(list_dict):
    sql_list = list()
    sql_list.append("insert into stat_distribution(date, server, type, level, count) values ")
    for server_id in sorted(list_dict[index].keys()):
        for level in sorted(list_dict[index][server_id].keys()):
            if len(sql_list) > 1:
                sql_list.append(', ')
            sql_list.append("('%s', %s, %d, %s, %s)" % (today, server_id, index + 1,
                                                        level, list_dict[index][server_id][level]))
    sql_list.append("ON DUPLICATE KEY UPDATE count=values(count)")
    sql = "".join(sql_list)
    try:
        gm_cur.execute(sql)
    except pymysql.err.IntegrityError as e:
        print(e)
    except pymysql.err.ProgrammingError:
        break
    index += 1

gm_cur.close()
game_cur.close()

gm_db_conn.close()
game_db_conn.close()
