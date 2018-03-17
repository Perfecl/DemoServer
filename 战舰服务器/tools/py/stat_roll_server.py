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
gm_conn = NewGMDBConn()
gm_cur = gm_conn.cursor()
auth_conn = NewAuthDBConn()
auth_cur = auth_conn.cursor()

time_point = datetime.now()
today = time_point.strftime("%Y-%m-%d")


server_list = dict()
sql = "select openid, server, uid from account_%d where openid in " \
      "(select openid from account_%d where openid not like 'DEBUG_%%' group by openid having count(openid) > 1) order by create_time"
x = 0
while True:
    try:
        auth_cur.execute(sql % (x, x))
        key_set = set()
        for row in auth_cur:
            if row[0] in key_set:
                if int(row[1]) not in server_list:
                    server_list[int(row[1])] = list()
                temp_list = list()
                temp_list.append(row[0])
                temp_list.append(row[1])
                temp_list.append(row[2])
                server_list[row[1]].append(temp_list)
            else:
                key_set.add(row[0])
    except pymysql.err.IntegrityError as e:
        print(e)
    except pymysql.err.ProgrammingError:
        break
    x += 1

sql = "select sum(total_recharge) from player_%d where uid in (%s)"
result = dict()
for key in server_list:
    add_value(result, key, "player_count", len(server_list[key]))
    player_str = "0"
    for value in server_list[key]:
        player_str += "," + str(value[2])
    y = 0
    while True:
        try:
            game_cur.execute(sql % (y, player_str))
            for row in game_cur:
                if row[0] is None:
                    add_value(result, key, "money_count", 0)
                else:
                    add_value(result, key, "money_count", int(row[0]))
        except pymysql.err.IntegrityError as e:
            print(e)
        except pymysql.err.ProgrammingError:
            break
        y += 1

sql = "insert into gm_day_server_info (server_id, date_time, gunfu_player_count, gunfu_pay_amount) values " \
      "(%d, '%s', %d, %d) on duplicate key update gunfu_player_count=values(gunfu_player_count)," \
      "gunfu_pay_amount=values(gunfu_pay_amount)"

for key in result:
    try:
        gm_cur.execute(sql % (key, today, result[key]["player_count"], result[key]["money_count"] // 100))
    except pymysql.err.IntegrityError as e:
        print(e)

game_cur.close()
game_db_conn.close()
auth_conn.close()
auth_cur.close()
gm_cur.close()
gm_conn.close()
