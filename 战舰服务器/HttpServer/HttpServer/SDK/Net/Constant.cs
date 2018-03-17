// Copyright (c) egmkang wang. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace SharpServer.SDK.Net
{
    using System;

    public static partial class Constant
    {
        //消息长度掩码
        public static readonly Int32 MSG_LEN_MASK = (int)((~0u) >> 8);
        //压缩Flag
        public static readonly UInt32 MSG_COMPRESS_FLAG = 1 << 24;
        //客户端包头
        public static readonly Int32 MIN_SS_HEAD_LEN = 15;
        //客户端可以发送的最大包长度(超过会被丢弃掉)
        public static readonly Int32 MAX_CS_MSG_LEN = 10 * 1024;
        //最大的包长度
        public static readonly Int32 MAX_MSG_LEN = 1 * 1024 * 1024;
        //最小的(包)压缩长度
        public static readonly Int32 MIN_COMPRESS_LEN = 128;
    }
}
