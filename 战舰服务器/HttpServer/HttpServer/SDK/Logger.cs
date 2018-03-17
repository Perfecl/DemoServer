// Copyright (c) egmkang wang. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace SharpServer.SDK
{
    using NLog;
    using NLog.Config;
    using NLog.Targets;

    public sealed class LoggerProvider
    {
        public static NLog.Logger InitLogger(string file_prefix, LogLevel level)
        {
            LogManager.ThrowExceptions = true;

            var fileTarget = new FileTarget();
            config.AddTarget("file", fileTarget);
            fileTarget.FileName = "${basedir}/log/" + file_prefix + "_${date:format=yyyy-MM-dd_HH}.txt";
            fileTarget.Layout = @"${date:format=HH\:mm\:ss.fff} [${level:uppercase=true}] ${message}";

#if DEBUG
            var consoleTarget = new ColoredConsoleTarget();
            config.AddTarget("console", consoleTarget);
            consoleTarget.Layout = @"${date:format=HH\:mm\:ss.fff} [${level:uppercase=true}] ${message}";
            var rule1 = new LoggingRule("*", level, consoleTarget);
            config.LoggingRules.Add(rule1);
#endif

            var rule = new LoggingRule("*", level, fileTarget);
            config.LoggingRules.Add(rule);
            LogManager.Configuration = config;
            if (Logger == null) Logger = LogManager.GetLogger(file_prefix);

            return Logger;
        }

        public static void ChangeLevel(LogLevel level)
        {
            foreach (var rule in config.LoggingRules)
            {
                rule.EnableLoggingForLevels(level, LogLevel.Fatal);
            }
            LogManager.ReconfigExistingLoggers();
        }

        private static LoggingConfiguration config = new LoggingConfiguration();
        public static NLog.Logger Logger { get; private set; }
    }
}
