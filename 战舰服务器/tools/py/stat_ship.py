#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime

day = datetime.now() - timedelta(days=1)
day_str = "%04d-%02d-%02d" % (day.year, day.month, day.day)
day_end = day + timedelta(days=1)
day_str_end = "%04d-%02d-%02d" % (day_end.year, day_end.month, day_end.day)

conn = NewDBConn()

item_dict = dict()

item_sql = """
select player.server, hero.player_id, hero.hero_id, hero.`level`, hero.grade, hero.fate_level, player.level
from player_%s as player inner join hero_%s as hero
where player.uid = hero.player_id
and from_unixtime(player.last_login_time) >= '%s' and from_unixtime(player.last_login_time) < '%s'
"""

class Ship:
    def __init__(self, row):
        self.PlayerID = int(row[1])
        self.PlayerLevel = row[6]
        self.ShipID = int(row[2])
        self.Level = row[3]
        self.Grade = row[4]
        self.FateLevel = row[5]

    def __str__(self):
        return "%s,%s,%s,%s,%s,%s" % (self.PlayerID, self.PlayerLevel, self.ShipID, self.Level, self.Grade, self.FateLevel)

try:
    for table in range(0, 16):
        cur = conn.cursor()
        try:
            sql = item_sql % (table, table, day_str, day_str_end)
            cur.execute(sql)
            for row in cur:
                server = int(row[0])
                item = Ship(row)
                if server not in item_dict:
                    item_dict[server] = list()
                item_dict[server].append(item)
                #print(row)
        except:
            pass
        finally:
            cur.close()
            pass
except:
    pass

conn_gm = NewGMDBConn()



for server in item_dict:
    cmp = {"stat_hero_level": lambda i: i.Level,
           "stat_hero_grade": lambda i: i.Grade,
           "stat_hero_fate_level": lambda i: i.FateLevel}
    item_list = item_dict[server]
    for table_name in cmp:
        sql_list = list()
        sql_list.append("insert into %s (date, server, player_id, player_level, hero_id, level, grade, fate_level) values" % table_name)

        ship_list = sorted(item_list, key=cmp[table_name], reverse=True)
        if len(ship_list) > 512:
            ship_list = ship_list[0:512]
        for ship in ship_list:
            if len(sql_list) > 1:
                sql_list.append(',')
            sql_list.append("('%s',%s,%s)" % (day_str, server, ship))
        sql_list.append("ON DUPLICATE KEY UPDATE player_level=values(player_level), level=values(level), grade=values(grade), fate_level=values(fate_level)")
        cur = conn_gm.cursor()
        sql = "".join(sql_list)
        try:
            cur.execute(sql)
        except:
            pass
        finally:
            cur.close()


conn_gm.close()
conn.close()
