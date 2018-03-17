#!/usr/local/bin/python3
import re

output = "D:\\code\\mbgame03\\code\\tools\\GM\src\\main\\resources\\item\\item_name.txt"
input_dir = "D:\\code\\mbgame03\\doc\SE\\04.配置文件\\client"

money_regex = re.compile('MONEYTYPE_(\d+).*desc":"(.*)"')
item_regex = re.compile('"id":([^,"]{4,}).*"name":"([^"]*)"')

item_list = [("item.json", "道具", 0), ("army.json", "海军", 0), ("equip.json", '装备', 0), ("carrier_plane.json", "飞机", 0),
             ("hero.json", "战舰", 1), ("carrier_info.json", "航母", 1),
             ]

output_file = open(output, "w", encoding="utf-8")

money_file = open(input_dir + '\\language.json', 'r', encoding='utf-8')
for line in money_file:
    for (money_type, desc) in money_regex.findall(line):
        if money_type == "3" or money_type == "4":
            output_file.write("\n%s,%s,%s,1" % ("资源", money_type, desc))
        else:
            output_file.write("\n%s,%s,%s,0" % ("资源", money_type, desc))


for (file_name, type, value) in item_list:
    item_file = open(input_dir + "\\" + file_name, 'r', encoding='utf-8')
    for line in item_file:
        for (item_id, desc) in item_regex.findall(line):
            output_file.write("\n%s,%s,%s,%s" % (type, item_id, desc, value))
