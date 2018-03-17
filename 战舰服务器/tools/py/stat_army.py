#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime
import re

day = datetime.now()
day_str = "%04d-%02d-%02d" % (day.year, day.month, day.day)
regex = re.compile(";37:(\d+)")

db_conn = NewDBConn()
gm_conn = NewGMDBConn()


class StatArmyInfo:
    def __init__(self):
        self.server = 0
        self.army_id = 0
        self.level = 0
        self.army_name = ""
        self.member_count = 0
        self.live_count = 0
        self.copy_15_player_count = 0
        self.copy_15_count = 0
        pass

    def __str__(self):
        return "%s,%s,%s,'%s',%s,%s,%s,%s" % (self.server, self.army_id, self.level,
                                              db_conn.escape_string(self.army_name), self.member_count,
                                              self.live_count, self.copy_15_player_count, self.copy_15_count)

#army_id => StatArmyInfo
army_dict = dict()

#获取army id
def FillArmyInfo():
    sql = "select army_id, army_name, server, level from army"
    cur = db_conn.cursor()
    cur.execute(sql)
    for (army_id, army_name, server, level) in cur:
        if level <= 1:
            continue
        if army_id not in army_dict:
            army_dict[army_id] = StatArmyInfo()
        army_dict[army_id].army_id = army_id
        army_dict[army_id].army_name = army_name
        army_dict[army_id].level = level
        army_dict[army_id].server = server
    pass

#获取捐献人数
def FillLiveCount():
    sql = "select army_id, count(*) from army_member where today_exp > 0 and from_unixtime(army_update_time) >= '%s' GROUP BY army_id" % day_str
    cur = db_conn.cursor()
    cur.execute(sql)
    for (army_id, live_count) in cur:
        if army_id not in army_dict:
            continue
        army_dict[army_id].live_count = live_count
    cur.close()
#获取人数
def FillArmyMemberCount():
    sql = "select army_id, count(*) from army_member group by army_id"
    cur = db_conn.cursor()
    cur.execute(sql)
    for (army_id, army_member_count) in cur:
        if army_id not in army_dict:
            continue
        army_dict[army_id].member_count = army_member_count
    cur.close()

#获取副本类型15的参与度
def FillCopy15Count():
    sql = """
        select uid, army_id, buy_count
        from player_%s as player
        inner join copy_%s as copy
        on player.uid = copy.player_id
        inner join tactic_%s as tactic 
        on player.uid = tactic.player_id
        where army_id != 0 and from_unixtime(player.fresh_time) >= '%s'
        """
    #army_id => set()
    copy_15_player = dict()
    #army_id => count
    copy_15_count = dict()

    for index in range(16):
        real_sql = sql % (index, index, index, day_str)
        cur = db_conn.cursor()
        try:
            cur.execute(real_sql)
            for (player_id, army_id, buy_count) in cur:
                if buy_count.find(";37:") < 0:
                    continue
                for count in regex.findall(buy_count):
                    if army_id not in copy_15_player:
                        copy_15_player[army_id] = set()
                    copy_15_player[army_id].add(player_id)
                    if army_id not in copy_15_count:
                        copy_15_count[army_id] = 0
                    copy_15_count[army_id] += int(count)
                    break
        except:
            break
        finally:
            cur.close()
            pass
    for army_id in copy_15_player.keys():
        if army_id not in army_dict:
            continue
        army_dict[army_id].copy_15_player_count = len(copy_15_player[army_id])
    for army_id in copy_15_count.keys():
        if army_id not in army_dict:
            continue
        army_dict[army_id].copy_15_count = copy_15_count[army_id]


FillArmyInfo()
FillLiveCount()
FillArmyMemberCount()
FillCopy15Count()

cur = gm_conn.cursor()
sql_list = list()
sql_list.append("insert into stat_army(date, server, army_id, level, name, member_count, live_count, copy_15_player_count, copy_15_count) values \n")
for army_id in army_dict.keys():
    if len(sql_list) > 1:
        sql_list.append(",\n")
    sql_list.append("('%s',%s)" % (day_str, army_dict[army_id]))

sql_list.append("\nON DUPLICATE KEY UPDATE")
sql_list.append(" level=values(level)")
sql_list.append(", name=values(name)")
sql_list.append(", member_count=values(member_count)")
sql_list.append(", live_count=values(live_count)")
sql_list.append(", copy_15_player_count=values(copy_15_player_count)")
sql_list.append(", copy_15_count=values(copy_15_count)")

sql = "".join(sql_list)
#print(sql)
cur.execute(sql)