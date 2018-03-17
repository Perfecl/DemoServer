#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime

time_point = datetime.now() - timedelta(days=0)
today = time_point.strftime("%Y-%m-%d")

game_db_conn = NewDBConn()
game_cur = game_db_conn.cursor()
gm_db_conn = NewGMDBConn()
gm_cur = gm_db_conn.cursor()


def get_daily_sign(container, type):
    if type < 0:
        return False
    index = type // 32
    if index >= len(container):
        return False
    if (int(container[index]) & (1 << (type % 32))) is not 0:
        return True
    return False


def add_value(container, key_a, key_b, val):
    if key_a not in container:
        container[key_a] = dict()
    if key_b not in container[key_a]:
        container[key_a][key_b] = 0
    container[key_a][key_b] += val

server_list = dict()
sql = "select b.`server`,a.daily_sign from reward_%d as a inner join player_%d as b on a.player_id = b.uid " \
      "where a.daily_sign is not null and a.daily_sign != '' and a.daily_sign != '0' " \
      "and b.fresh_time >= unix_timestamp('%s')"
for x in range(8):
    try:
        game_cur.execute(sql % (x, x, today))
        for row in game_cur:
            server = row[0]
            daily_sign = str(row[1]).split(",")
            for y in range(64):
                if get_daily_sign(daily_sign, y) is True:
                    add_value(server_list, server, y, 1)
    except pymysql.err.IntegrityError as e:
        print(e)
    except pymysql.err.ProgrammingError:
        break

sql = "insert into stat_recharge_award (server, date, all_daily_sign) values(%d, '%s', '%s') " \
      "ON DUPLICATE KEY UPDATE all_daily_sign=values(all_daily_sign)"
for key in server_list:
    daily_sign_str = ""
    for type_d in sorted(server_list[key].keys()):
        daily_sign_str += (str(type_d) + ':' + str(server_list[key][type_d]) + ",")
    daily_sign_str = daily_sign_str[:-1]
    try:
        gm_cur.execute(sql % (key, today, daily_sign_str))
    except pymysql.err.IntegrityError as e:
        print(e)

gm_db_conn.close()
gm_cur.close()
game_cur.close()
game_db_conn.close()
