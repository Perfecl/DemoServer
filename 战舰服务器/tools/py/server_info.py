#!/usr/bin/python3
#coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime

conn_log = NewLogDBConn()
conn_gm = NewGMDBConn()
conn_auth = NewAuthDBConn()
conn_db = NewDBConn()

day = datetime.now() + timedelta(days=0)
tomorrow = datetime.now() + timedelta(days=1)
today_str = "%04d-%02d-%02d" % (day.year, day.month, day.day)
tomorrow_str = "%04d-%02d-%02d" % (tomorrow.year, tomorrow.month, tomorrow.day)


def ExecGMSQL(sql):
    cur = conn_gm.cursor()
    try:
        cur.execute(sql)
        for x in cur:
            pass
    except:
        pass
    finally:
        cur.close()
    pass

def GetMaxOnline():
    online_dict = dict()
    # 最高在线
    max_online_sql = """
    select server_id, online_num from gm_online_info
    where from_unixtime(cur_time) >= '%s' and from_unixtime(cur_time) < '%s' and online_num != 0
    group by server_id, online_num""" % (today_str, tomorrow_str)

    cur = conn_gm.cursor()
    try:
        cur.execute(max_online_sql)
        for (server_id, online_num) in cur:
            if server_id not in online_dict:
                online_dict[server_id] = 0
            if online_dict[server_id] < int(online_num):
                online_dict[server_id] = int(online_num)
        for server_id in online_dict.keys():
            #print("GetMaxOnline, 最高在线", server_id, online_num)
            sql = """
            insert into gm_day_server_info (server_id, date_time, top_hour_online) values ('%s', '%s', '%s')
            on DUPLICATE KEY UPDATE top_hour_online=values(top_hour_online)""" % (server_id, today_str, online_dict[server_id])
            ExecGMSQL(sql)
    except:
        print("GetMaxOnline Fail")
    finally:
        cur.close()
    pass

def GetAvgOnline():
    count = datetime.now().hour * 12 + int(datetime.now().minute / 5)
    if count == 0:
        return
    avg_online_sql = """
    select server_id, sum(online_num) from gm_online_info
    where from_unixtime(cur_time) >= '%s' and from_unixtime(cur_time) < '%s'
    group by server_id, online_num """ % (today_str, tomorrow_str)
    cur = conn_gm.cursor()
    d = dict()
    try:
        cur.execute(avg_online_sql)
        for (server_id, sum) in cur:
            if server_id not in d:
                d[server_id] = 0
            d[server_id] += int(sum)
        pass
        for server_id in d.keys():
            #print("GetAvgOnline,平均在线数", server_id, d[server_id], count)
            sql = """
            insert into gm_day_server_info (server_id, date_time, equally_hour_online) values ('%s', '%s', '%s')
            on DUPLICATE KEY UPDATE equally_hour_online=values(equally_hour_online)""" % (server_id, today_str, int(d[server_id] / count))
            ExecGMSQL(sql)
    except:
        pass
    finally:
        cur.close()

def GetAvgOnlineTime():
    login_count_sql = "select server, count(1) from (select server, player_id from `login_%s` group by server, player_id ) as t group by server" % today_str
    online_time_sql = "select server, sum(online_time) from `login_%s` group by server" % today_str
    #server => (online_time, count)
    online_dict = dict()
    cur_count = conn_log.cursor()
    cur_time = conn_log.cursor()
    try:
        cur_count.execute(login_count_sql)
        cur_time.execute(online_time_sql)
        for (server, count) in cur_count:
            if server not in online_dict:
                online_dict[server] = (0, 0)
            (k1, k2) = online_dict[server]
            online_dict[server] = (k1, int(count))
            pass
        for (server, time) in cur_time:
            if server not in online_dict:
                online_dict[server] = (0, 0)
            (k1, k2) = online_dict[server]
            online_dict[server] = (int(time), k2)
            pass
        for server in online_dict.keys():
            (online_time, count) = online_dict[server]
            #print("GetAvgOnlineTime,平均在线时间", server, online_time, count)

            if count == 0:
                continue
            sql = """
            insert into gm_day_server_info (server_id, date_time, player_dt_online) values ('%s', '%s', '%s')
            on DUPLICATE KEY UPDATE player_dt_online=values(player_dt_online)""" % (server, today_str, int(online_time / count / 60))
            ExecGMSQL(sql)
    except:
        pass
    finally:
        cur_count.close()
        cur_time.close()
    pass

