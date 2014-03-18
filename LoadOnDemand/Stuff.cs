using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace LoadOnDemand
{
    public static class Stuff
    {
        static int UnityThreadId;
        static Queue<string> delayedMsgs = new Queue<string>();
        static Stuff()
        {
            System.IO.File.WriteAllText("LoadOnDemand.log", "");
            UnityThreadId = System.Threading.Thread.CurrentThread.ManagedThreadId;
        }
    }

    public static class Extensions
    {
        public static void TryCall<T1, T2>(this Action<T1, T2> self, T1 t1, T2 t2)
        {
            if (self != null)
            {
                self(t1, t2);
            }
        }
    }

    public class AggregateException : Exception
    {

        public System.Collections.ObjectModel.ReadOnlyCollection<Exception> InnerExceptions { get; private set; }
        public AggregateException(string message, params Exception[] innerExceptions)
            : base(message)
        {
            if (innerExceptions == null)
            {
                throw new ArgumentNullException("innerException");
            }
            InnerExceptions = new System.Collections.ObjectModel.ReadOnlyCollection<Exception>(innerExceptions);
        }

        public AggregateException(string message, IEnumerable<Exception> innerExceptions) : this(message, innerExceptions.ToArray()) { }
        

        public override string ToString()
        {
            var sb = new StringBuilder(base.ToString());
            sb.AppendLine("Contained exceptions:");
            foreach (var inner in InnerExceptions)
            {
                sb.AppendLine("---------------------------------------");
                sb.AppendLine(inner.ToString());
            }
            return sb.ToString();
        }

    }
    public static class Logger
    {
        /*
        const bool Async = false;
        struct LogItem { public String Text; public DateTime Time;  }
        static Queue<LogItem> QueuedMessages = new Queue<LogItem>();
        static Logger()
        {

            if (Async)
            {
                System.IO.File.WriteAllText("LoadOnDemand.log", "Logger started at " + DateTime.Now.ToString() + " (Async)" + Environment.NewLine);
                var thread = new System.Threading.Thread(() =>
                {
                    while (true)
                    {
                        var any = false;
                        var sb = new StringBuilder();
                        while (true)
                        {
                            LogItem Current;
                            lock (QueuedMessages)
                            {
                                if (QueuedMessages.Count <= 0)
                                {
                                    break;
                                }
                                Current = QueuedMessages.Dequeue();
                            }
                            any = true;
                            sb.AppendLine(Current.Time.ToString() + ": " + Current.Text);
                        }
                        if (any)
                        {
                            System.IO.File.AppendAllText("LoadOnDemand.log", sb.ToString());
                        }
                        System.Threading.Thread.Sleep(1000);
                    }
                });
                thread.Name = "LoggerThread";
                thread.IsBackground = true;
                thread.Start();
            }
            else
            {
                System.IO.File.WriteAllText("LoadOnDemand.log", "Logger started at " + DateTime.Now.ToString() + " (Sync)" + Environment.NewLine);
            }
        }
        public static void Log(this String self)
        {
            lock (QueuedMessages)
            {
                if (Async)
                {
                    QueuedMessages.Enqueue(new LogItem { Text = self, Time = DateTime.Now });
                }
                else
                {
                    System.IO.File.AppendAllText("LoadOnDemand.log", DateTime.Now.ToString() + ": " + self + Environment.NewLine);
                }
            }
        }
        public static void Log(this Exception self, String comment = null)
        {
            // Todo20: Make sure this error log is actually written to disc before continuing...
            // Todo20: comment...
            Log(self.ToString());
        }*/

        public static void Log(this String self)
        {
            UnityEngine.Debug.Log("LoadOnDemand: " + self);
        }
        public static void Log(this Exception self, String comment = null)
        {
            if (comment != null)
            {
                Log(comment + ": " + self.ToString());
            }
            else
            {
                Log(self.ToString());
            }
        }
    }
}
