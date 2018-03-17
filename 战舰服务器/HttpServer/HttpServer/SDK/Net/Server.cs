// Copyright (c) egmkang wang. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace SharpServer.SDK.Net
{
    using System;
    using System.Collections.Generic;
    using DotNetty.Transport.Bootstrapping;
    using DotNetty.Transport.Channels;
    using DotNetty.Transport.Channels.Sockets;
    using DotNetty.Handlers.Timeout;
    using DotNetty.Common.Utilities;
    using System.Threading.Tasks;
    using System.Net;
    using DotNetty.Buffers;
    using DotNetty.Codecs;

    public class Server
    {
        private static readonly string AttrPlayerID = "PLAYER_ID";
        private static readonly string AttrOpenID = "OPEN_ID";
        private static readonly string AttrIpAddr = "IP_ADDR";
        private static readonly string AttrLastActiveTime = "ACTIVE_TIME";
        private static readonly string AttrSessionID = "SESSION_ID";
        private static readonly string AttrServerID = "SERVER_ID";

        public static readonly AttributeKey<RefValue<Int64>> PLAYER_ID = AttributeKey<RefValue<Int64>>.ValueOf(AttrPlayerID);
        public static readonly AttributeKey<string> OPEN_ID = AttributeKey<string>.ValueOf(AttrOpenID);
        public static readonly AttributeKey<IPEndPoint> IP_ADDR = AttributeKey<IPEndPoint>.ValueOf(AttrIpAddr);
        public static readonly AttributeKey<RefValue<Int64>> ACTIVE_TIME = AttributeKey<RefValue<Int64>>.ValueOf(AttrLastActiveTime);
        public static readonly AttributeKey<RefValue<Int64>> SESSION_ID = AttributeKey<RefValue<Int64>>.ValueOf(AttrSessionID);
        public static readonly AttributeKey<RefValue<Int64>> SERVER_ID = AttributeKey<RefValue<Int64>>.ValueOf(AttrServerID);

        public static Int32 BackLog = 128;

        public Server(Int32 thread_count = 1)
        {
            threads = new MultithreadEventLoopGroup(thread_count);
            LoggerProvider.Logger.Trace("WorkderPool Thread:{0}", thread_count);
        }

        public void Listen<H>(Int16 port,
            Int32 RCV_BUFF = 32 * 1024, Int32 SND_BUFF = 32 * 1024,
            Int32 READ_TIME_OUT = 30, Int32 WRITE_TIME_OUT = 30, Int32 RW_TIME_OUT = 30)
            where H : ByteToMessageDecoder, new()
        {
            var bootstrap = new ServerBootstrap();
            bootstrap
                .Group(threads)
                .Channel<TcpServerSocketChannel>()
                .Option(ChannelOption.SoBacklog, BackLog)
                .Option(ChannelOption.SoRcvbuf, RCV_BUFF)
                .Option(ChannelOption.SoSndbuf, SND_BUFF)
                .Option(ChannelOption.TcpNodelay, true)
                .Option(ChannelOption.SoReuseaddr, true)
                .Option(ChannelOption.SoKeepalive, true)
                .Option(ChannelOption.Allocator, PooledByteBufferAllocator.Default)

                .ChildAttribute(PLAYER_ID, new RefValue<Int64>(0))
                .ChildAttribute(IP_ADDR, new IPEndPoint(IPAddress.Any, 0))
                .ChildAttribute(OPEN_ID, "")
                .ChildAttribute(SERVER_ID, new RefValue<Int64>(0))
                .ChildAttribute(ACTIVE_TIME, new RefValue<Int64>(0))
                .ChildAttribute(SESSION_ID, new RefValue<Int64>(0))

                .ChildHandler(new ActionChannelInitializer<ISocketChannel>(channel =>
                {
                    if (channel.RemoteAddress is IPEndPoint)
                    {
                        channel.GetAttribute(IP_ADDR).Set(channel.RemoteAddress as IPEndPoint);
                    }
                    channel.GetAttribute(ACTIVE_TIME).Set(Platform.GetSeconds());
                    Singleton<ConnManager>.Instance.AddNewConntion(channel);
                    channel.GetAttribute(SESSION_ID).Get().Value = ConnectionSessionID++;

                    IChannelPipeline pipeline = channel.Pipeline;
                    pipeline.AddLast("TimeOut", new IdleStateHandler(READ_TIME_OUT, WRITE_TIME_OUT, RW_TIME_OUT));
                    pipeline.AddLast(TypeInstance<H>.New());

                    LoggerProvider.Logger.Trace("NewSession SessionID:{0} IpAddr:{1}", channel.GetSessionID(), channel.GetIpAddr());
                }));

            Task.WaitAll(bootstrap.BindAsync(port));
            LoggerProvider.Logger.Trace("Listen Port:{0} Type:{1}", port);
            ports.Add(bootstrap);
        }

        public void ShutDown()
        {
            Task.WaitAll(threads.ShutdownGracefullyAsync());
        }

        private List<ServerBootstrap> ports = new List<ServerBootstrap>();
        private MultithreadEventLoopGroup threads = null;
        private static AtomicInt64 ConnectionSessionID = new AtomicInt64(1);
    }
}
