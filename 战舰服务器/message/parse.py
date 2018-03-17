#!/usr/bin/python3

import os
import glob
import re

print("parse proto begin")
print(os.getcwd())
os.system('mkdir -p cpp')

folder = 'message'
files = []

includes = []
namespaces = []
messages = []

regex_filename = re.compile(r"^(\w*)\.proto$")
regex_namespaces = re.compile(r"package\s*([\w\.]*);")
regex_messages = re.compile(r"//(MSG_(?!CS_RESPONSE|CS_NOTIFY).*)[\b\r\n]*message\s*(Message\w*)")

def parse_include(file_name):
    include = file_name[file_name.find('/') + 1: file_name.find('.')] + '.pb.h'
    includes.append(include)

def parse_namespace(file_str):
    new_str = file_str[0:len(file_str)]
    find = regex_namespaces.findall(new_str)
    for namespace in find:
        namespaces.append(namespace)

def parse_message(file_str):
    new_str = file_str[0:len(file_str)]
    find = regex_messages.findall(new_str)
    for (msg_id, message) in find:
        print("%s => %s" % (msg_id, message))
        messages.append((msg_id, message))

for fs in os.walk(folder):
    for f in fs[2]:
        matchs = regex_filename.findall(f)
        for m in matchs:
            files.append(m + ".proto")

for f in files:
    filename = folder + "/" + f
    file_str = open(filename, encoding='utf8').read()
    parse_include(filename)
    parse_namespace(file_str)
    parse_message(file_str)


gen_cpp = open(os.getcwd() + "/cpp/message_factory_init.cpp", 'w')
gen_cpp.write('#include "../message_factory.h"\n')
gen_cpp.write('\n')

for include in set(includes):
    gen_cpp.write('#include "%s"\n' % include)

gen_cpp.write('\n')
for namespace in set(namespaces):
    n = namespace[0 : len(namespace)]
    gen_cpp.write("using namespace %s;\n" % n.replace('.', '::'))
gen_cpp.write('\n')

gen_cpp.write('\n')
gen_cpp.write('void MessageFactory::Init()\n')
gen_cpp.write('{\n')

test = open("./test.txt", "w", encoding='utf8')
for (msg_id, message) in set(messages):
    gen_cpp.write("    RegisterMessage(%s, new %s());\n" % (msg_id, message))
    if msg_id.find("_SS_") == -1:
        test.write("Test(MSG_CS.%s, new %s());\n" % (msg_id, message))
test.close()
gen_cpp.write('}\n')

gen_cpp.close()

print("parse proto end")
