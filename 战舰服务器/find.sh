#!/bin/sh

args=$1

find . -iname "*.h" -o -iname "*.cpp" | xargs grep --color -E -H -n "$args"