def GetIpCount():
    ip_count_sql = "select server, count(1) from (select server, ipaddr from `login_%s` where login=1 group by server, ipaddr ) as t group by server" % today_str
    cur = conn_log.cursor()
    try:
        cur.execute(ip_count_sql)
        for (server, ip_count) in cur:
            #print("GetIpCount,获取IP数", server, ip_count)
            sql = """
            insert into gm_day_server_info (server_id, date_time, connect_ip) values ('%s', '%s', '%s')
            on DUPLICATE KEY UPDATE connect_ip=values(connect_ip)""" % (server, today_str, ip_count)
            ExecGMSQL(sql)
    except:
        pass
    finally:
        cur.close()
    pass

def GetLoginAccountCount():
    account_dict = dict()
    login_account_sql = """
    select server, count(1) from (
    select server, openid from account_%s where from_unixtime(last_login_time) >= '%s' and from_unixtime(last_login_time) < '%s' group by server, openid
    ) as t group by server
    """
    for index in range(16):
        cur = conn_auth.cursor()
        try:
            account_sql = login_account_sql % (index, today_str, tomorrow_str)
            cur.execute(account_sql)
            for (server, count) in cur:
                if server not in account_dict:
                    account_dict[server] = 0
                account_dict[server] += int(count)
                pass
        except:
            pass
        finally:
            cur.close()
    for server in account_dict:
        #print("GetLoginAccountCount,当日登录账号", server, account_dict[server])
        sql = """
        insert into gm_day_server_info (server_id, date_time, login_account) values ('%s', '%s', '%s')
        on DUPLICATE KEY UPDATE login_account=values(login_account)""" % (server, today_str, account_dict[server])
        ExecGMSQL(sql)
    return account_dict

def GetOldLoginAccountCount():
    account_dict = dict()
    login_account_sql = """
    select server, count(1) from (
        select server, openid from account_%s
        where from_unixtime(last_login_time) >= '%s' and from_unixtime(last_login_time) < '%s' and from_unixtime(create_time) < '%s'
        group by server, openid
    ) as t group by server
    """
    for index in range(16):
        cur = conn_auth.cursor()
        try:
            account_sql = login_account_sql % (index, today_str, tomorrow_str, today_str)
            cur.execute(account_sql)
            for (server, count) in cur:
                if server not in account_dict:
                    account_dict[server] = 0
                account_dict[server] += int(count)
                pass
        except:
            pass
        finally:
            cur.close()
    for server in account_dict:
        #print("GetOldLoginAccountCount,当日老账号登录", server, account_dict[server])
        sql = """
        insert into gm_day_server_info (server_id, date_time, old_login_account) values ('%s', '%s', '%s')
        on DUPLICATE KEY UPDATE old_login_account=values(old_login_account)""" % (server, today_str, account_dict[server])
        ExecGMSQL(sql)
    pass

def GetNewAccountCount():
    account_dict = dict()
    login_account_sql = """
    select server, count(1) from (
        select server, openid from account_%s
        where from_unixtime(create_time) >= '%s' and from_unixtime(create_time) < '%s'
        group by server, openid
    ) as t group by server
    """
    for index in range(16):
        cur = conn_auth.cursor()
        try:
            account_sql = login_account_sql % (index, today_str, tomorrow_str)
            cur.execute(account_sql)
            for (server, count) in cur:
                if server not in account_dict:
                    account_dict[server] = 0
                account_dict[server] += int(count)
                pass
        except:
            pass
        finally:
            cur.close()
    for server in account_dict:
        #print("GetNewAccountCount,当日注册账号", server, account_dict[server])
        sql = """
        insert into gm_day_server_info (server_id, date_time, new_account) values ('%s', '%s', '%s')
        on DUPLICATE KEY UPDATE new_account=values(new_account)""" % (server, today_str, account_dict[server])
        ExecGMSQL(sql)
    pass

