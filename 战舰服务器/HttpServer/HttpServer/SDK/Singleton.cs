// Copyright (c) egmkang wang. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace SharpServer.SDK
{
    public class Singleton<T> where T : class, new()
    {
        private static volatile T v = TypeInstance<T>.New();
        public static T Instance { get { return v; } }

        public Singleton()
        {
        }
    }
}
