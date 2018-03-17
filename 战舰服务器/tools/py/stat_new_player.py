#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime

day = datetime.now() - timedelta(days=0)
day_str = day.strftime("%Y-%m-%d")




gm_conn = NewGMDBConn()

server_list = list()

try:
    sql = (datetime.now() - timedelta(days=8)).strftime("select server_id from server where from_unixtime(server_start_time) > '%Y-%m-%d'")
    cur = gm_conn.cursor()
    cur.execute(sql)
    for (server) in cur:
        server_list.append(int(server[0]))
    cur.close()
    pass
except:
    pass

if len(server_list) < 1:
    exit(0)
sql_template = "select server,uid,openid,name,create_time,last_login_time,level,vip_level," \
          "total_recharge/100 as recharge, money, max_fight_attr from player_%s as player "\
          "inner join tactic_%s as tactic on player.uid = tactic.player_id where server in %s"
s = str(server_list).replace('[', '(').replace(']', ')')

output_path = r"/usr/share/nginx/html/report/"

file = open("%snew_player_%s.csv" % (output_path, day_str), "w", encoding='utf-8')
#用户名 创建时间 最后登录时间 VIP 等级 充值金额  剩余黄金 战力
file.write("Server,PlayerID,OpenID,Name,CreateTime,LastLoginTime,Level,VipLevel,Recharge,Money,FightAttr")
conn = NewDBConn()

for x in range(8):
    cur = conn.cursor()
    sql = sql_template % (x, x, s)
    try:
        cur.execute(sql)
        for (server, uid, openid, name, create_time, last_login_time, level, vip_level, recharge, money, fight_attr) in cur:
            file.write("\n%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s" % (server, uid, openid, name, create_time, last_login_time, level, vip_level, recharge, money, fight_attr))
    except:
        pass
    finally:
        cur.close()

file.close()
conn.close()
gm_conn.close()