def GetCreatePlayerCount():
    account_dict = dict()
    newplayer_sql = """select server, count(1) from player_%s where from_unixtime(create_time) >= '%s' and from_unixtime(create_time) < '%s' group by server"""
    for index in range(16):
        cur = conn_db.cursor()
        try:
            account_sql = newplayer_sql % (index, today_str, tomorrow_str)
            cur.execute(account_sql)
            for (server, count) in cur:
                if server not in account_dict:
                    account_dict[server] = 0
                account_dict[server] += int(count)
                pass
        except:
            pass
        finally:
            cur.close()
    for server in account_dict:
        #print("GetCreatePlayerCount,当日创建角色", server, account_dict[server])
        sql = """
        insert into gm_day_server_info (server_id, date_time, activation_account) values ('%s', '%s', '%s')
        on DUPLICATE KEY UPDATE activation_account=values(activation_account)""" % (server, today_str, account_dict[server])
        ExecGMSQL(sql)
    pass

#获取充值账号数
def GetRechargeAcountCount():
    recharge_sql = """
    select server_id, count(distinct user_id) from recharge_details
    where from_unixtime(recharge_time) >= '%s'
    and channel_id != 'rsdkdebug'
    and stage=3
    group by server_id, user_id
    """ % today_str
    account_dict = dict()
    cur = conn_db.cursor()
    try:
        cur.execute(recharge_sql)
        for (server, count) in cur:
            if server not in account_dict:
                account_dict[server] = 0
            account_dict[server] += count
            pass
    except:
        pass
    finally:
        cur.close()
    for server in account_dict:
        sql = """
        insert into gm_day_server_info (server_id, date_time, charge_account) values ('%s', '%s', '%s')
        on DUPLICATE KEY UPDATE charge_account=values(charge_account)""" % (server, today_str, account_dict[server])
        ExecGMSQL(sql)
    pass

#获取累计充值账号数
def GetTotalRechargeAccount():
    total_recharge_sql = "select server, count(*) from player_%s where total_recharge > 0 group by server"
    server_dict = dict()
    for index in range(16):
        sql = total_recharge_sql % index
        cur = conn_db.cursor()
        try:
            cur.execute(sql)
            for (server, count) in cur:
                if server not in server_dict:
                    server_dict[server] = 0
                server_dict[server] += count
        except:
            pass
        finally:
            cur.close()
    for server in server_dict:
        sql = """
        insert into gm_day_server_info (server_id, date_time, total_charge_account) values ('%s', '%s', '%s')
        on DUPLICATE KEY UPDATE total_charge_account=values(total_charge_account)""" % (server, today_str, server_dict[server])
        ExecGMSQL(sql)
    pass

#获取新增充值账号数
def GetNewRechargeAcountCount():
    account_sql = """select server, count(*) from recharge_openid where from_unixtime(recharge_time) >= '%s' group by server""" % today_str
    accout_dict = dict()
    cur = conn_db.cursor()
    try:
        cur.execute(account_sql)
        for (server, count) in cur:
            if server not in accout_dict:
                accout_dict[server] = 0
            accout_dict[server] += count
    except:
        pass
    finally:
        cur.close()
    for server in accout_dict:
        sql = """
        insert into gm_day_server_info (server_id, date_time, new_charge_account) values ('%s', '%s', '%s')
        on DUPLICATE KEY UPDATE new_charge_account=values(new_charge_account)""" % (server, today_str, accout_dict[server])
        ExecGMSQL(sql)
    pass

