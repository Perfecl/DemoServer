#!/usr/bin/python3
# coding:utf-8

from datetime import datetime, timedelta

from mysql_config import *
import xian_yu_api as xy
import time

filter_condition = " and (openid like 'XY%%' or openid like 'XIANYU%%')"

game_db_conn = NewDBConn()
game_cur = game_db_conn.cursor()
log_db_conn = NewLogDBConn()
log_cur = log_db_conn.cursor()
gm_db_conn = NewGMDBConn()
gm_cur = gm_db_conn.cursor()

time_point = datetime.now() - timedelta(minutes=5)
today = time_point.strftime("%Y-%m-%d")
ACCESS_LOG = r"/etc/nginx/logs/access.log"
ACCESS_LOG_1 = (time_point + timedelta(minutes=5)).strftime("/etc/nginx/logs/%Y/%Y%m/access.%Y%m%d.log")
begin_time = datetime(time_point.year, time_point.month, time_point.day, time_point.hour,
                      time_point.minute - (time_point.minute % 5))
end_time = begin_time + timedelta(minutes=5)
begin_sharp_time = int(begin_time.timestamp())
end_sharp_time = int(end_time.timestamp())

role_tp = datetime.now() - timedelta(hours=1)
role_today = role_tp.strftime("%Y-%m-%d")
role_begin_time = datetime(role_tp.year, role_tp.month, role_tp.day, role_tp.hour,
                           role_tp.minute - (role_tp.minute % 5))
role_end_time = role_begin_time + timedelta(hours=1)
role_begin_sharp_time = int(role_begin_time.timestamp())
role_end_sharp_time = int(role_end_time.timestamp())


def str_cov(data):
    if data and len(str(data)) > 0:
        return str(data)
    return 'Unknown'


def query_game_db(sql):
    try:
        game_cur.execute(sql)
        return game_cur
    except pymysql.err.IntegrityError as e:
        print(e)
        return None
    except pymysql.err.ProgrammingError:
        return None


def query_log_db(sql):
    try:
        log_cur.execute(sql)
        return log_cur
    except pymysql.err.IntegrityError as e:
        print(e)
        return None
    except pymysql.err.ProgrammingError:
        return None


def query_gm_db(sql):
    try:
        gm_cur.execute(sql)
        return gm_cur
    except pymysql.err.IntegrityError as e:
        print(e)
        return None
    except pymysql.err.ProgrammingError:
        return None


def request_newplayer():
    param_list = []
    sql = 'select `server`, openid, player_id, `time`, `channel` from'
    sql += ' zhanjian_log.`newplayer_%s`' % today
    sql += ' where `time` >= %d and `time` < %d' % (begin_sharp_time, end_sharp_time)
    sql += filter_condition
    result_cur = query_log_db(sql)
    if not result_cur:
        return
    for row in result_cur:
        login_dict = dict()
        login_dict['chr'] = str_cov(row[4])
        login_dict['chl'] = str_cov(row[4])
        login_dict['svr'] = str_cov(row[0])
        login_dict['ts'] = int(row[3])
        login_dict['aid'] = str_cov(row[1])
        param_list.append(login_dict)
    xy.request_xian_yu_api('loginserver', param_list)
    pass


def request_role():
    param_list = []
    sql = 'select `server`,create_time,uid,openid,channel,login_channel from player_%d '
    sql += 'where create_time >= %d and create_time < %d' % (begin_sharp_time, end_sharp_time)
    sql += filter_condition
    for x in range(100):
        result_cur = query_game_db(sql % x)
        if not result_cur:
            break
        for row in result_cur:
            role_dict = dict()
            role_dict['chr'] = str_cov(row[4])
            role_dict['chl'] = str_cov(row[5])
            role_dict['svr'] = str_cov(row[0])
            role_dict['ts'] = int(row[1])
            role_dict['rid'] = str_cov(row[2])
            role_dict['aid'] = str_cov(row[3])
            param_list.append(role_dict)
    xy.request_xian_yu_api('role', param_list)


