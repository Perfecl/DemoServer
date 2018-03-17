// Copyright (c) egmkang wang. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace SharpServer.SDK
{
    using System;
    using System.Linq;
    using System.Net;
    using System.Net.Sockets;

    public static class Platform
    {
        private static readonly DateTime UTC = new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc);

        public static long GetSeconds()
        {
            return (long)DateTime.Now.Subtract(UTC).TotalSeconds;
        }
        public static long GetMilliSeconds()
        {
            return (long)DateTime.Now.Subtract(UTC).TotalMilliseconds;
        }

        public static long GetFileModifyTime(string fileName)
        {
            return (long)System.IO.File.GetLastWriteTime(fileName).Subtract(UTC).TotalSeconds;
        }

        public static string GetLocalIP()
        {
            return Dns.GetHostEntry(Dns.GetHostName())
                    .AddressList
                    .FirstOrDefault(ip => ip.AddressFamily == AddressFamily.InterNetwork)
                    .ToString();
        }
    }
}
