// Copyright (c) egmkang wang. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace SharpServer.SDK
{
    /// <summary>
    /// 用来生成一个堆上的值对象
    /// </summary>
    /// <typeparam name="T">必须是一个值对象</typeparam>
    public class RefValue<T> where T : struct
    {
        public RefValue() : this(default(T))
        {
        }

        public RefValue(T v = default(T))
        {
            this.Value = v;
        }

        public static implicit operator T(RefValue<T> v)
        {
            return v.Value;
        }

        public static implicit operator RefValue<T>(T v)
        {
            RefValue<T> ret = new RefValue<T>();
            ret.Value = v;
            return ret;
        }

        public T Value = default(T);
    }
}