def request_login():
    param_server_list = []
    param_role_list = []
    sql = 'select b.channel,b.login_channel,b.`server`,a.`time`,b.openid,b.uid ' \
          'from zhanjian_log.`login_%s` as a ' % today
    sql += 'inner join zhanjian.player_%d as b on a.player_id = b.uid '
    sql += 'where a.`time` >= %d and a.`time` < %d and a.`time` > b.create_time ' % (begin_sharp_time, end_sharp_time)
    sql += filter_condition
    for x in range(100):
        result_cur = query_game_db(sql % x)
        if not result_cur:
            break
        for row in result_cur:
            login_dict = dict()
            login_dict['chr'] = str_cov(row[0])
            login_dict['chl'] = str_cov(row[1])
            login_dict['svr'] = str_cov(row[2])
            login_dict['ts'] = int(row[3])
            login_dict['aid'] = str_cov(row[4])
            param_server_list.append(login_dict)
            role_dict = login_dict.copy()
            role_dict['rid'] = str_cov(row[5])
            param_role_list.append(role_dict)
    xy.request_xian_yu_api('loginserver', param_server_list)
    xy.request_xian_yu_api('loginrole', param_role_list)


def request_online_num():
    param_list = []
    sql = 'select server_id,cur_time,online_num from gm_online_info ' \
          'where cur_time >= %d and cur_time < %d' % (begin_sharp_time, end_sharp_time)
    result_cur = query_gm_db(sql)
    for row in result_cur:
        online_dict = dict()
        online_dict['chr'] = 'ALL'
        online_dict['svr'] = str_cov(row[0])
        online_dict['ts'] = int(row[1])
        online_dict['cnt'] = int(row[2])
        param_list.append(online_dict)
    xy.request_xian_yu_api('online', param_list)


def request_charge():
    param_list = []
    sql = 'select b.channel,b.login_channel,b.`server`,a.recharge_time,a.order_id,b.uid,b.openid,a.pay_amount,' \
          'a.goods_id,b.device_id,c.ipaddr from zhanjian.recharge_details as a inner join zhanjian.player_%d as b ' \
          'on a.role_id = b.uid '
    sql += 'left join (select ipaddr,player_id from zhanjian_log.`login_%s` where login = 1 group by player_id) ' \
           'as c on b.uid = c.player_id ' % today
    sql += 'where a.recharge_time >= %d and a.recharge_time < %d and a.stage = 3 ' % (begin_sharp_time, end_sharp_time)
    sql += filter_condition
    for x in range(100):
        result_cur = query_game_db(sql % x)
        if not result_cur:
            break
        for row in result_cur:
            charge_dict = dict()
            charge_dict['chr'] = str_cov(row[0])
            charge_dict['chl'] = str_cov(row[1])
            charge_dict['svr'] = str_cov(row[2])
            charge_dict['ts'] = int(row[3])
            charge_dict['oid'] = str_cov(row[4])
            charge_dict['rid'] = str_cov(row[5])
            charge_dict['aid'] = str_cov(row[6])
            charge_dict['mny'] = int(row[7])
            charge_dict['gc'] = str_cov(row[8])
            charge_dict['gn'] = xy.get_goods_name_by_id(str_cov(row[8]))
            charge_dict['ga'] = 1
            charge_dict['did'] = str_cov(row[9])
            charge_dict['ip'] = str_cov(row[10])
            param_list.append(charge_dict)
    xy.request_xian_yu_api('charge', param_list)


def request_consume():
    param_list = []
    sql = 'select b.channel,b.login_channel,b.`server`,a.`time`,b.uid,b.openid,-a.delta_2,a.money_2,c.item_id,' \
          'a.msgid,c.item_count,b.device_id, c.item_uid from zhanjian_log.`currency_%s` as a ' % today
    sql += 'inner join zhanjian.player_%d as b on a.player_id = b.uid '
    sql += 'left join zhanjian_log.`item_%s` as c on a.tid = c.tid and a.player_id = c.player_id ' \
           'where a.delta_2 < 0 and a.`time` >= %d and a.`time` < %d' % (today, begin_sharp_time, end_sharp_time)
    sql += filter_condition
    for x in range(100):
        result_cur = query_game_db(sql % x)
        if not result_cur:
            break
        for row in result_cur:
            consume_dict = dict()
            consume_dict['chr'] = str_cov(row[0])
            consume_dict['chl'] = str_cov(row[1])
            consume_dict['svr'] = str_cov(row[2])
            consume_dict['ts'] = int(row[3])
            consume_dict['rid'] = str_cov(row[4])
            consume_dict['aid'] = str_cov(row[5])
            consume_dict['cnt'] = int(row[6])
            consume_dict['cnta'] = int(row[7])
            consume_dict['tc'] = str_cov(row[8]) + '_' + str_cov(row[12])
            consume_dict['tn'] = str_cov(xy.get_item_name_by_id(int(row[8])) if row[8] else xy.get_desc_by_msg(int(row[9])))
            consume_dict['ta'] = int(row[10] if row[10] else 0)
            consume_dict['did'] = str_cov(row[11])
            param_list.append(consume_dict)
    xy.request_xian_yu_api('consume', param_list)


