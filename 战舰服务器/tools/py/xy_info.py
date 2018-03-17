#!/usr/bin/python3
# coding=utf-8

import os
import smtplib
import time
from mysql_config import *
from email.mime.multipart import MIMEMultipart
from email.mime.base import MIMEBase
from email import encoders
from datetime import timedelta, datetime

ROOT = "/usr/share/nginx/html/xy"

day = datetime.now() - timedelta(days=0)
day_now_str = "%04d-%02d-%02d" % (day.year, day.month, day.day)

def SendMail(from_address, to_address, subject, content, attach_file):
    print(to_address)
    message = MIMEMultipart()
    message['From'] = from_address
    message['To'] = to_address
    message['Subject'] = subject

    part = MIMEBase('application', "octet-stream")
    part.set_payload(open(attach_file, "rb").read())
    encoders.encode_base64(part)
    part.add_header('Content-Disposition', 'attachment; filename="{0}"'.format(os.path.basename(attach_file)))
    message.attach(part)

    msg_full = message.as_string()
    with smtplib.SMTP("smtp.mxhichina.com") as s:
        s.login("zjry_log@xinghehudong.com", "P@ssw0rd2013")
        s.sendmail(from_address, [to_address], msg_full)
        s.quit()
        pass

conn = NewDBConn()
conn_gm = NewGMDBConn()
conn_auth = NewAuthDBConn()

class StatCount:
    def __init__(self):
        self.online = 0
        self.new_player = 0
        self.create_player = 0
        self.pay_player = 0
        self.pay_amount = 0

    def __str__(self):
        return "(%s,%s,%s,%s,%s)" % (self.online, self.new_player, self.create_player, self.pay_player, self.pay_amount)

time_now = int(time.time())
seconds = int(time.time()) - 60*10
server_map = dict()

#获取在线用户数
def GetOnlineCount():
    sql = "SELECT server_id, sum(online_num)/count(online_num) from gm_online_info where cur_time >= %s GROUP BY server_id" % seconds
    cur = conn_gm.cursor()
    cur.execute(sql)
    for (server, online) in cur:
        if server not in server_map:
            server_map[server] = StatCount()
        server_map[server].online = int(online)
    cur.close()
    pass

#获取注册用户数
def GetNewPlayer():
    sql_template ="SELECT server, count(uid) from account_%s where openid like \"XY_%%\" and create_time >= %s GROUP BY server"
    cur = conn_auth.cursor()
    for table_index in range(16):
        sql = sql_template % (table_index, seconds)
        cur.execute(sql)
        for (server, player_count) in cur:
            if server not in server_map:
                server_map[server] = StatCount()
            server_map[server].new_player += player_count
    cur.close()
    pass

#获取10分钟内的创角数
def GetCreatePlayer():
    sql_template = "SELECT server, count(uid) from player_%s where openid like \"XY_%%\" and create_time >= %s GROUP BY server"
    cur = conn.cursor()
    for table_index in range(8):
        try:
            sql = sql_template % (table_index, seconds)
            cur.execute(sql)
            for (server, create_count) in cur:
                if server not in server_map:
                    server_map[server] = StatCount()
                server_map[server].create_player += create_count
        except:
            pass
        finally:
            pass
    cur.close()
    pass

#获取付费用户数/付费金额
def GetPayPlayer():
    sql_template = "select server_id, count(DISTINCT role_id), sum(pay_amount) from recharge_details where recharge_time >= %s and stage = 3 and user_id like \"XY_%%\" and channel_id != 'rsdkdebug' group by server_id"
    sql = sql_template % seconds
    cur = conn.cursor()
    cur.execute(sql)
    for (server, pay_count, pay_amount) in cur:
        if server not in server_map:
            server_map[server] = StatCount()
        server_map[server].pay_player += pay_count
        server_map[server].pay_amount += pay_amount
    cur.close()
    pass

GetOnlineCount()
GetNewPlayer()
GetCreatePlayer()
GetPayPlayer()


file_now = open("%s/xy_%s.txt" % (ROOT, day_now_str), "a+", encoding='utf8')
seq = int(time.time() * 1000)
for x in server_map:
    if x <= 20001:
        continue
    if x >= 30000 and x <= 49999:
        continue
    seq += 1
    o = server_map[x]
    if file_now.tell() <= 0:
        file_now.write("序号(id)\t时间(time_ts)\t游戏服务器序号(server_id)\t在线用户人数(online_user)\t注册用户数(resgiter_user)\t创角用户数(create_role_user)\t付费用户数(charde_user)\t付费金额(rmb)\n")
    file_now.write("%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" % (seq, time_now, x, o.online, o.new_player, o.create_player, o.pay_player, o.pay_amount / 100))

    if day.hour == 0 and day.minute < 10:
        yesterday = datetime.now() - timedelta(days=1)
        yesterday_str = "%04d-%02d-%02d" % (yesterday.year, yesterday.month, yesterday.day)
        file = open("%s/xy_%s.txt" % (ROOT, yesterday_str), "a+", encoding="utf8")
        file.write("%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" % (seq, time_now, x, o.online, o.new_player, o.create_player, o.pay_player, o.pay_amount / 100))
        file.close()

file_now.close()


file_name = "%s/xy_%s.txt" % (ROOT, day_now_str)
# tintinding@xinghehudong.com
# shawnchen@xinghehudong.com
# SendMail("zjry_log@xinghehudong.com", "wangxiaokang@goodplay.com", "战舰荣耀数据上报", "", file_name)
# SendMail("zjry_log@xinghehudong.com", "tintinding@xinghehudong.com", "战舰荣耀数据上报", "", file_name)
# SendMail("zjry_log@xinghehudong.com", "shawnchen@xinghehudong.com", "战舰荣耀数据上报", "", file_name)

