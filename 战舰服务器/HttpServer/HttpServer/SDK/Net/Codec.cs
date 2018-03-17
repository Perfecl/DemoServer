// Copyright (c) egmkang wang. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace SharpServer.SDK.Net
{
    using System;
    using System.Text;
    using System.Reflection;
    using System.Collections.Generic;
    using DotNetty.Buffers;
    using DotNetty.Transport.Channels;
    using DotNetty.Codecs;
    using ProtoBuf;
    using System.IO;

    public partial class ProtoBuffDecode
    {
        static Dictionary<int, Func<MemoryStream, IExtensible>> parsers = new Dictionary<int, Func<MemoryStream, IExtensible>>();
        static OnceFlag load = new OnceFlag();

        public static IExtensible NewMessage(int msgid, byte[] array)
        {
            load.CallOnce(Load);

            if (parsers.ContainsKey(msgid))
            {
                return parsers[msgid](array != null ? new MemoryStream(array) : null);
            }
            return null;
        }
    }

    public interface BytesToMessage<Message>
    {
        void Decode(IByteBuffer input, List<Message> output);
    }

    public sealed class CSMsgDecoder : BytesToMessage<SSMessage>
    {
        public void Decode(IByteBuffer input, List<SSMessage> output)
        {
            input = input.WithOrder(ByteOrder.LittleEndian);
            output.Clear();
            SSHead head = new SSHead();

            while (input.ReadableBytes >= Constant.MIN_SS_HEAD_LEN)
            {
                input.MarkReaderIndex();
                head.length = input.ReadInt();
                head.msgid = input.ReadShort();
                head.dest_type = input.ReadByte();
                head.dest_id = input.ReadLong();

                if (head.RealLength() > input.ReadableBytes)
                {
                    input.ResetReaderIndex();
                    return;
                }
                if (head.RealLength() > Constant.MAX_MSG_LEN)
                {
                    throw new TooLongFrameException("");
                }

                
                IByteBuffer buffer = input.SliceAndRetain(head.RealLength());
                var bytes = buffer.ReadSlice(buffer.ReadableBytes).ToArray();
                output.Add(new SSMessage(ProtoBuffDecode.NewMessage(head.msgid, bytes), Platform.GetMilliSeconds()));
                buffer.Release();
            }
        }
    }
}
