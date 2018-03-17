#if DEBUG
#define _TEST
#endif

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net;
using System.Security.Cryptography;
using System.Data;
using System.Text.RegularExpressions;
using System.IO;
using System.Timers;
using System.Threading;
using MySql.Data.MySqlClient;


namespace RechargeServer
{
    class Program
    {
        public static MySqlConnection mysql_connect;        //数据库连接
        public static string log_path;                      //日志路径

        static void Main(string[] args)
        {
            #region 启动信息
            int platform_id = 0;
            if (args.Length > 0)
                platform_id = Convert.ToInt32(args[0]);
            Console.WriteLine("*****************************************");
            Console.Write("服务器名:\tRechargeServer");
#if _TEST
            Console.Write("(测试版)");
#endif
            Console.Write("\n运行版本:\t");
#if DEBUG
            Console.WriteLine("Debug");
#else
            Console.WriteLine("Release");
#endif
            Console.Write("启动时间:\t");
            Console.WriteLine(DateTime.Now);
            Console.WriteLine("平台ID:\t\t" + platform_id);
            Console.WriteLine("*****************************************");
            #endregion


            //系统是否支持
            if (!HttpListener.IsSupported)
            {
                Console.WriteLine("Windows XP SP2 or Server 2003 is required to use the HttpListener class.");
                return;
            }

            log_path = @".\recharge_log\";
            if (!Directory.Exists(log_path))
                Directory.CreateDirectory(log_path);
            log_path += DateTime.Now.ToString("yyyyMdHHmm") + ".log";

#if _TEST
            NewConnection(platform_id);
#else
            NewConnection(platform_id);
#endif

            try
            {
                mysql_connect.Open();
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex);
                Console.WriteLine("连接数据库失败");
                return;
            }

            new Thread(QueryLoop).Start();

            //打开监听
            HttpListener listener = new HttpListener();
            listener.Prefixes.Add("http://+:8080/game/pay/");
            listener.Prefixes.Add("http://+:8080/game/userExists/");
            listener.Start();
            Console.WriteLine("Start Listening...");

            try
            {
                while (true)
                {
                    HttpListenerContext context = listener.GetContext();
                    HttpListenerRequest request = context.Request;

                    if (request.HttpMethod == "GET")
                    {
                        int result = 0;

                        if (request.RawUrl.Contains("game/userExists?"))
                        {
                            result = CheckUser(request);
                        }
                        else if (request.RawUrl.Contains("game/pay?"))
                        {
                            result = Recharge(request);
                        }

                        HttpListenerResponse response = context.Response;

                        string responseString = result.ToString();
                        byte[] buffer = System.Text.Encoding.UTF8.GetBytes(responseString);
                        response.ContentLength64 = buffer.Length;
                        System.IO.Stream output = response.OutputStream;
                        output.Write(buffer, 0, buffer.Length);
                        output.Close();
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex);
            }
            finally
            {
                listener.Stop();
                mysql_connect.Close();
            }

            Console.ReadLine();
        }

        static int CheckUser(HttpListenerRequest request)
        {
            return 1;
        }

        static int Recharge(HttpListenerRequest request)
        {
            Regex reg = new Regex("[0-9A-Za-z_]{1,35}");
            string str_serverid = reg.Match(request.QueryString["serverid"]).ToString();
            string str_orderno = reg.Match(request.QueryString["orderno"]).ToString();
            string str_passport = reg.Match(request.QueryString["passport"]).ToString();
            string str_addgold = reg.Match(request.QueryString["addgold"]).ToString();
            string str_rmb = reg.Match(request.QueryString["rmb"]).ToString();
            string str_sign = reg.Match(request.QueryString["sign"]).ToString();
            string str_paytime = reg.Match(request.QueryString["paytime"]).ToString();

            string order_info = string.Format("日期:{0}\r\n订单号:{1}\r\n服务器ID:{2}\r\n用户名:{3}\r\n充值点数:{4}\r\n充值金额:{5}\r\n充值码:{6}\r\n充值时间戳:{7}\r\n"
              , DateTime.Now.ToString("u"), str_orderno, str_serverid, str_passport, str_addgold, str_rmb, str_sign, str_paytime);

            Console.Write(order_info);

            StreamWriter file_writer = new StreamWriter(log_path, true, Encoding.UTF8);

            file_writer.Write(order_info);

            int server_id = 0;
            int add_gold = 0;
            int rmb = 0;
            Int64 paytime = 0;

            try
            {
                server_id = Convert.ToInt32(str_serverid);
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex);
                Console.WriteLine("结果:服务器ID错误\r\n");
                file_writer.WriteLine("结果:服务器ID错误\r\n");
                file_writer.Close();
                return -8;
            }

