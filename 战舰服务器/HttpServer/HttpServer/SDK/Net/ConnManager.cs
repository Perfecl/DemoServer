// Copyright (c) egmkang wang. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace SharpServer.SDK.Net
{
    using System;
    using System.Threading.Tasks;
    using System.Collections.Concurrent;
    using DotNetty.Transport.Channels;
    
    public class ConnManager
    {
        public ConnManager() { }

        public void AddNewConntion(IChannel channel)
        {
            if (channel == null) return;
            Int64 id = channel.GetSessionID();
            WeakReference c = null;
            this.sessions.TryRemove(id, out c);
            this.sessions.TryAdd(id, new WeakReference(channel));
        }

        public void RemoveConnection(IChannel channel)
        {
            if (channel == null) return;
            Int64 id = channel.GetSessionID();
            WeakReference c = null;
            this.sessions.TryRemove(id, out c);
        }

        public ConcurrentDictionary<Int64, WeakReference> Sessions { get { return this.sessions; } }
        ConcurrentDictionary<Int64, WeakReference> sessions = new ConcurrentDictionary<long, WeakReference>();
    }
}
