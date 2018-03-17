#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
import re
import sys

ServerID = 2

#备份库设置
backup_db_host = '192.168.16.102'
backup_db_port = 3306
backup_db_user = 'root'
backup_db_password = '1q2w3e'
backup_db_db = 'zhanjian'

if len(sys.argv) > 1:
    ServerID = int(sys.argv[1])
    retry_server_id = input("再次确认服务器ID:")
    if ServerID != int(retry_server_id):
        print("两次输入的服务器ID不一致")
        exit(0)
    pass
else:
    print("请在命令行参数写上服务器ID")
    exit(0)
    pass

backup_conn = pymysql.connect(host=backup_db_host, port=backup_db_port, user=backup_db_user,
                              password=backup_db_password, database=backup_db_db, charset='utf8',
                              autocommit=True)
conn = NewDBConn()

regex = re.compile('  (`.*`).*,\n')

#1. player_表主键是uid
GamePlayerTable = ['player_', 'activity_record_', 'activity_record_new_', 'carrier_', 'copy_', 'copy_star_', 'friend_', 'hero_',
                   'item_', 'mail_', 'new_shop_', 'report_', 'reward_', 'shop_', 'tactic_',
                   ]
#2. 直接按照ServerID删除
GameServerTable = {'army': 'server', 'army_apply': 'server_id',
                   'army_member': 'server', 'dstrike_boss': 'server_id', 'pk_rank_list': 'server',
                   'rank_list_details': 'server_id',  'server_mail': 'server_id', 'server_notice': 'server_id',
                   'server_shop': 'server_id',
                   }
#3. 和role_name表inner join删除
GameSinglePlayerTable = ['rank_player_details']
#4. role_name表

#(insert sql, select sql)
sql_list = list()


for table_prefix in GamePlayerTable:
    sql = 'show create table `%s0`' % table_prefix
    cur = backup_conn.cursor()
    cur.execute(sql)
    for x in cur:
        create_table = x[1]
        columns = regex.findall(create_table)
        for index in range(8):
            table_name = "%s%s" % (table_prefix, index)
            insert_columns = ', '.join(columns)
            select_columns = 't' + "." + (', t.'.join(columns))
            player_id = 'player_id'
            if table_prefix == 'player_':
                player_id = 'uid'
            insert_sql = 'insert into %s (%s) values' % (table_name, insert_columns)
            select_sql = 'select %s from %s as t inner join role_name on t.%s = role_name.uid where role_name.server="%s"' % (select_columns, table_name, player_id, ServerID)
            sql_list.append((insert_sql, select_sql))


for table_name in GameServerTable:
    sql = 'show create table `%s`' % table_name
    server_id = GameServerTable[table_name]
    cur = backup_conn.cursor()
    cur.execute(sql)
    for x in cur:
        create_table = x[1]
        columns = regex.findall(create_table)
        insert_columns = ', '.join(columns)
        select_columns = 't' + "." + (', t.'.join(columns))
        insert_sql = 'insert into %s (%s) values' % (table_name, insert_columns)
        select_sql = 'select %s from %s as t where %s="%s"' % (select_columns, table_name, server_id, ServerID)
        sql_list.append((insert_sql, select_sql))


for table_name in GameSinglePlayerTable:
    sql = 'show create table `%s`' % table_name
    cur = backup_conn.cursor()
    cur.execute(sql)
    for x in cur:
        create_table = x[1]
        player_id = 'player_id'
        columns = regex.findall(create_table)
        insert_columns = ', '.join(columns)
        select_columns = 't' + "." + (', t.'.join(columns))
        insert_sql = 'insert into %s (%s) values' % (table_name, insert_columns)
        select_sql = 'select %s from %s as t inner join role_name on t.%s = role_name.uid where role_name.server="%s"' % (select_columns, table_name, player_id, ServerID)
        sql_list.append((insert_sql, select_sql))


cur = backup_conn.cursor()
try:
    table_name = 'role_name'
    cur.execute('show create table `%s`' % table_name)
    for x in cur:
        create_table = x[1]
        columns = regex.findall(create_table)
        insert_columns = ', '.join(columns)
        select_columns = 't' + "." + (', t.'.join(columns))
        insert_sql = 'insert into %s (%s) values' % (table_name, insert_columns)
        select_sql = 'select %s from %s as t where t.server="%s"' % (select_columns, table_name, ServerID)
        sql_list.append((insert_sql, select_sql))
except:
    pass
finally:
    cur.close()

real_sql_list = list()

def GetEscapeStr(v):
    if v is None:
        return "''"
    if isinstance(v, str):
        return "'" + backup_conn.escape_string(v) + "'"
    if isinstance(v, bytes):
        s = v.decode('utf8')
        return "'" + backup_conn.escape_string(s) + "'"
    return "'" + str(v) + "'"

for (insert_sql, select_sql) in sql_list:
    backup_cur = backup_conn.cursor()
    try:
        backup_cur.execute(select_sql)
        for value in backup_cur:
            l = list()
            l.append('(')
            for index in range(len(value)):
                if len(l) != 1:
                    l.append(',')
                l.append(GetEscapeStr(value[index]))
            l.append(')')
            real_sql_list.append(insert_sql + " " + ("".join(l)) + ";")
    except:
        pass
    finally:
        backup_cur.close()

print("开始从备份库导入数据到主库")
for s in real_sql_list:
    cur = conn.cursor()
    try:
        cur.execute(s)
        print("ExecSQL, %s, %s" % ('SUCCESS', s))
    except:
        print("ExecSQL, %s, %s" % ('FAIL', s))
        pass
    finally:
        cur.close()
print("导入结束")