def request_output():
    param_list = []
    sql = 'select b.channel,b.login_channel,b.`server`,a.`time`,b.uid,b.openid,a.delta_2,a.money_2,a.msgid from ' \
          'zhanjian_log.`currency_%s` as a inner join ' % today
    sql += 'zhanjian.player_%d as b on a.player_id = b.uid where delta_2 > 0 and '
    sql += 'a.`time` >= %d and a.`time` < %d' % (begin_sharp_time, end_sharp_time)
    sql += filter_condition
    for x in range(100):
        result_cur = query_game_db(sql % x)
        if not result_cur:
            break
        for row in result_cur:
            output_dict = dict()
            output_dict['chr'] = str_cov(row[0])
            output_dict['chl'] = str_cov(row[1])
            output_dict['svr'] = str_cov(row[2])
            output_dict['ts'] = int(row[3])
            output_dict['rid'] = str_cov(row[4])
            output_dict['aid'] = str_cov(row[5])
            output_dict['cnt'] = int(row[6])
            output_dict['cnta'] = int(row[7])
            output_dict['src'] = xy.get_desc_by_msg(int(row[8]))
            param_list.append(output_dict)
    xy.request_xian_yu_api('output', param_list)


def request_role_info():
    if role_begin_time.minute != 0:
        return
    param_list = []
    sql = 'select a.channel,a.`server`,a.create_time,a.uid,a.`name`,a.openid,a.last_login_time,' \
          'a.`level`,b.ipaddr,a.total_recharge,a.money from zhanjian.player_%d '
    sql += 'as a inner join zhanjian_log.`login_%s` as b on a.uid = b.player_id where uid in ' \
           '(select distinct(player_id) from zhanjian_log.`currency_%s` where delta_1!=0 and `time` >= %d ' \
           'and `time` < %d ) and b.login = 1 ' % (role_today, role_today, role_begin_sharp_time, role_end_sharp_time)
    sql += filter_condition
    sql += ' group by a.uid'
    for x in range(100):
        result_cur = query_game_db(sql % x)
        if not result_cur:
            break
        for row in result_cur:
            role_dict = dict()
            role_dict['chr'] = str_cov(row[0])
            role_dict['svr'] = str_cov(row[1])
            role_dict['tsr'] = int(row[2])
            role_dict['rid'] = str_cov(row[3])
            role_dict['rn'] = str_cov(row[4])
            role_dict['aid'] = str_cov(row[5])
            role_dict['tsl'] = int(row[6])
            role_dict['lv'] = int(row[7])
            role_dict['ip'] = str_cov(row[8])
            role_dict['tpc'] = int(row[9])
            role_dict['rc'] = int(row[10])
            role_dict['ts'] = int(time.time())
            param_list.append(role_dict)
    xy.request_xian_yu_api('roleinfo', param_list)


