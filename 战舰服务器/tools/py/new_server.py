#!/usr/bin/python3
#coding=utf-8

import sys
import os
import urllib.request as request
import time
from mysql_config import *

IP_URL = 'http://119.29.52.145:18800/ipaddr'
ROOT_PATH = '/data/server'

conn = NewGMDBConn()
response = request.urlopen(IP_URL)
local_ip = response.read().decode("utf-8")


ServerIDBegin = int(input("请输入开始的服务器ID:"))
ServerCount = int(input("请输入开启的服务器个数:"))

ServerList = []
for x in range(ServerCount):
    ServerList.append(ServerIDBegin + x)

print(local_ip)
print(ServerList)


sql_template = """INSERT INTO `server` (`opgame_id`, `server_id`, `server_name`, `server_ip`, `server_port`, `server_start_time`, 
`db_game_name`, `db_log_name`, `dbwip`, `dbwuser`, `dbwpassword`, `dbwport`, `http_get_status`)
VALUES (%s, '%s', '%s', '%s', %s, %s, 'zhanjian', 'zhanjian_log', '%s', '%s', '%s', %s, 1)"""

def GenerateLogicServerConfig(server_list):
    for server in server_list:
        server_id = int(server)
        platform = server_id // 10000
        listen = server_id % 10000 + 10000
        server_name = "%s服" % server_id
        server_begin_time = int(time.time()) + 2 * 365 * 24 * 3600
        print('Begin Generate LogicServer %s config' % server_id)
        os.system('mkdir -p %s/%s/config' % (ROOT_PATH, server_id))
        file = open('%s/%s/logic.xml' % (ROOT_PATH, server_id), 'w')
        file.write("""<?xml version="1.0" encoding="UTF-8"?>
<root>
 <listen port="%s" type="client"/>
 <log_level level="0" />
 <report enable="0" enable_attr="0"/>
 <dirtywords enable="1"/>
 <recharge platform="1"/>
 <auth_server ip="127.0.0.1" port="8888"/>
 <record_server ip="127.0.0.1" port="3724"/>
 <center_server ip="10.186.3.155" port="8887"/>
 <server_id id="%s">
  <server id="%s"/>
 </server_id>
</root>""" % (listen, server_id, server_id))
        cur = conn.cursor()
        try:
            sql = sql_template % (platform, server_id, server_name, local_ip, listen, server_begin_time, \
                                  mysql_host, mysql_user, mysql_password, mysql_port)
            cur.execute(sql)
        except:
            print("%s已经存在" % server_id)
            pass
        finally:
            cur.close()
    pass

def GenerateRecordServerConfig():
    os.system('mkdir -p %s/record' % ROOT_PATH)
    file = open('%s/record/record.xml' % ROOT_PATH, 'w')
    file.write("""<?xml version="1.0" encoding="UTF-8"?>
<root>
  <listen port="3724" type="server"/>
  <log_level level="0" />
  <mysql ip="%s" port="%s" db_name="zhanjian" user_name="%s" password="%s"/>
</root>""" % (mysql_host, mysql_port, mysql_user, mysql_password))
    file.close()
    pass

def GenerateAuthServerConfig():
    os.system('mkdir -p %s/auth' % ROOT_PATH)
    file = open('%s/auth/auth.xml' % ROOT_PATH, 'w')
    file.write("""<?xml version="1.0" encoding="UTF-8"?>
<root>
  <listen port="8888" type="server"/>
  <log_level level="0" />
  <mysql ip="%s" port="%s" db_name="auth_db" user_name="%s" password="%s"/>
</root>""" % (mysql_host, mysql_port, mysql_user, mysql_password))
    file.close()
    pass


GenerateLogicServerConfig(ServerList)
GenerateAuthServerConfig()
GenerateRecordServerConfig()