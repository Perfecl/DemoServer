using SharpServer.SDK;
using SharpServer.SDK.Net;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HttpServer
{
    class Program
    {
        static void Main(string[] args)
        {
            LoggerProvider.InitLogger("http", NLog.LogLevel.Info);
            ProtoBuffDecode.NewMessage(0, null);
            Console.ReadKey();
        }
    }
}
