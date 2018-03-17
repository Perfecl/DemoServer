#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime

time_point = datetime.now()
today = time_point.strftime("%Y-%m-%d")

game_db_conn = NewDBConn()
game_cur = game_db_conn.cursor()

gm_db_conn = NewGMDBConn()
gm_cur = gm_db_conn.cursor()

server_list = dict()

sql = "select server from zhanjian_log.`item_%s` where msgid=0x216D group by server" % today

try:
    gm_cur.execute(sql)
    for row in gm_cur:
        server_list[row[0]] = dict()
except pymysql.err.IntegrityError as e:
    print(e)


def get_value(str_src, str_key):
    value = 0
    index_0 = str_src.find(str_key+":")
    if index_0 > 0:
        index_1 = str_src[index_0:].find(";")
        value = str_src[index_0 + 4: index_0 + index_1] if index_1 > 0 else ach[index_0 + 4: -1]
    return value


def add_value(container, key_a, key_b, val):
    if key_a not in container:
        container[key_a] = dict()
    if key_b not in container[key_a]:
        container[key_a][key_b] = 0
    container[key_a][key_b] += val


def get_v(container, k):
    if k not in container:
        return 0
    return container[k]

sql = "select achievement from copy_%d where achievement is not NULL and player_id in " \
      "(select uid from role_name where server = '%s')"
for x in range(8):
    for key in server_list:
        try:
            game_cur.execute(sql % (x, key))
            for row in game_cur:
                ach = str(row[0])
                point = int(get_value(ach, "405"))
                record = int(get_value(ach, "406"))
                if point > 0:
                    add_value(server_list, key, "buy_players", 1)
                    add_value(server_list, key, "buy_count", point // 10)
                for y in range(8):
                    if (record & (1 << y)) != 0:
                        add_value(server_list, key, "box" + str(y + 1), 1)
        except pymysql.err.IntegrityError as e:
            print(e)
        except pymysql.err.ProgrammingError:
            break

sql = "insert into stat_research_item (server,`date`, buy_player_count, buy_count, box1_count, box2_count, box3_count, " \
      "box4_count, box5_count, box6_count, box7_count) values (%d, '%s', %d, %d, %d, %d, %d, %d, %d, %d, %d) " \
      "ON DUPLICATE KEY UPDATE buy_player_count=values(buy_player_count), buy_count=values(buy_count), " \
      "box1_count=values(box1_count), box2_count=values(box2_count),box3_count=values(box3_count)," \
      "box4_count=values(box4_count),box5_count=values(box5_count),box6_count=values(box6_count)," \
      "box7_count=values(box7_count)"

for key in server_list:
    t_sql = sql % (int(key), today, get_v(server_list[key], "buy_players"), get_v(server_list[key], "buy_count"),
                   get_v(server_list[key], "box1"), get_v(server_list[key], "box2"), get_v(server_list[key], "box3"),
                   get_v(server_list[key], "box4"), get_v(server_list[key], "box5"), get_v(server_list[key], "box6"),
                   get_v(server_list[key], "box7"))
    try:
        gm_cur.execute(t_sql)
    except pymysql.err.IntegrityError as e:
        print(e)

game_cur.close()
game_db_conn.close()

gm_cur.close()
gm_db_conn.close()
