#!/usr/bin/python3
# coding:utf-8

import time
import gzip
import base64
import hashlib
import urllib.request as url_request
import json
import re
import threading
from datetime import datetime

g_secret = 'zolw5y693se5m9j'
g_key = 'zcrteat4e9a8bf5'
url_address = "http://api.data.xianyugame.com/"
send_buf = []
mutex = threading.Lock()
resend = 2
loop_flag = False

now = datetime.now().strftime("%Y-%m-%d %H:%M:%S  ")

today = datetime.now().strftime("%Y-%m-%d")
log_path = '/home/wangtest01/py_log/%s.log' % today
output_file = open(log_path, "a", encoding="utf-8")


def bg_compress(data):
    step1 = str.encode(data, encoding='utf-8')
    step2 = gzip.compress(step1)
    step3 = bytes.decode(base64.encodebytes(step2)).replace('\n', '')
    return str.encode(step3)


def bg_decompress(data):
    step1 = base64.decodebytes(data)
    step2 = gzip.decompress(step1)
    return bytes.decode(step2)


def loop():
    while True:
        time.sleep(1)
        if loop_flag and not send_buf:
            break
        mutex.acquire()
        other_list = []
        for x in send_buf:
            try:
                response_str = url_request.urlopen(x[0], x[1]).read()
                output_file.write("%s, %s, %s, \n" % (datetime.now(), x[0], response_str))
                if response_str.find(b"1") >= 0:
                    output_file.write("%s\n" % bg_decompress(x[1]))
            except (url_request.URLError, url_request.HTTPError):
                if x[2] < resend:
                    x[2] += 1
                    other_list.append(x)
            except:
                output_file.write("exception occured\n")
                pass
        send_buf.clear()
        send_buf.extend(other_list)
        mutex.release()
    output_file.close()


def http_request(url, get, post):
    if post:
        url += '?'
        for key in get:
            url += key + '=' + str(get[key]) + '&'
        url = url[:-1]
    mutex.acquire()
    send_buf.append([url, post, 0])
    mutex.release()


def make_xian_yu_sign(param, content):
    request_head = 'api' + param['api'] + 'key' + param['key'] + 'vtm' + param['vtm']
    request_body = bytes.decode(content)
    sign_str = g_secret + request_head + request_body + g_secret
    md5hash = hashlib.md5(sign_str.encode('utf-8'))
    return md5hash.hexdigest().upper()


def do_request(api, data):
    bg_data = bg_compress(data)
    param = dict()
    param['api'] = api
    param['key'] = g_key
    param['vtm'] = str(int(time.time()))
    param['sign'] = make_xian_yu_sign(param, bg_data)
    return http_request(url_address, param, bg_data)


def serialize_instance(obj):
    d = dict()
    d.update(vars(obj))
    return d


def request_xian_yu_api(api, param_list):
    start_index = 0
    step = 100
    while True:
        if start_index >= len(param_list):
            break
        j_data = json.dumps(param_list[start_index:start_index + step], default=serialize_instance)
        output_file.write(now + api + ':' + j_data + '\n')
        do_request(api, j_data)
        start_index += step

MSG_REGEX = re.compile('MSG_CS_.*=.*0x(.*);.*//([^(\n]*)')

MSG_DICT = dict()


def get_desc_by_msg(msg_id):
    msg = "%X" % int(msg_id)
    if len(MSG_DICT) <= 0:
        file = open("./message/message.proto", "r", encoding='utf8')
        for line in file:
            for (MSG_ID, DESC) in MSG_REGEX.findall(line):
                if MSG_ID not in MSG_DICT:
                    MSG_DICT[MSG_ID] = DESC
    if msg in MSG_DICT:
        return MSG_DICT[msg]
    return "MSGID:0x" + msg


GOODS_DICT = dict()


def get_goods_name_by_id(good_id):
    if len(GOODS_DICT) <= 0:
        DESC = "\"desc\":"
        file = open("./config/goodid_map.json", 'r', encoding='utf8')
        for line in file:
            if line.find("desc") < 0:
                continue
            array = line.split(',')
            desc = None
            for x in array:
                if x.find(DESC) >= 0:
                    desc = x[x.find(":") + 1:]
                    desc = desc.replace("\"", "")
                if x.find("goodid") >= 0:
                    goodid = x[x.find(":") + 1:]
                    goodid = goodid.replace("\"", "").replace("}", "").replace(",", "").replace("\n", "")
                    if len(goodid) < 1:
                        continue
                    if goodid not in GOODS_DICT:
                        GOODS_DICT[goodid] = desc

    if good_id in GOODS_DICT:
        return GOODS_DICT[good_id]
    return "Unknown"

ITEM_DICT = dict()


def get_item_name_by_id(item_id):
    item_id = str(item_id)
    if len(ITEM_DICT) <= 0:
        file = open("./item_name/item_name.txt", 'r', encoding="utf8")
        for line in file:
            array = line.split(',')
            if len(array) < 3:
                continue
            if array[1] not in ITEM_DICT:
                ITEM_DICT[array[1]] = array[2].replace('\n', '')
    if item_id in ITEM_DICT:
        return ITEM_DICT[item_id]
    return "ItemID:" + item_id

t1 = threading.Thread(target=loop, name='Thread-1')
t1.start()

