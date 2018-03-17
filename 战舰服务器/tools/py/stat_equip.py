#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from collections import *
from datetime import timedelta, datetime
import re

day = datetime.now() - timedelta(days=1)
day_str = "%04d-%02d-%02d" % (day.year, day.month, day.day)
day_end = day + timedelta(days=1)
day_str_end = "%04d-%02d-%02d" % (day_end.year, day_end.month, day_end.day)

conn = NewDBConn()

item_dict = dict()

regex = re.compile("[;:]")

item_sql = """
select player.server, item.player_id, item.item_id, item.item_attr, item.uid
from player_%s as player inner join item_%s as item
where player.uid = item.player_id
and from_unixtime(player.last_login_time) >= '%s' and from_unixtime(player.last_login_time) < '%s'
and length(item.item_attr) > 0
"""

class Item:
    def __init__(self, row):
        self.PlayerID = int(row[1])
        self.ItemID = int(row[2])
        self._attr = row[3]
        self.ItemUID = row[4]
        self.EquipLevel = 0
        self.EquipRefineLevel = 0
        self.NavyLevel = 0
        self.NavyRefineLevel = 0
        self.parseAttr()

    def parseAttr(self):
        #ITEM_ATTR_LEVEL             = 1;  //装备升级等级
        #ITEM_ATTR_REFINE_LEVEL      = 2;  //装备改造等级
        #ITEM_ATTR_REFINE_EXP        = 3;  //装备改造经验
        #ITEM_ATTR_EQUIPED_HERO      = 4;  //装备在哪个船上(已经废弃了)
        #ITEM_ATTR_NAVY_LEVEL        = 5;  //海军等级
        #ITEM_ATTR_NAVY_EXP          = 6;  //海军经验
        #ITEM_ATTR_NAVY_REFINE_LEVEL = 7;  //海军突破等级
        #ITEM_ATTR_EQUIPED_POS       = 8;  //装备在哪个槽位上
        array = regex.split(self._attr)
        if len(array) / 2 <= 0.5:
            return
        for index in range(0, len(array), 2):
            k = int(array[index])
            if k == 1:
                self.EquipLevel = int(array[index + 1])
            elif k == 2:
                self.EquipRefineLevel = int(array[index + 1])
            elif k == 5:
                self.NavyLevel = int(array[index + 1])
            elif k == 7:
                self.NavyRefineLevel = int(array[index + 1])
            pass

    def __str__(self):
        return "%s,%s,%s,%s,%s,%s,%s" % (self.PlayerID, self.ItemID, self.ItemUID, self.EquipLevel, self.EquipRefineLevel, self.NavyLevel, self.NavyRefineLevel)

try:
    for table in range(0, 16):
        cur = conn.cursor()
        try:
            sql = item_sql % (table, table, day_str, day_str_end)
            cur.execute(sql)
            for row in cur:
                server = int(row[0])
                item = Item(row)
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
    cmp = {"stat_equip_level": lambda i: i.EquipLevel,
           "stat_equip_refine_level": lambda i: i.EquipRefineLevel,
           "stat_navy_level": lambda i: i.NavyLevel,
           "stat_navy_refine_level": lambda i: i.NavyRefineLevel}

    item_list = item_dict[server]
    for table_name in cmp:
        sql_list = list()
        sql_list.append("insert into %s (date, server, player_id, item_id, item_uid, equip_level, equip_refine_level, navy_level, navy_refine_level) values" % table_name)

        item_list = sorted(item_list, key=cmp[table_name], reverse=True)

        if len(item_list) > 512:
            item_list = item_list[0:512]
        for item in item_list:
            if len(sql_list) > 1:
                sql_list.append(',')
            sql_list.append("('%s',%s,%s)" % (day_str, server, item))
        sql_list.append('ON DUPLICATE KEY UPDATE equip_level=values(equip_level), equip_refine_level=values(equip_refine_level), navy_level=values(navy_level), navy_refine_level=values(navy_refine_level)')
        cur = conn_gm.cursor()
        try:
            sql = "".join(sql_list)
            cur.execute(sql)
        except:
            pass
        finally:
            cur.close()

conn_gm.close()
conn.close()