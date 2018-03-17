#!/bin/sh

export PATH=$PATH:/usr/local/bin:/usr/bin
chmod +x ./*.py

python3 ./stat_army.py
python3 ./stat_mission.py
python3 ./stat_distribution.py
python3 ./stat_currency_in_stock.py
python3 ./stat_tactic_info.py
python3 ./stat_recharge_award.py
