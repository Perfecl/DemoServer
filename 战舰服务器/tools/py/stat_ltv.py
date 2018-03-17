#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime

time_point = datetime.now()
today = time_point.strftime("%Y-%m-%d")

game_db_conn = NewDBConn()
game_cur = game_db_conn.cursor()
gm_conn = NewGMDBConn()
gm_cur = gm_conn.cursor()

sql = "select server, datediff(now(), from_unixtime(create_time)) as diff_day, count(uid), (sum(total_recharge)/100) " \
      "as all_recharge from player_%d  where openid not like 'DEBUG_%%' group by server, diff_day " \
      "having (diff_day < 30 or diff_day in (36,43,50,57))"

result = dict()
x = 0
while True:
    try:
        game_cur.execute(sql % x)
        for row in game_cur:
            if row[0] not in result:
                result[row[0]] = dict()
            if row[1] not in result[row[0]]:
                result[row[0]][row[1]] = dict()
            if "count" not in result[row[0]][row[1]]:
                result[row[0]][row[1]]["count"] = 0
            if "recharge" not in result[row[0]][row[1]]:
                result[row[0]][row[1]]["recharge"] = 0
            result[row[0]][row[1]]["count"] += row[2]
            result[row[0]][row[1]]["recharge"] += row[3]
    except pymysql.err.IntegrityError as e:
        print(e)
    except pymysql.err.ProgrammingError:
        break
    x += 1

sql = "insert into stat_ltv (server_id, date_time, new_account, day%d) values (%d, '%s', %d, %.2f) on duplicate key " \
      "update new_account=values(new_account), day%d=values(day%d)"
for server_id in result:
    for diff in result[server_id]:
        time_day = (time_point-timedelta(days=diff)).strftime("%Y-%m-%d")
        if diff < 0:
            continue
        sql_r = sql % (diff + 1, server_id, time_day, result[server_id][diff]["count"],
                       result[server_id][diff]["recharge"] / result[server_id][diff]["count"], diff + 1, diff + 1)
        try:
            gm_cur.execute(sql_r)
        except pymysql.err.IntegrityError as e:
            print(e)
        except pymysql.err.ProgrammingError as e:
            pass
        except pymysql.InternalError as e:
            print(e)
            pass

game_cur.close()
game_db_conn.close()
gm_cur.close()
gm_conn.close()
