#!/usr/bin/python3
# coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime

time_point = datetime.now() - timedelta(days=1)
today = time_point.strftime("%Y-%m-%d")
tomorrow = (time_point + timedelta(days=1)).strftime("%Y-%m-%d")

# today = "2016-11-11"

auth_db_conn = NewAuthDBConn()
gm_db_conn = NewGMDBConn()
game_db_conn = NewDBConn()
log_db_conn = NewLogDBConn()

auth_cur = auth_db_conn.cursor()
gm_cur = gm_db_conn.cursor()
game_cur = game_db_conn.cursor()
log_cur = log_db_conn.cursor()


class ServerInfo:
    login_num = 0
    max_online_num = 0
    new_player_num = 0
    recharge_num = 0
    pay_player_num = 0

server_info = dict()

sql = "select `server`,count(distinct player_id) from `login_%s` group by `server`" % today
try:
    log_cur.execute(sql)
except pymysql.err.IntegrityError as e:
    print(e)
for row in log_cur:
    if row[0] not in server_info:
        server_info[row[0]] = ServerInfo()
    server_info[row[0]].login_num = int(row[1])

sql = "select server_id, max(online_num) from gm_online_info where cur_time >= unix_timestamp('%s') and " \
      "cur_time < unix_timestamp('%s') group by server_id" % (today, tomorrow)
try:
    gm_cur.execute(sql)
except pymysql.err.IntegrityError as e:
    print(e)
for row in gm_cur:
    if row[0] not in server_info:
        server_info[row[0]] = ServerInfo()
    server_info[row[0]].max_online_num = int(row[1])

sql = "select `server`, count(*) from `newplayer_%s` group by `server`" % today
try:
    log_cur.execute(sql)
except pymysql.err.IntegrityError as e:
    print(e)
for row in log_cur:
    if row[0] not in server_info:
        server_info[row[0]] = ServerInfo()
    server_info[row[0]].new_player_num = int(row[1])

sql = "select server_id, sum(game_coin) from recharge_details where stage = 3 and recharge_time >= " \
      "unix_timestamp('%s') and recharge_time < unix_timestamp('%s') group by server_id" % (today, tomorrow)
try:
    game_cur.execute(sql)
except pymysql.err.IntegrityError as e:
    print(e)
for row in game_cur:
    if row[0] not in server_info:
        server_info[row[0]] = ServerInfo()
    server_info[row[0]].recharge_num = int(row[1])

sql = "select server_id, count(distinct role_id) from recharge_details where stage = 3 and recharge_time >= " \
      "unix_timestamp('%s') and recharge_time < unix_timestamp('%s') group by server_id" % (today, tomorrow)
try:
    game_cur.execute(sql)
except pymysql.err.IntegrityError as e:
    print(e)
for row in game_cur:
    if row[0] not in server_info:
        server_info[row[0]] = ServerInfo()
    server_info[row[0]].pay_player_num = int(row[1])

sql = "insert into gm_day_pay_info (platform_id, server_id, date_time, new_account, total_login_account, " \
      "top_online_num, total_pay_account, total_pay_money) values(%d, %d, '%s', %d, %d, %d, %d, %d) " \
      "on DUPLICATE KEY UPDATE new_account=values(new_account), total_login_account=values(total_login_account), " \
      "top_online_num=values(top_online_num), total_pay_account=values(total_pay_account), " \
      "total_pay_money=values(total_pay_money)"

for key in server_info:
    temp = sql % (key // 10000, key, today, server_info[key].new_player_num, server_info[key].login_num,
                  server_info[key].max_online_num, server_info[key].pay_player_num, server_info[key].recharge_num)
    try:
        gm_cur.execute(temp)
    except pymysql.err.IntegrityError as e:
        print(e)

auth_cur.close()
gm_cur.close()
game_cur.close()
log_cur.close()

auth_db_conn.close()
gm_db_conn.close()
game_db_conn.close()
log_db_conn.close()

