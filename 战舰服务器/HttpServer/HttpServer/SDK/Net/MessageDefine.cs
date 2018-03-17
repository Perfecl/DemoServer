// Copyright (c) egmkang wang. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


namespace SharpServer.SDK.Net
{
    using System;
    using ProtoBuf;

    /// <summary>
    /// 客户端和服务器通讯的消息头(LittleEndian)
    /// 只有四个字节的长度, 其中高八位保留, 有效长度字段为低24bit
    /// </summary>
    public sealed class SSHead
    {
        public SSHead() { }
        public Int32 length;
        public Int16 msgid;
        public byte dest_type;
        public Int64 dest_id;

        public Int32 RealLength()
        {
            return length & Constant.MSG_LEN_MASK;
        }

        public Boolean Compressed()
        {
            return (length & Constant.MSG_COMPRESS_FLAG) != 0;
        }

        public void Clear()
        {
            this.length = 0;
        }
    }

    /// <summary>
    /// 收到的客户端消息
    /// </summary>
    public struct SSMessage
    {
        public SSMessage(IExtensible msg, long milliSeconds)
        {
            this.msg = msg;
            this.milliSeconds = milliSeconds;
        }
        public IExtensible msg;     //消息
        public long milliSeconds;  //收到消息的时间戳
    }
}
