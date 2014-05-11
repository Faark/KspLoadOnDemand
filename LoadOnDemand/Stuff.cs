using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;
using System.Security.Permissions;
using System.Runtime.ConstrainedExecution;

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
        public static T NotNull<T>(this T self) where T : class
        {
            if (self == null)
            {
                throw new ArgumentNullException("self");
            }
            return self;
        }
        public static void TryCall<T1, T2>(this Action<T1, T2> self, T1 t1, T2 t2)
        {
            if (self != null)
            {
                self(t1, t2);
            }
        }
        public static void TryCall<T1>(this Action<T1> self, T1 t1)
        {
            if (self != null)
            {
                self(t1);
            }
        }
        public static int ToInt(this Double self)
        {
            return (int)self;
        }
    }

    public class AggregateException : Exception
    {

        public System.Collections.ObjectModel.ReadOnlyCollection<Exception> InnerExceptions { get; private set; }
        public AggregateException(string message, params Exception[] innerExceptions)
            : base(InnerExText(message, innerExceptions))
        {
            if (innerExceptions == null)
            {
                throw new ArgumentNullException("innerException");
            }
            InnerExceptions = new System.Collections.ObjectModel.ReadOnlyCollection<Exception>(innerExceptions);
        }

        public AggregateException(string message, IEnumerable<Exception> innerExceptions) : this(message, innerExceptions.ToArray()) { }

        static String InnerExText(String msg, Exception[] innerExceptions)
        {
            var sb = new StringBuilder(msg);
            sb.AppendLine("Contained exceptions:");
            foreach (var inner in innerExceptions)
            {
                sb.AppendLine("---------------------------------------");
                sb.AppendLine(inner.ToString());
            }
            return sb.ToString();
        }
    }
    /// <summary>
    /// Utility class to wrap an unmanaged DLL and be responsible for freeing it.
    /// http://blogs.msdn.com/b/jmstall/archive/2007/01/06/typesafe-getprocaddress.aspx
    /// </summary>
    /// <remarks>This is a managed wrapper over the native LoadLibrary, GetProcAddress, and
    /// FreeLibrary calls.
    /// </example>
    public sealed class UnmanagedLibrary : IDisposable
    {
        #region Safe Handles and Native imports
        // See http://msdn.microsoft.com/msdnmag/issues/05/10/Reliability/ for more about safe handles.
        [SecurityPermission(SecurityAction.LinkDemand, UnmanagedCode = true)]
        sealed class SafeLibraryHandle : SafeHandle, IDisposable
        {
            private SafeLibraryHandle() : base(IntPtr.Zero, true) { }

            public override bool IsInvalid
            {
                get
                {
                    return this.handle == (System.IntPtr)(-1) || this.handle == (System.IntPtr)0;
                }
            }

            protected override bool ReleaseHandle()
            {
                return NativeMethods.FreeLibrary(handle);
            }
        }

        static class NativeMethods
        {
            const string s_kernel = "kernel32";
            [DllImport(s_kernel, CharSet = CharSet.Auto, BestFitMapping = false, SetLastError = true)]
            public static extern SafeLibraryHandle LoadLibrary(string fileName);

            [ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
            [DllImport(s_kernel, SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            public static extern bool FreeLibrary(IntPtr hModule);

            [DllImport(s_kernel)]
            public static extern IntPtr GetProcAddress(SafeLibraryHandle hModule, String procname);
        }
        #endregion // Safe Handles and Native imports

        /// <summary>
        /// Constructor to load a dll and be responible for freeing it.
        /// </summary>
        /// <param name="fileName">full path name of dll to load</param>
        /// <exception cref="System.IO.FileNotFound">if fileName can't be found</exception>
        /// <remarks>Throws exceptions on failure. Most common failure would be file-not-found, or
        /// that the file is not a  loadable image.</remarks>
        public UnmanagedLibrary(string fileName)
        {
            try
            {
                m_hLibrary = NativeMethods.LoadLibrary(fileName);
                if (m_hLibrary.IsInvalid)
                {

                    int dwLastError = Marshal.GetLastWin32Error();
                    ("ERROR: Native load failed with error code " + dwLastError + ".").ForceLog();
                    if ((dwLastError & 0x80000000) != 0x80000000)
                        dwLastError = (dwLastError & 0x0000FFFF) | unchecked((int)0x80070000);
                    Marshal.ThrowExceptionForHR(dwLastError);
                }
            }
            catch (Exception err)
            {
                throw new Exception("Failed to load native library: ", err);
            }
        }

        /// <summary>
        /// Dynamically lookup a function in the dll via kernel32!GetProcAddress.
        /// </summary>
        /// <param name="functionName">raw name of the function in the export table.</param>
        /// <returns>null if function is not found. Else a delegate to the unmanaged function.
        /// </returns>
        /// <remarks>GetProcAddress results are valid as long as the dll is not yet unloaded. This
        /// is very very dangerous to use since you need to ensure that the dll is not unloaded
        /// until after you're done with any objects implemented by the dll. For example, if you
        /// get a delegate that then gets an IUnknown implemented by this dll,
        /// you can not dispose this library until that IUnknown is collected. Else, you may free
        /// the library and then the CLR may call release on that IUnknown and it will crash.</remarks>
        public TDelegate GetUnmanagedFunction<TDelegate>(string functionName) where TDelegate : class
        {
            IntPtr p = NativeMethods.GetProcAddress(m_hLibrary, functionName);

            // Failure is a common case, especially for adaptive code.
            if (p == IntPtr.Zero)
            {
                return null;
            }
            Delegate function = Marshal.GetDelegateForFunctionPointer(p, typeof(TDelegate));

            // Ideally, we'd just make the constraint on TDelegate be
            // System.Delegate, but compiler error CS0702 (constrained can't be System.Delegate)
            // prevents that. So we make the constraint system.object and do the cast from object-->TDelegate.
            object o = function;

            return (TDelegate)o;
        }

        #region IDisposable Members
        /// <summary>
        /// Call FreeLibrary on the unmanaged dll. All function pointers
        /// handed out from this class become invalid after this.
        /// </summary>
        /// <remarks>This is very dangerous because it suddenly invalidate
        /// everything retrieved from this dll. This includes any functions
        /// handed out via GetProcAddress, and potentially any objects returned
        /// from those functions (which may have an implemention in the
        /// dll).
        /// </remarks>
        public void Dispose()
        {
            if (!m_hLibrary.IsClosed)
            {
                m_hLibrary.Close();
            }
        }

        // Unmanaged resource. CLR will ensure SafeHandles get freed, without requiring a finalizer on this class.
        SafeLibraryHandle m_hLibrary;

        #endregion
    } // UnmanagedLibrary
    public static class Logger
    {
#if DEBUG_BUT_I_DONT_USE_IT_ATM
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
        }
#else

        public static void Log(this String self)
        {
            UnityEngine.Debug.Log("LoadOnDemand: " + self);
        }
        public static void Log(this Exception self, String comment = null)
        {
            if (comment != null)
            {
                Log(comment + " " + self.ToString());
            }
            else
            {
                Log(self.ToString());
            }
        }
        public static void ForceLog(this String self)
        {
            self.Log();
        }
#endif
    }
}
