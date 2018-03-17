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

sql = "show tables like 'copy_star_%%'"
try:
    game_cur.execute(sql)
    table_counts = game_cur.rowcount
except pymysql.err.IntegrityError as e:
    print(e)

player_tactic_info = dict()
sql = "select `server`,`player_id`,`rank` from pk_rank_list where rank <= 50"
player_ids = "("
try:
    game_cur.execute(sql)
    for row in game_cur:
        if (int(row[1])) not in player_tactic_info:
            player_tactic_info[int(row[1])] = dict()
        player_tactic_info[int(row[1])]["server"] = int(row[0])
        player_tactic_info[int(row[1])]["rank"] = int(row[2])
        if len(player_ids) > 1:
            player_ids += ","
        player_ids += str(row[1])
    player_ids += ")"
except pymysql.err.IntegrityError as e:
    print(e)

sql = "select uid,current_carrier_id from player_%d where uid in " + player_ids
for x in range(table_counts):
    try:
        game_cur.execute(sql % x)
        for row in game_cur:
            player_tactic_info[int(row[0])]["carrier_id"] = int(row[1])
    except pymysql.err.IntegrityError as e:
        print(e)
    except pymysql.err.ProgrammingError:
        break

for key in player_tactic_info:
    sql = "select level,reform_level from carrier_%d where player_id = %d and carrier_id = %d" \
           % (key % table_counts, key, player_tactic_info[key]["carrier_id"])
    try:
        game_cur.execute(sql)
        for row in game_cur:
            player_tactic_info[key]["carrier_level"] = int(row[0])
            player_tactic_info[key]["carrier_reform_level"] = int(row[1])
    except pymysql.err.IntegrityError as e:
        print(e)
    if "carrier_level" not in player_tactic_info[key]:
        player_tactic_info[key]["carrier_level"] = 0
    if "carrier_reform_level" not in player_tactic_info[key]:
        player_tactic_info[key]["carrier_reform_level"] = 0

sql = "select player_id, battle_pos from tactic_%d where player_id in " + player_ids
for x in range(table_counts):
    try:
        game_cur.execute(sql % x)
        for row in game_cur:
            temp_list = list()
            kv_ship = row[1].split(';')
            for item in kv_ship:
                v_ship = item.split(':')
                temp_list.append(v_ship[1])
            player_tactic_info[int(row[0])]["tactic"] = ",".join(temp_list)
    except pymysql.err.IntegrityError as e:
        print(e)
    except pymysql.err.ProgrammingError:
        break

for key in player_tactic_info:
    sql = "select hero_id, level, grade, fate_level, wake_level from hero_%d where player_id = %d and uid in %s" \
          % (key % table_counts, key, "(" + player_tactic_info[key]["tactic"] + ")")
    try:
        game_cur.execute(sql)
        str_ship_info = ""
        for row in game_cur:
            str_ship_info += str(row[0]) + "," + str(row[1]) + "," + str(row[2]) + "," \
                             + str(row[3]) + "," + str(row[4]) + ";"
        player_tactic_info[key]["ship_info"] = str_ship_info
    except pymysql.err.IntegrityError as e:
        print(e)

sql_list = list()
sql_list.append( "insert into stat_tactic_info (date, server, player_id, rank, carrier_id, carrier_level, "
                 "carrier_reform_level, ship_info) values")
for key in player_tactic_info:
    if len(sql_list) > 1:
        sql_list.append(",")
    sql_list.append("('%s', %d, %d, %d, %d, %d, %d, '%s')" % (today, player_tactic_info[key]["server"],
                                                              key,  player_tactic_info[key]["rank"],
                                                              player_tactic_info[key]["carrier_id"],
                                                              player_tactic_info[key]["carrier_level"],
                                                              player_tactic_info[key]["carrier_reform_level"],
                                                              player_tactic_info[key]["ship_info"]))
sql_list.append(" ON DUPLICATE KEY UPDATE rank=values(rank), carrier_id=values(carrier_id), "
                "carrier_level=values(carrier_level), carrier_reform_level=values(carrier_reform_level), "
                "ship_info=values(ship_info)")
sql = "".join(sql_list)
try:
    gm_cur.execute(sql)
except pymysql.err.IntegrityError as e:
    print(e)

gm_db_conn.close()
gm_cur.close()
game_cur.close()
game_db_conn.close()
