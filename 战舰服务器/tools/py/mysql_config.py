import pymysql
import warnings

warnings.filterwarnings('ignore')

report_dir = "../report"

mysql_host = '192.168.16.102'
mysql_port = 3306
mysql_user = 'root'
mysql_password = '1q2w3e'
mysql_db = 'zhanjian'
mysql_log_db = 'zhanjian_log'
mysql_auth_db = "auth_db"
mysql_gm_db = "zhanjian_gm"

def NewDBConn():
    conn = pymysql.connect(host=mysql_host, port=mysql_port, user=mysql_user, password=mysql_password, database=mysql_db, charset='utf8', autocommit=True)
    return conn

def NewLogDBConn():
    conn = pymysql.connect(host=mysql_host, port=mysql_port, user=mysql_user, password=mysql_password, database=mysql_log_db, charset='utf8', autocommit=True)
    return conn

def NewAuthDBConn():
    conn = pymysql.connect(host=mysql_host, port=mysql_port, user=mysql_user, password=mysql_password, database=mysql_auth_db, charset='utf8', autocommit=True)
    return conn

def NewGMDBConn():
    conn = pymysql.connect(host=mysql_host, port=mysql_port, user=mysql_user, password=mysql_password, database=mysql_gm_db, charset='utf8', autocommit=True)
    return conn
