// Copyright (c) egmkang wang. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace SharpServer.SDK.Net
{
    using System;
    using System.Net;
    using System.Text;
    using System.IO;
    using DotNetty.Buffers;
    using DotNetty.Transport.Channels;
    using ProtoBuf;

    public static partial class Ext
    {
        public static IByteBuffer SliceAndRetain(this IByteBuffer buffer, Int32 size)
        {
            IByteBuffer slice = buffer.ReadSlice(size);
            slice.Retain();
            return slice;
        }

        public static Int64 GetSessionID(this IChannel channel)
        {
            var value = channel.GetAttribute(Server.SESSION_ID);
            return value != null ? value.Get().Value : 0;
        }
        public static string GetOpenID(this IChannel channel)
        {
            var value = channel.GetAttribute(Server.OPEN_ID);
            return value != null ? value.Get() : null;
        }
        public static Int64 GetPlayerID(this IChannel channel)
        {
            var value = channel.GetAttribute(Server.PLAYER_ID);
            return value != null ? value.Get().Value : 0;
        }
        public static Int64 GetServerID(this IChannel channel)
        {
            var value = channel.GetAttribute(Server.SERVER_ID);
            return value != null ? value.Get().Value : 0;
        }
        public static IPEndPoint GetIpAddr(this IChannel channel)
        {
            var value = channel.GetAttribute(Server.IP_ADDR);
            return value != null ? value.Get() : null;
        }
        public static Int64 GetActiveTime(this IChannel channel)
        {
            var value = channel.GetAttribute(Server.ACTIVE_TIME);
            return value != null ? value.Get().Value : 0;
        }

        public static byte[] ToBytes(this IExtensible msg, int msgid)
        {
            int length = sizeof(short) + 2 + msg.GetExtensionObject(true).GetLength();
            var buffer = PooledByteBufferAllocator.Default.Buffer(length + sizeof(int)).WithOrder(ByteOrder.LittleEndian);
            buffer.WriteInt(length);
            buffer.WriteShort(msgid);

            MemoryStream stream = new MemoryStream(buffer.Array, buffer.WriterIndex + buffer.ArrayOffset, msg.GetExtensionObject(true).GetLength());
            Serializer.NonGeneric.Serialize(stream, msg);
            stream.Flush();
            buffer.SetWriterIndex(sizeof(int) + length);

            var ret = buffer.ReadBytes(buffer.ReadableBytes).ToArray();
            buffer.Release();
            return ret;
        }
    }
}
