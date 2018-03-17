#!/bin/sh

export PATH=$PATH:/usr/local/bin
chmod +x ./*.py

python3 ./daily_log.py
python3 ./delete_daily_log.py
python3 ./gm_run.py
python3 ./stat_equip.py
python3 ./stat_item.py
python3 ./stat_ship.py
python3 ./stat_shop.py
python3 ./stat_new_guy.py
python3 ./stat_new_ship.py
python3 ./stat_item_in_stock.py
python3 ./stat_copy.py
python3 ./stat_currency.py
python3 ./stat_deadman_copy.py
python3 ./stat_player_recharge.py
python3 ./stat_day_pay_info.py
python3 ./stat_day_recharge.py