def request_item():
    param_list = []
    sql1 = 'select b.channel,b.`server`,a.`time`,b.uid,b.openid,a.msgid,a.item_id,0,a.item_old_count,' \
           'a.item_count-a.item_old_count,a.item_count,a.item_uid from zhanjian_log.`item_%s` as a inner join ' % today
    sql1 += 'zhanjian.player_%d as b on a.player_id = b.uid '
    sql1 += 'where a.`time` >= %d and a.`time` < %d ' % (begin_sharp_time, end_sharp_time)
    sql1 += filter_condition

    sql2 = 'select b.channel,b.`server`,a.`time`,b.uid,b.openid,a.msgid,a.item_id,0,a.item_count,a.item_count,0,item_uid from ' \
           'zhanjian_log.`item_delete_%s` as a inner join ' % today
    sql2 += 'zhanjian.player_%d as b on a.player_id = b.uid '
    sql2 += 'where a.`time` >= %d and a.`time` < %d ' % (begin_sharp_time, end_sharp_time)
    sql2 += filter_condition

    sql_list = [sql1, sql2]

    for x in range(100):
        for sql in sql_list:
            result_cur = query_game_db(sql % x)
            if not result_cur:
                break
            for row in result_cur:
                item_dict = dict()
                item_dict['chr'] = str_cov(row[0])
                item_dict['svr'] = str_cov(row[1])
                item_dict['ts'] = int(row[2])
                item_dict['rid'] = str_cov(row[3])
                item_dict['aid'] = str_cov(row[4])
                item_dict['op'] = xy.get_desc_by_msg(int(row[5]))
                item_dict['nm'] = xy.get_item_name_by_id(int(row[6])) + ":" + str(row[11])
                item_dict['ipos'] = str_cov(row[7])
                item_dict['quab'] = int(row[8])
                item_dict['qua'] = abs(int(row[9]))
                item_dict['quaa'] = int(row[10])
                param_list.append(item_dict)
    xy.request_xian_yu_api('item', param_list)


def request_money():
    param_list = []
    money_list = [(1, '美元'), (7, '钢铁'), (8, '原料'), (10, '威名'), (9, '声望'), (11, '功勋'), (12, '联盟贡献')]
    sql = 'select b.channel,b.`server`,a.`time`,b.uid,b.openid,a.msgid,a.money_%d-a.delta_%d,a.delta_%d,a.money_%d '
    sql += 'from zhanjian_log.`currency_%s` as a inner join ' % today
    sql += 'zhanjian.player_%d as b on a.player_id = b.uid '
    sql += 'where a.`time` >= %d and a.`time` < %d ' % (begin_sharp_time, end_sharp_time)
    sql += ' and delta_%d != 0 '
    sql += filter_condition
    for money in money_list:
        for x in range(100):
            result_cur = query_game_db(sql % (money[0], money[0], money[0], money[0], x, money[0]))
            if not result_cur:
                break
            for row in result_cur:
                money_dict = dict()
                money_dict['chr'] = str_cov(row[0])
                money_dict['svr'] = str_cov(row[1])
                money_dict['ts'] = int(row[2])
                money_dict['rid'] = str_cov(row[3])
                money_dict['aid'] = str_cov(row[4])
                money_dict['ot'] = 1 if int(row[7]) > 0 else 2
                money_dict['op'] = xy.get_desc_by_msg(int(row[5]))
                money_dict['nm'] = money[1]
                money_dict['quab'] = int(row[6])
                money_dict['qua'] = abs(int(row[7]))
                money_dict['quaa'] = int(row[8])
                param_list.append(money_dict)
    xy.request_xian_yu_api('money', param_list)


def request_level_change():
    param_level_list = []
    param_vip_list = []
    sql = 'select b.channel,b.login_channel,b.`server`,a.`time`,b.uid,b.openid,a.`level`,a.vip_level, a.delta_level,' \
          'a.delta_vip from zhanjian_log.`currency_%s` as a inner join ' % today
    sql += 'zhanjian.player_%d as b on a.player_id=b.uid where (delta_level!=0 or delta_vip!=0) and '
    sql += 'a.`time`>=%d and a.`time`<%d ' % (begin_sharp_time, end_sharp_time)
    sql += filter_condition
    for x in range(100):
        result_cur = query_game_db(sql % x)
        if not result_cur:
            break
        for row in result_cur:
            common_dict = dict()
            common_dict['chr'] = str_cov(row[0])
            common_dict['chl'] = str_cov(row[1])
            common_dict['svr'] = str_cov(row[2])
            common_dict['ts'] = int(row[3])
            common_dict['rid'] = str_cov(row[4])
            common_dict['aid'] = str_cov(row[5])
            if int(row[8]):
                common_dict['lv'] = int(row[6])
                param_level_list.append(common_dict)
            if int(row[9]):
                common_dict['grd'] = int(row[7])
                param_vip_list.append(common_dict)
    xy.request_xian_yu_api('rolelevel', param_level_list)
    xy.request_xian_yu_api('vipgrade', param_vip_list)

# =========================================================================================================

