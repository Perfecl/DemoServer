#!/usr/bin/python3
# coding=utf-8

from mysql_config import *
from datetime import timedelta, datetime


class PlayerInfo:
    uid = 0
    server_id = ''
    create_time = 0
    login_time = 0
    online_time = 0
    level = 0
    vip_level = 0


def get_login_player_info(log_cur, day):
    players = {}
    sql = "select player_id, server, time, sum(online_time) from `login_%s` group by player_id" % day
    try:
        log_cur.execute(sql)
        for row in log_cur:
            if row[0] not in players:
                players[row[0]] = PlayerInfo()
            players[row[0]].uid = int(row[0])
            players[row[0]].server_id = str(row[1])
            players[row[0]].login_time = int(row[2])
            players[row[0]].online_time = int(row[3])
    except pymysql.err.IntegrityError as e:
        print(e)
    return players


def get_new_player_info(log_cur, day):
    players = {}
    sql = "select player_id, server from `newplayer_%s`" % day
    try:
        log_cur.execute(sql)
        for row in log_cur:
            if row[0] not in players:
                players[row[0]] = PlayerInfo()
            players[row[0]].uid = int(row[0])
            players[row[0]].server_id = str(row[1])
    except pymysql.err.IntegrityError as e:
        print(e)
    return players


def get_player_simple_info(game_cur, players):
    if not players:
        return
    uid_str = "(0"
    for key in players:
        uid_str += ',' + str(key)
    uid_str += ')'
    sql = "select uid, create_time, level, vip_level from player_%d " + ("where uid in %s" % uid_str)
    x = 0
    while True:
        try:
            game_cur.execute(sql % x)
            for row in game_cur:
                if row[0] not in players:
                    continue
                players[row[0]].create_time = row[1]
                players[row[0]].level = row[2]
                players[row[0]].vip_level = row[3]
        except pymysql.err.IntegrityError as e:
                print(e)
        except pymysql.err.ProgrammingError:
            break
        x += 1

def split_player_info_multi_server(players):
    info = {}
    if not players:
        return info
    for key in players:
        if players[key].server_id not in info:
            info[players[key].server_id] = []
        info[players[key].server_id].append(players[key])
    return info


def insert_into_gm_take_rates(server_id, gm_cur, day, new_players, login_players):
    if not new_players:
        return
    new_account = len(new_players)
    new_login_account = 0
    valid_account = 0
    valid_10min = 0
    valid_30min = 0
    valid_60min = 0
    for row in new_players:
        if row.create_time is 0:
            continue
        new_login_account += 1
        if row.level > 10:
            valid_account += 1
        if row.uid in login_players:
            if login_players[row.uid ].online_time > 10 * 60:
                valid_10min += 1
            if login_players[row.uid ].online_time > 30 * 60:
                valid_30min += 1
            if login_players[row.uid ].online_time > 60 * 60:
                valid_60min += 1
    connectivity = new_login_account / new_account
    valid_rate = valid_account / new_account
    valid_10min_rate = valid_10min / new_account
    valid_30min_rate = valid_30min / new_account
    valid_60min_rate = valid_60min / new_account
    sql = """insert into gm_day_take_rates (platform_id,server_id,date_time,new_account,new_login_account,
          connectivity_rate,valid_account,valid_rate,minute_valid_10,minute_valid_30,minute_valid_60,
          minute_valid_rate_10,minute_valid_rate_30,minute_valid_rate_60,account_source)
          values('0','%s','%s',%d,%d,%.2f,%d,%.2f,%d,%d,%d,%.2f,%.2f,%.2f,3)
          on DUPLICATE KEY UPDATE new_account=values(new_account), new_login_account=values(new_login_account),
          connectivity_rate=values(connectivity_rate),valid_account=values(valid_account),valid_rate=values(valid_rate),
          minute_valid_10=values(minute_valid_10),minute_valid_30=values(minute_valid_30),
          minute_valid_60=values(minute_valid_60),minute_valid_rate_10=values(minute_valid_rate_10),
          minute_valid_rate_30=values(minute_valid_rate_30), minute_valid_rate_60=values(minute_valid_rate_60),
          account_source=values(account_source)""" \
          % (server_id, day, new_account, new_login_account, connectivity, valid_account,
             valid_rate, valid_10min, valid_30min, valid_60min, valid_10min_rate, valid_30min_rate, valid_60min_rate)
    try:
        gm_cur.execute(sql)
    except pymysql.err.IntegrityError as e:
        print(e)


