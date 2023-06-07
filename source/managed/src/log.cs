// Copyright(c) 2019 - 2022, KaoruXun All rights reserved.

using System;
using System.Runtime.CompilerServices;

namespace MetaDotManagedCore
{
    public class Log
    {
        // print stack trace before message
        public static bool ShowStackTrace = true;
        // don't print method name
        public static bool Shortened = true;

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern bool METADOT_COPY_FILE(string from, string to);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern void METADOT_PROFILE_BEGIN_SESSION(string name, string path);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern string METADOT_CURRENT_CONFIG();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern void METADOT_PROFILE_END_SESSION();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void metadot_trace(string message);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void metadot_info(string message);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void metadot_warn(string message);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern void metadot_error(string message);

        private static string buildMessage(string fileName, string memberName, int line, string message)
        {
            if (!ShowStackTrace)
                return message;
            fileName = fileName.Replace('\\', '/');
            return "[" + fileName.Substring(fileName.LastIndexOf("/") + 1) + (Shortened ? "" : (":" + memberName)) + ":" + line + "]: " + message;

        }
        public static void METADOT_TRACE(string message,
           [CallerFilePath] string fileName = "",
           [CallerMemberName] string memberName = "",
           [CallerLineNumber] int line = 0)
        {
            metadot_trace(buildMessage(fileName, memberName, line, message));
        }
        public static void METADOT_INFO(string message,
          [CallerFilePath] string fileName = "",
          [CallerMemberName] string memberName = "",
          [CallerLineNumber] int line = 0)
        {
            metadot_info(buildMessage(fileName, memberName, line, message));
        }
        public static void METADOT_WARN(string message,
         [CallerFilePath] string fileName = "",
         [CallerMemberName] string memberName = "",
         [CallerLineNumber] int line = 0)
        {
            metadot_warn(buildMessage(fileName, memberName, line, message));
        }
        public static void METADOT_ERROR(string message,
       [CallerFilePath] string fileName = "",
       [CallerMemberName] string memberName = "",
       [CallerLineNumber] int line = 0)
        {
            metadot_error(buildMessage(fileName, memberName, line, message));
        }

    }
}