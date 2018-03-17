#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime

day = datetime.now() - timedelta(days=0)
day_str = "%04d-%02d-%02d" % (day.year, day.month, day.day)

sql_1 = """
SELECT player.player_id, pk_rank_list.server, max_fight_attr, rank, p.level
from tactic_%d as player
inner join pk_rank_list
inner join player_%d as p
on player.player_id = pk_rank_list.player_id
and p.uid = player.player_id
where rank <= 500
"""

sql_2 = """
SELECT log.player_id, sum(case when log.delta_2 <0 then log.delta_2 else 0 end) as cost,
sum(case when log.delta_2 > 0 then log.delta_2 else 0 end) as earn 
from zhanjian_log.`currency_%s` as log
inner join pk_rank_list on log.player_id = pk_rank_list.player_id
where rank <= 500 and delta_2 != 0 group by player_id;
"""

#player_id => (server, cost, earn, max_fight, rank, level)
map = dict()

game_db_conn = NewDBConn()
cur = game_db_conn.cursor()

for x in range(8):
    sql = sql_1 % (x, x)
    try:
        cur.execute(sql)
        for (player_id, server, max_fight, rank, level) in cur:
            if player_id not in map:
                map[player_id] = (server, 0, 0, max_fight, rank, level)
            pass
    except:
        pass
    finally:
        pass

sql = sql_2 % day_str
cur.execute(sql)

for (player_id, cost, earn) in cur:
    if player_id not in map:
        continue
    (server, _1, _2, max_fight, rank, level) = map[player_id]
    map[player_id] = (server, int(cost), int(earn), max_fight, rank, level)


sql_list = list()
sql_list.append("insert into zhanjian_gm.stat_top50_cost_earn(date, server, rank, player_id, earn, cost, max_fight, level) values ")

for player_id in map:
    (server, cost, earn, max_fight, rank, level) = map[player_id]
    if len(sql_list) != 1:
        sql_list.append(",")
    sql_list.append("('%s', %s, %s, %s, %s, %s, %s, %s)" % (day_str, server, rank, player_id, earn, cost, max_fight, level))

sql_list.append("ON DUPLICATE KEY UPDATE player_id=values(player_id), earn=values(earn), cost=values(cost), max_fight=values(max_fight), level=values(level)")
sql = "".join(sql_list)

cur.execute(sql)


cur.close()
game_db_conn.close()
