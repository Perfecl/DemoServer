// Copyright (c) egmkang wang. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace SharpServer.SDK
{
    using System;
    using System.Runtime.CompilerServices;
    using System.Threading;

    public class AtomicInt64
    {
        public AtomicInt64()
        {
            value = 0L;
        }

        public AtomicInt64(Int64 v)
        {
            value = v;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static implicit operator Int64(AtomicInt64 v)
        {
            return v.Load();
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Int64 Load()
        {
            return Interlocked.Read(ref value);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void Store(Int64 v)
        {
            this.value = v;
            Interlocked.MemoryBarrier();
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Int64 GetAndInc()
        {
            return this.Add(1);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool CompareExchange(Int64 old_value, Int64 new_value)
        {
            return old_value == Interlocked.CompareExchange(ref this.value, new_value, old_value);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static AtomicInt64 operator ++(AtomicInt64 value)
        {
            return new AtomicInt64(value.Add(1));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static AtomicInt64 operator --(AtomicInt64 value)
        {
            return new AtomicInt64(value.Add(-1));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Int64 Add(Int32 v)
        {
            return Interlocked.Add(ref this.value, (Int64)v);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Int64 Add(Int64 v)
        {
            return Interlocked.Add(ref this.value, v);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static AtomicInt64 operator +(AtomicInt64 a, AtomicInt64 b)
        {
            return new AtomicInt64(a.Load() + b.Load());
        }

        private Int64 value = 0L;
    }
}
