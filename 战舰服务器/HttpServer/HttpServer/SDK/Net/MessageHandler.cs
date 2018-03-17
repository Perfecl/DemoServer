// Copyright (c) egmkang wang. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace SharpServer.SDK.Net
{
    using System;
    using System.Collections.Generic;
    using DotNetty.Buffers;
    using DotNetty.Codecs;
    using DotNetty.Handlers.Timeout;
    using DotNetty.Transport.Channels;

    public abstract class MessageHandler<Decoder, Message> : ByteToMessageDecoder
        where Decoder : BytesToMessage<Message>, new()
    {
        protected abstract void ProcessMessage(IChannelHandlerContext context, List<Message> messages);

        protected abstract void OnConnectionClosing();

        protected override void Decode(IChannelHandlerContext context, IByteBuffer input, List<object> output)
        {
            list.Clear();
            decoder.Decode(input, this.list);
            this.ProcessMessage(context, this.list);
        }

        public override void ExceptionCaught(IChannelHandlerContext context, Exception exception)
        {
            LoggerProvider.Logger.Trace("SessionID:{0}, Exception:{1}", context.Channel.GetSessionID(), exception.ToString());
            Singleton<ConnManager>.Instance.RemoveConnection(context.Channel);
            context.CloseAsync();
        }

        public override void UserEventTriggered(IChannelHandlerContext context, object evt)
        {
            if (evt is IdleStateEvent && (evt as IdleStateEvent).State == IdleState.ReaderIdle)
            {
                LoggerProvider.Logger.Trace("SessionID:{0} TimeOut, Close", context.Channel.GetSessionID());
                context.CloseAsync();
            }
            else
            {
                base.UserEventTriggered(context, evt);
            }
        }

        private Decoder decoder = new Decoder();
        private List<Message> list = new List<Message>();
    }
}
