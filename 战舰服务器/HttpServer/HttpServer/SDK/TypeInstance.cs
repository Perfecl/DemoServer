// Copyright (c) egmkang wang. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace SharpServer.SDK
{
    using System;
    using System.Linq.Expressions;

    /// <summary>
    /// 创建T类型的实例, 比new T()快不少
    /// </summary>
    /// <typeparam name="T">类型</typeparam>
    public sealed class TypeInstance<T>
    {
        public static readonly Func<T> New = Expression.Lambda<Func<T>>(Expression.New(typeof(T))).Compile();
    }
}