            try
            {
                add_gold = Convert.ToInt32(str_addgold);
                rmb = Convert.ToInt32(str_rmb);
                paytime = Convert.ToInt64(str_paytime);

                if (add_gold < 0 || rmb < 0)
                {
                    Console.WriteLine("结果:充值金额错误\r\n");
                    file_writer.WriteLine("结果:充值金额错误\r\n");
                    file_writer.Close();
                    return -1;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex);
                Console.WriteLine("结果:充值金额错误\r\n");
                file_writer.WriteLine("结果:充值金额错误\r\n");
                file_writer.Close();
                return -1;
            }

#if !_TEST
            string code = "";
            MD5 md5 = new MD5CryptoServiceProvider();
            byte[] md5_hash = md5.ComputeHash(System.Text.Encoding.UTF8.GetBytes(str_orderno + str_passport + str_serverid + str_addgold + str_rmb + str_paytime + "kQOoX3s7zK"));
            for (int i = 0; i < md5_hash.Length; i++)
                code += md5_hash[i].ToString("x").PadLeft(2, '0');
            md5.Clear();
            if (str_sign != code)
            {
                Console.WriteLine("结果:MD5不匹配\r\n");
                file_writer.WriteLine("结果:MD5不匹配\r\n");
                file_writer.Close();
                return -4;
            }

            if (Math.Abs(GetTimeStamp() - paytime) > 120)
            {
                Console.WriteLine("结果:时间不匹配\r\n");
                file_writer.WriteLine("结果:时间不匹配\r\n");
                file_writer.Close();
                return -9;
            }
#endif
            MySqlCommand cmd_query = new MySqlCommand("select pid from tb_player where sid = " + str_serverid + " and username = '" + str_passport + "'", mysql_connect);

            int pid = 0;

            try
            {
                MySqlDataReader reader = cmd_query.ExecuteReader();

                if (reader.Read())
                {
                    pid = reader.GetInt32("pid");
                }
                else
                {
                    Console.WriteLine("结果:用户不存在\r\n");
                    file_writer.WriteLine("结果:用户不存在\r\n");
                    file_writer.Close();
                    reader.Close();
                    return -2;
                }

                reader.Close();
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex);
                Console.WriteLine("结果:SQL语句查询用户错误\r\n");
                file_writer.WriteLine("结果:SQL语句查询用户错误\r\n");
                file_writer.Close();
                return -1;
            }

            MySqlCommand cmd_insert = new MySqlCommand("insert into tb_order_form values('" + str_orderno + "'," + pid + "," + add_gold + "," + rmb + ",default,false,default)", mysql_connect);

            try
            {
                cmd_insert.ExecuteNonQuery();
            }
            catch (Exception)
            {
                Console.WriteLine("结果:订单已存在\r\n");
                file_writer.WriteLine("结果:订单已存在\r\n");
                file_writer.Close();
                return -5;
            }

            file_writer.Close();
            return 1;
        }

        static void NewConnection(int id)
        {
            string db_ip = "192.168.1.6";
            string db_username = "root";
            string db_password = "1234";
            string db_name = "db_game";

            switch (id)
            {
                case 32:
                    db_ip = "egamehlzj.mysql.rds.aliyuncs.com";
                    db_username = "egame";
                    db_password = "yz067jrp";
                    db_name = "db_32wan";
                    break;
            }

            mysql_connect = new MySqlConnection("server=" + db_ip + ";uid=" + db_username + ";pwd=" + db_password + ";database=" + db_name + ";");
        }

        static void QueryLoop()
        {
            while (true)
            {
                Thread.Sleep(10000);

                try
                {
                    MySqlCommand cmd_query = new MySqlCommand("select now()", mysql_connect);
                    cmd_query.ExecuteReader().Close();
                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex);
                }
            }
        }

        static Int64 GetTimeStamp()
        {
            DateTime timeStamp = new DateTime(1970, 1, 1);
            return (DateTime.UtcNow.Ticks - timeStamp.Ticks) / 10000000;
        }
    }
}
