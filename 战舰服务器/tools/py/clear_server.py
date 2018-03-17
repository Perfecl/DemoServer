#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
import sys

ServerID = 0

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

game_conn = NewDBConn()
auth_conn = NewAuthDBConn()
log_conn = NewLogDBConn()

print("开始删除服务器%s的数据" % ServerID)
#1. 和role_name表inner join删除
# player_表主键是uid
GamePlayerTable = ['activity_record_', 'activity_record_new_', 'carrier_', 'copy_', 'copy_star_', 'friend_', 'hero_', \
                   'item_', 'mail_', 'new_shop_', 'report_', 'reward_', 'shop_', 'tactic_', 'player_'\
                   ]

#2. 直接按照ServerID删除
GameServerTable = {'activity': 'server_id', 'activity_new': 'server_id', 'army': 'server', 'army_apply': 'server_id', \
                   'army_member': 'server', 'dstrike_boss': 'server_id', 'ip_list': 'server_id', 'pk_rank_list': 'server',\
                   'rank_list_details': 'server_id', 'server_mail': 'server_id', 'server_notice': 'server_id',\
                   'server_shop': 'server_id', 'recharge_openid': 'server', 'recharge_uid': 'server', 'research_hero_log': 'server',\
                   }

#3. 和role_name表inner join删除
GameSinglePlayerTable = ['recharge', 'rank_player_details']
#4. 最后删除role_name表

def DeleteGameDB():
    for player_table_name in GamePlayerTable:
        player_id = "player_id"
        if player_table_name == 'player_':
            player_id = 'uid'
        for index in range(8):
            table_name = player_table_name + ('%s' % index)
            sql = 'delete %s.* from %s inner join role_name on %s.%s = role_name.uid where role_name.server="%s"' % (table_name, table_name, table_name, player_id, ServerID)
            cur = game_conn.cursor()
            try:
                rows = cur.execute(sql)
                print("delete from %s, %s rows affected" % (table_name, rows))
            except:
                print("delete from %s, except" % table_name)
            finally:
                cur.close()

def DeleteServerDB():
    for table_name in GameServerTable:
        sql = 'delete from %s where %s=%s' % (table_name, GameServerTable[table_name], ServerID)
        cur = game_conn.cursor()
        try:
            rows = cur.execute(sql)
            print("delete from %s, %s rows affected" % (table_name, rows))
        except:
            print('delete from %s, except' % table_name)
        finally:
            cur.close()

def DeletePlayerSingleDB():
    for table_name in GameSinglePlayerTable:
        sql = 'delete %s.* from %s inner join role_name on %s.player_id = role_name.uid where role_name.server=%s' % (table_name, table_name, table_name, ServerID)
        cur = game_conn.cursor()
        try:
            rows = cur.execute(sql)
            print("delete from %s, %s rows affected" % (table_name, rows))
        except:
            print('delete from %s, except' % table_name)
        finally:
            cur.close()

def DeleteRoleNameDB():
    sql = 'delete from role_name where server=%s' % ServerID
    cur = game_conn.cursor()
    try:
        rows = cur.execute(sql)
        print("delete from %s, %s rows affected" % ('role_name', rows))
    except:
        print('delete from %s, except' % 'role_name')
    finally:
        cur.close()

def DeleteAuthDB():
    for index in range(16):
        sql = "delete from account_%s where server=%s" % (index, ServerID)
        cur = auth_conn.cursor()
        try:
            rows = cur.execute(sql)
            print("delete from account_%s, %s rows affected" % (index, rows))
        except:
            print('delete from %s%s, except' % ('account_', index))
        finally:
            cur.close()

DeleteGameDB()
DeleteServerDB()
DeletePlayerSingleDB()
DeleteRoleNameDB()
DeleteAuthDB()


game_conn.close()
auth_conn.close()