def insert_into_gm_retention(server_id, gm_cur, day, new_players):
    if not new_players:
        return
    sql = """insert into gm_day_retention_rates (platform_id,server_id,date_time,new_account,account_source)
          values(0,'%s','%s',%d,3) on DUPLICATE KEY UPDATE new_account=values(new_account),
          account_source=values(account_source)""" % (server_id, day, len(new_players))
    try:
        gm_cur.execute(sql)
    except pymysql.err.IntegrityError as e:
        print(e)


def insert_into_gm_retention_30(server_id, gm_cur, day, new_players):
    if not new_players:
        return
    sql = "insert into gm_day_retention_30 (platform_id,server_id,date_time,new_account) values(0,'%s','%s',%d) "\
          "on DUPLICATE KEY UPDATE new_account=values(new_account)" % (server_id, day, len(new_players))
    try:
        gm_cur.execute(sql)
    except pymysql.err.IntegrityError as e:
        print(e)


def update_gm_retention(server_id, gm_cur, log_cur, day, login_player):
    if not login_player:
        return
    today_time = datetime.strptime(day, "%Y-%m-%d")
    uid_str = "(0"
    for row in login_player:
        uid_str += ',' + str(row.uid)
    uid_str += ')'
    for x in range(6):
        day_before = (today_time - timedelta(days=(x+1))).strftime("%Y-%m-%d")
        sql = "select count(*) from `newplayer_%s` where player_id in %s" % (day_before, uid_str)
        try:
            log_cur.execute(sql)
        except pymysql.err.IntegrityError as e:
            print(e)
            continue
        except pymysql.err.ProgrammingError:
            continue
        login_num = log_cur.fetchone()[0]
        sql = "update gm_day_retention_rates set day_stay_%d = %d, day_stay_rate_%d = day_stay_%d / new_account "\
              "where date_time = '%s' and server_id = '%s'" % (x + 2, login_num, x + 2, x + 2, day_before, server_id)
        try:
            gm_cur.execute(sql)
        except pymysql.err.IntegrityError as e:
            print(e)
    update_gm_retention_other(server_id, gm_cur, log_cur, day)


def update_gm_retention_30(server_id, gm_cur, log_cur, day, login_player):
    if not login_player:
        return
    today_time = datetime.strptime(day, "%Y-%m-%d")
    uid_str = "(0"
    for row in login_player:
        uid_str += ',' + str(row.uid)
    uid_str += ')'
    for x in range(29):
        day_before = (today_time - timedelta(days=(x+1))).strftime("%Y-%m-%d")
        sql = "select count(*) from `newplayer_%s` where player_id in %s" % (day_before, uid_str)
        try:
            log_cur.execute(sql)
        except pymysql.err.IntegrityError as e:
            print(e)
            continue
        except pymysql.err.ProgrammingError:
            continue
        login_num = log_cur.fetchone()[0]
        sql = "update gm_day_retention_30 set day_stay_%d = %d where date_time = '%s' and server_id = '%s'"\
              % (x + 2, login_num, day_before, server_id)
        try:
            gm_cur.execute(sql)
        except pymysql.err.IntegrityError as e:
            print(e)


def update_gm_retention_other(server_id, gm_cur, log_cur, day):
    day_login = {}
    day2_num = 0
    day3_num = 0
    day4_num = 0
    today_time = datetime.strptime(day, "%Y-%m-%d")
    for x in range(7):
        day_before = (today_time - timedelta(days=x)).strftime("%Y-%m-%d")
        sql = "select player_id from `login_%s` where `server` = '%s' group by player_id" % (day_before, server_id)
        try:
            log_cur.execute(sql)
        except pymysql.err.IntegrityError as e:
            print(e)
            continue
        except pymysql.err.ProgrammingError:
            continue
        for row in log_cur:
            if row[0] not in day_login:
                day_login[row[0]] = 0
            else:
                day_login[row[0]] += 1
    for key in day_login:
        if day_login[key] >= 2:
            day2_num += 1
        if day_login[key] >= 3:
            day3_num += 1
        if day_login[key] >= 4:
            day4_num += 1
    day_before = (today_time - timedelta(days=6)).strftime("%Y-%m-%d")
    day_3_2 = 0
    day_4_3 = 0
    if day2_num is not 0:
        day_3_2 = day3_num / day2_num
    if day3_num is not 0:
        day_4_3 = day4_num / day3_num
    sql = "update gm_day_retention_rates set day_3_2 = %.2f, day_4_3 = %.2f where date_time = '%s' "\
          "and server_id = '%s'"\
          % (day_3_2, day_4_3, day_before, server_id)
    try:
        gm_cur.execute(sql)
    except pymysql.err.IntegrityError as e:
        print(e)

