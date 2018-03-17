#!/usr/bin/python3
# coding:utf-8

from mysql_config import *
import re

player_id = 15909
backup_player_id = 14212
address = ['192.168.16.102', 3306, 'root', '1q2w3e', 'zhanjian']
back_address = ['192.168.16.102', 3306, 'root', '1q2w3e', 'zhanjian']
conn = pymysql.connect(host=address[0], port=address[1], user=address[2], password=address[3],
                       database=address[4], charset='utf8', autocommit=True)
backup_conn = pymysql.connect(host=address[0], port=address[1], user=address[2], password=address[3],
                              database=address[4], charset='utf8', autocommit=True)

cursor = conn.cursor()
backup_cursor = backup_conn.cursor()
cursor.execute("show tables like 'player_%%'")
backup_cursor.execute("show tables like 'player_%%'")

table_index = player_id % cursor.rowcount
backup_table_index = backup_player_id % backup_cursor.rowcount


def convert_val(value):
    if value is None:
        return 'NULL'
    if type(value) is bytes:
        value = bytes.decode(value)
    return "'%s'" % conn.escape_string(str(value))


def str_replace(str_s, express):
    try:
        str_s %= express
    except TypeError:
        pass
    return str_s


def is_field(index, field_name, cur):
    for j, tup in enumerate(cur.description):
        if j == index and field_name == tup[0]:
            return True
    return False


def is_player_id(index, table_name, cur):
    return is_field(index, 'uid' if 0 == table_name.find("player_") else 'player_id', cur)


except_tables = ('activity', 'activity_mould', 'activity_new',  'ip_list',
                 'server_notice', 'server_shop', 'server_mail',
                 'army', 'army_apply', 'army_member',
                 'pk_rank_list', 'rank_list_details', 'rank_player_details', 'research_hero_log',
                 'recharge', 'recharge_details', 'recharge_openid', 'recharge_uid', 'role_name')
cursor.execute("show tables")
all_tables = set()
for table in cursor:
    if table[0] not in except_tables and table[0].find('friend_') < 0:
        all_tables.add(re.sub(r'_\d', '_%d', table[0]))

delete_sql = list()
insert_sql = list()
for table in all_tables:
    army_id = 0
    server_id = 0
    openid = ''
    player_name = ''
    sql = "select `openid`, `name`, `server` from player_%d where uid = %d" % (table_index, player_id)
    cursor.execute(sql)
    for row in cursor:
        openid = row[0]
        player_name = row[1]
        server_id = row[2]
    sql = "select army_id from tactic_%d where player_id = %d" % (table_index, player_id)
    cursor.execute(sql)
    for row in cursor:
        army_id = row[0]

    condition_field = 'uid' if 0 == table.find('player_') or 0 == table.find('role_name') else 'player_id'
    del_sql = 'delete from `%s` where %s = %d' % (table, condition_field, player_id)
    delete_sql.append(str_replace(del_sql, table_index))

    sql = 'select * from `%s` where %s = %d' % (table, condition_field, backup_player_id)
    backup_cursor.execute(str_replace(sql, backup_table_index))
    if backup_cursor.rowcount <= 0:
        continue
    ist_sql = "insert into `%s` (" % table
    for desc in backup_cursor.description:
        ist_sql += '`' + desc[0] + '`,'
    ist_sql = ist_sql.rstrip(',')
    ist_sql += ") values"

    for row in backup_cursor:
        ist_sql += "("
        for i, val in enumerate(row):
            if is_player_id(i, table, backup_cursor):
                val = player_id
            if table.find("tactic_") == 0 and is_field(i, "army_id", backup_cursor):
                val = army_id
            if table.find("player_") == 0 and is_field(i, "name", backup_cursor):
                val = player_name
            if table.find("player_") == 0 and is_field(i, "server", backup_cursor):
                val = server_id
            if table.find("player_") == 0 and is_field(i, "openid", backup_cursor):
                val = openid
            ist_sql += convert_val(val) + ","
        ist_sql = ist_sql.rstrip(',')
        ist_sql += "),"
    ist_sql = ist_sql.rstrip(',')
    insert_sql.append(str_replace(ist_sql, table_index))

for sql in delete_sql:
    print(sql)
    cursor.execute(sql)
for sql in insert_sql:
    print(sql)
    cursor.execute(sql)

conn.close()
backup_conn.close()
