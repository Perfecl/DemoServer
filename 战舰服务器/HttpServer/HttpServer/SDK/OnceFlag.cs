// Copyright (c) egmkang wang. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace SharpServer.SDK
{
    using System;
    using System.Threading;

    public class OnceFlag
    {
        public delegate void Action();

        public void CallOnce(Action fn)
        {
            if (run != 0) return;
            bool got_lock = false;
            try
            {
                spin_lock.Enter(ref got_lock);
                if (Interlocked.CompareExchange(ref this.run, 1, 0) == 0)
                {
                    fn();
                }
            }
            catch (Exception e)
            {
                LoggerProvider.Logger.Error("OnceFlag Exception:{0}", e.Message);
            }
            finally
            {
                if (got_lock) spin_lock.Exit();
            }
        }
        private volatile int run = 0;
        private SpinLock spin_lock = new SpinLock();
    }
}
