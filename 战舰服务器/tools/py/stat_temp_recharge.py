#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime

game_db_conn = NewDBConn()
game_cur = game_db_conn.cursor()
gm_conn = NewGMDBConn()
gm_cur = gm_conn.cursor()
log_conn = NewLogDBConn()
log_cur = log_conn.cursor()

use_days = [7, 14, 30]


def recharge_history(date_time_point):
    date_str = date_time_point.strftime("%Y-%m-%d")
    sql_new_player = "select a.`server`, sum(b.game_coin)*10 from zhanjian_log.`newplayer_%s` as a " \
                     "inner join zhanjian.recharge_details as b on a.player_id = b.role_id where b.stage = 3 " \
                     "and b.pay_amount != 0 and datediff(from_unixtime(b.recharge_time), '%s') < %d group by `server`;"
    insert_sql = "insert into stat_day_recharge (date,server,day%s) values('%s',%u,%d) " \
                 "on duplicate key update day%s=values(day%s)"
    for days in use_days:
        try:
            temp_sql = sql_new_player % (date_str, date_str, days)
            log_cur.execute(temp_sql)
            for row in log_cur:
                temp_sql = insert_sql % (days, date_str, row[0], row[1], days, days)
                gm_cur.execute(temp_sql)
        except pymysql.err.IntegrityError as e:
            print(e)
        except pymysql.err.ProgrammingError:
            return

for x in range(30):
    time_point = datetime.strptime("2017-03-01", "%Y-%m-%d") + timedelta(days=x)
    recharge_history(time_point)


log_cur.close()
log_conn.close()
gm_cur.close()
gm_conn.close()
game_cur.close()
game_db_conn.close()
