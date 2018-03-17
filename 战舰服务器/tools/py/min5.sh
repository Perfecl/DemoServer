#!/bin/sh

export PATH=$PATH:/usr/local/bin
chmod +x ./*.py

python3 ./server_info.py
python3 ./gm_run.py `date -d '-0 day' +%F`
python3 ./stat_research_item.py
python3 ./stat_roll_server.py
python3 ./stat_ltv.py
python3 ./stat_top50_cost_earn.py