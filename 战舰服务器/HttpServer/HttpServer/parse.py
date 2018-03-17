#!/usr/bin/python3

import os
import glob
import re

print("parse proto begin")
print(os.getcwd())

folder = 'Message'
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


gen_cpp = open(os.getcwd() + "/SDK/Net/RegisterMessage.cs", 'w')
gen_cpp.write('namespace SharpServer.SDK.Net\n{\n')

gen_cpp.write('using ProtoBuf;\n')
for namespace in set(namespaces):
    n = namespace[0 : len(namespace)]
    gen_cpp.write("using %s;\n" % n.replace('.', '::'))
gen_cpp.write('\n')

gen_cpp.write('\n')
gen_cpp.write("public partial class ProtoBuffDecode\n{\n")
gen_cpp.write('static void Load()\n')
gen_cpp.write('{\n')
for (msg_id, message) in set(messages):
    enumname = msg_id
    if msg_id.find("MSG_CS") >= 0:
        enumname = "MSG_CS." + msg_id
    else:
        enumname = "MSG_SS." + msg_id
    gen_cpp.write("    parsers[(int)%s] = (stream => Serializer.Deserialize<%s>(stream));\n" % (enumname, message))
gen_cpp.write('}\n')
gen_cpp.write("}\n")


gen_cpp.write('}')
gen_cpp.close()

print("parse proto end")
