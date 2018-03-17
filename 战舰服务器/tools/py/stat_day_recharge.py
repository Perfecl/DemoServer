#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime

game_db_conn = NewDBConn()
game_cur = game_db_conn.cursor()

gm_conn = NewGMDBConn()
gm_cur = gm_conn.cursor()

time_point = datetime.now() - timedelta(days=1)
today = time_point.strftime("%Y-%m-%d")


total_recharge = dict()
select_sql = """
select `server`,datediff('%s',from_unixtime(create_time))+1 as days,sum(total_recharge) as money 
from player_%d 
where total_recharge !=0 and datediff('%s',from_unixtime(create_time)) < 30 
group by `server`,days
"""

x = 0
while True:
    try:
        game_cur.execute(select_sql % (today, x, today))
        for row in game_cur:
            if row[0] not in total_recharge:
                total_recharge[row[0]] = dict()
            if row[1] not in total_recharge[row[0]]:
                total_recharge[row[0]][row[1]] = 0
            total_recharge[row[0]][row[1]] += int(row[2])
    except pymysql.err.IntegrityError as ex:
        print(ex)
    except pymysql.err.ProgrammingError:
        break
    x += 1

insert_sql = "insert into stat_day_recharge (date,server,day%s) values('%s',%u,%d) " \
             "on duplicate key update day%s=values(day%s)"
day_list = [7, 14, 30]
for server in total_recharge:
    for day_count in total_recharge[server]:
        for days in day_list:
            if day_count <= days:
                date_str = (time_point - timedelta(days=(day_count-1))).strftime("%Y-%m-%d")
                temp_sql = insert_sql % (days, date_str, server, total_recharge[server][day_count], days, days)
                try:
                    gm_cur.execute(temp_sql)
                except pymysql.err.IntegrityError as ex:
                    print(ex)

gm_cur.close()
gm_conn.close()

game_cur.close()
game_db_conn.close()