#新增充值玩家数量
#获取新增充值金额
def GetNewRechargeMoney():
    player_id_sql = """select player_id from recharge_uid where from_unixtime(recharge_time) >= '%s'""" % today_str
    player_id_set = set()
    cur = conn_db.cursor()
    try:
        cur.execute(player_id_sql)
        for player_id in cur:
            player_id_set.add(player_id)
    except:
        pass
    finally:
        cur.close()
    count = 0
    sql_list = list()
    for player in player_id_set:
        if count == 0:
            sql_list.append("""select server_id, role_id, game_coin from recharge_details
                            where from_unixtime(recharge_time) >= '%s' and stage = 3 and channel_id != 'rsdkdebug' and role_id in (""" % today_str)
        if count != 0:
            sql_list.append(',')
        sql_list.append('%s' % player)
        count += 1
    sql_list.append(")")
    sql = "".join(sql_list)
    cur = conn_db.cursor()
    #server => (set, game_coin)
    server_dict = dict()
    try:
        cur.execute(sql)
        for (server, player_id, game_coin) in cur:
            if server not in server_dict:
                server_dict[server] = (set(), 0)
            (_1, _2) = server_dict[server]
            _1.add(player_id)
            _2 += game_coin

            server_dict[server] = (_1, _2)
    except:
        pass
    finally:
        cur.close()
    for server in server_dict:
        sql = """
        insert into gm_day_server_info (server_id, date_time, new_charge_times, new_charge_value) values ('%s', '%s', '%s', '%s')
        on DUPLICATE KEY UPDATE new_charge_times=values(new_charge_times), new_charge_value=values(new_charge_value)""" % (server, today_str, len(server_dict[server][0]), server_dict[server][1])
        ExecGMSQL(sql)
    pass

#获取今日充值人数和充值金额
def GetRechargePlayerCount():
    sql = """select server_id, count(*), sum(game_coin), count(DISTINCT role_id) from recharge_details
            where from_unixtime(recharge_time)>='%s' and stage=3 and channel_id != 'rsdkdebug'
            group by server_id""" % today_str
    cur = conn_db.cursor()
    server_dict = dict()
    try:
        cur.execute(sql)
        for (server, recharge_times, money, player_count) in cur:
            if server not in server_dict:
                server_dict[server] = (0, 0, player_count)
            server_dict[server] = (recharge_times, money, player_count)
    except:
        pass
    finally:
        cur.close()
    for server in server_dict:
        sql = """insert into gm_day_server_info (server_id, date_time, charge_times, charge_value) values ('%s', '%s', '%s', '%s')
                 on DUPLICATE KEY UPDATE charge_times=values(charge_times), charge_value=values(charge_value)""" % (server, today_str, server_dict[server][0], server_dict[server][1])
        ExecGMSQL(sql)
    return server_dict

#获取付费率(今天付费人数/登录人数)
#获取ARRPU(充值金额/付费人数)
#获取ARPU(充值金额/登录人数)
def GetRechargeRate(login_account_dict, recharge_player_dict):    #server_id => (rate, arppu, arpu)
    server_dict = dict()
    print(login_account_dict)
    print(recharge_player_dict)
    for server in login_account_dict:
        if server not in server_dict:
            server_dict[server] = (0, 0, 0)
    for server in recharge_player_dict:
        if server not in server_dict:
            server_dict[server] = (0, 0, 0)
    for server in server_dict:
        pay_player_count = 0
        payment_money = 0
        login_player_count = 0
        (_1, _2, _3) = (0, 0, 0)
        if server in recharge_player_dict:
            pay_player_count = recharge_player_dict[server][2]
            payment_money = recharge_player_dict[server][1] / 10
        if server in login_account_dict:
            login_player_count = login_account_dict[server]
        print(login_player_count, pay_player_count, payment_money)
        if login_player_count != 0:
            _1 = pay_player_count / login_player_count
        if pay_player_count != 0:
            _2 = payment_money / pay_player_count
        if login_player_count != 0:
            _3 = payment_money / login_player_count
        server_dict[server] = (_1, _2, _3)
    for server in server_dict:
        sql = """insert into gm_day_server_info (server_id, date_time, pay_rate, arppu, arpu) values ('%s', '%s', '%s', '%s', '%s')
                 on DUPLICATE KEY UPDATE pay_rate=values(pay_rate), arppu=values(arppu), arpu=values(arpu)""" % (server, today_str, server_dict[server][0], server_dict[server][1], server_dict[server][2])
        ExecGMSQL(sql)
    pass

GetMaxOnline()
GetAvgOnline()
GetAvgOnlineTime()
GetIpCount()
login_account_dict = GetLoginAccountCount()
GetOldLoginAccountCount()
GetCreatePlayerCount()
GetNewAccountCount()

GetNewRechargeMoney()
GetRechargeAcountCount()
GetTotalRechargeAccount()
GetNewRechargeAcountCount()
recharge_player_dict = GetRechargePlayerCount()
GetRechargeRate(login_account_dict, recharge_player_dict)