#-|04/Aug/2017:15:51:04 +0800|"GET /xianyujingfen?api=tutorial&chr=${channel}&svr=${server_id}&ts=${seconds}&rid=${player_id}&aid=${open_id}&step=${step} HTTP/1.1"|404|169|-|-|"-"|"Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:54.0) Gecko/20100101 Firefox/54.0"|"JSESSIONID=1E4F98168A5A0707B8E0E0E8E0E1C5FD"|58.247.169.38|cn-tx-sh-zjry-all-gms-211-159-220-183-xhhd|0.000
#-|04/Aug/2017:15:46:10 +0800|"GET /xianyujingfen?api=loginsdk&chr=${channel}&svr=${server_id}&ts=${seconds}&aid=${openid} HTTP/1.1"|404|169|-|-|"-"|"Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:54.0) Gecko/20100101 Firefox/54.0"|"JSESSIONID=1E4F98168A5A0707B8E0E0E8E0E1C5FD"|58.247.169.38|cn-tx-sh-zjry-all-gms-211-159-220-183-xhhd|0.000
def get_param_from_query_string(array, param):
    p = param + "="
    for str in array:
        index = str.find(p)
        if index < 0:
            continue
        return str[(index + len(p)):]
    return ""


def fill_login_sdk(array, list):
    chr = get_param_from_query_string(array, 'chr')
    chl = chr
    ts = int(get_param_from_query_string(array, 'ts'))
    aid = get_param_from_query_string(array, 'aid')
    if aid.find("XY_") != 0 and aid.find("XIANYU_") != 0:
        return
    d = dict()
    d['chr'] = chr
    d['chl'] = chl
    d['ts'] = ts
    d['aid'] = aid
    list.append(d)
    pass


def fill_tutorial(array, list):
    chr = get_param_from_query_string(array, 'chr')
    chl = chr
    svr = get_param_from_query_string(array, 'svr')
    ts = int(get_param_from_query_string(array, 'ts'))
    aid = get_param_from_query_string(array, 'aid')
    rid = get_param_from_query_string(array, 'rid')
    step = int(get_param_from_query_string(array, 'step'))
    stat = 1
    if aid.find("XY_") != 0 and aid.find("XIANYU_") != 0:
        return
    d = dict()
    d['chr'] = chr
    d['chl'] = chl
    d['ts'] = ts
    d['aid'] = aid
    d['stat'] = stat
    d['svr'] = svr
    d['rid'] = rid
    d['step'] = step
    list.append(d)
    pass


def request_loginsdk_and_tutorial():
    list_login_sdk = list()
    list_tutorial = list()

    file_name = ACCESS_LOG if datetime.now().day == time_point.day else ACCESS_LOG_1
    file = open(file_name, "r", encoding='utf-8')
    time_begin = begin_time.hour * 10000 + begin_time.minute * 100 + begin_time.second
    time_end = end_time.hour * 10000 + end_time.minute * 100 + end_time.second
    begin_token = "xianyujingfen?"
    end_token = " HTTP"
    for line in file:
        begin_index = line.find(begin_token)
        if begin_index < 0:
            continue
        try:
            t1 = int(line[14:16]) * 10000 + int(line[17:19]) * 100 + int(line[20:22])
            if t1 < time_begin or t1 >= time_end:
                continue
            end_index = line.find(end_token)
            if end_index < 0:
                continue
            content = line[begin_index + len(begin_token): end_index]
            array = content.split("&")
            try:
                api = get_param_from_query_string(array, "api")
                if api == "loginsdk":
                    fill_login_sdk(array, list_login_sdk)
                    pass
                if api == "tutorial":
                    fill_tutorial(array, list_tutorial)
                    pass
            except:
                pass
        except:
            pass
    if len(list_login_sdk) > 0:
        xy.request_xian_yu_api('loginsdk', list_login_sdk)
    if len(list_tutorial) > 0:
        xy.request_xian_yu_api("tutorial", list_tutorial)
    pass

time.sleep(30)

try: request_newplayer()
except: pass
try: request_role()
except: pass
try: request_login()
except: pass
try: request_online_num()
except: pass
try: request_role_info()
except: pass
try: request_item()
except: pass
try: request_money()
except: pass
try: request_level_change()
except: pass
try: request_loginsdk_and_tutorial()
except: pass
try: request_charge()
except: pass
try: request_consume()
except: pass
try: request_output()
except: pass

gm_cur.close()
gm_db_conn.close()
log_cur.close()
log_db_conn.close()
game_cur.close()
game_db_conn.close()

xy.loop_flag = True
