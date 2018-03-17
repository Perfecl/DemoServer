#!/usr/bin/python3
# coding=utf-8

import sys
from gm_function import *
from mysql_config import *
from datetime import timedelta, datetime

if len(sys.argv) > 1:
    try:
        time_point = datetime.strptime(sys.argv[1], "%Y-%m-%d")
    except ValueError:
        time_point = datetime.now() - timedelta(days=1)
else:
    time_point = datetime.now() - timedelta(days=1)

yesterday = (time_point - timedelta(days=1)).strftime("%Y-%m-%d")
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

# 今日登陆玩家信息
today_login_player_info = get_login_player_info(log_cur, today)

# 今日注册玩家信息
today_new_player_info = get_new_player_info(log_cur, today)

# 获取玩家信息
get_player_simple_info(game_cur, today_login_player_info)
get_player_simple_info(game_cur, today_new_player_info)

# 分成多组服务器
multi_server_new_player = split_player_info_multi_server(today_new_player_info)
multi_server_login_player = split_player_info_multi_server(today_login_player_info)

# 插入玩家转化率
for server in multi_server_new_player:
    insert_into_gm_take_rates(server, gm_cur, today, multi_server_new_player[server], today_login_player_info)

# 插入更新玩家留存
for server in multi_server_new_player:
    insert_into_gm_retention(server, gm_cur, today, multi_server_new_player[server])
    insert_into_gm_retention_30(server, gm_cur, today, multi_server_new_player[server])
for server in multi_server_login_player:
    update_gm_retention(server, gm_cur, log_cur, today, multi_server_login_player[server])
    update_gm_retention_30(server, gm_cur, log_cur, today, multi_server_login_player[server])

auth_cur.close()
gm_cur.close()
game_cur.close()
log_cur.close()

auth_db_conn.close()
gm_db_conn.close()
game_db_conn.close()
log_db_conn.close()

# input("Press any key to exit")


