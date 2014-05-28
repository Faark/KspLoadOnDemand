using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;
using UnityEngine;

namespace LoadOnDemand
{
    internal class NativeBridge
    {
        static System.Object libraryRef;




        public static class PInvokeDelegates
        {
            [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
            public delegate void DumpTexture(IntPtr texturePtr, [MarshalAs(UnmanagedType.LPStr)]string file_name);

            // NotImplementedException / Todo6: Make sure no interop between mono and net4 is leaking! E.g. passing strings...
            // Todo: Do we need UnmanagedFunctionPointer(CallingConvention.Cdecl)? Some examples have them, others dont...
            [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
            public unsafe delegate void Setup(
                [MarshalAs(UnmanagedType.LPStr)] string cacheDirectory,
                void* thumbUpdateCallback,
                void* textureLoadedCallback,
                void* statusUpdatedCallback,
                void* requestUpdateFromKspThreadCallback,
                void* onSignalThreadIdlleCallback
                );
            [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
            public unsafe delegate int RegisterTexture(
                [MarshalAs(UnmanagedType.LPStr)] string file,
                [MarshalAs(UnmanagedType.LPStr)] string key,
                void* thumbTexturePtr,
                bool isNormalMap
                );
            [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
            public delegate void RequestTextureLoad(int nativeId);
            [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
            public delegate bool CancelTextureLoad(int nativeId);
            [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
            public delegate void RequestTextureUnload(int nativeId);
            [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
            public unsafe delegate bool RequestedUpdate(void* deviceRefTexturePtr);
            [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
            public delegate bool StartSignalMessages();
        }

        public static class Callbacks
        {
            
            [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
            public delegate void OnThumbnailUpdatedDelegate(int textureId);
            [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
            public delegate void OnTextureLoadedDelegate(int textureId, IntPtr nativePtr);
            [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
            public delegate void OnStatusUpdatedDelegate([MarshalAs(UnmanagedType.LPStr)] string text);
            [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
            public delegate void OnRequestKspUpdateDelegate();
            [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
            public delegate void OnSignalThreadIdleDelegate();
        }

       


        static PInvokeDelegates.Setup NlodSetup;
        static PInvokeDelegates.RegisterTexture NlodRegisterTexture;
        static PInvokeDelegates.RequestTextureLoad NlodRequestTextureLoad;
        static PInvokeDelegates.CancelTextureLoad NlodCancelTextureLoad;
        static PInvokeDelegates.RequestTextureUnload NlodRequestTextureUnload;
        static PInvokeDelegates.RequestedUpdate NlodRequestedUpdate;
        static PInvokeDelegates.StartSignalMessages NlodStartSignalMessages;
        static PInvokeDelegates.DumpTexture NlodDebug_DumpTexture;

        // references have to be kept alive?!
        static Callbacks.OnThumbnailUpdatedDelegate otu;
        static Callbacks.OnTextureLoadedDelegate otl;
        static Callbacks.OnStatusUpdatedDelegate osu;
        static Callbacks.OnRequestKspUpdateDelegate orku;
        static Callbacks.OnSignalThreadIdleDelegate osti;

        static string CopyEmbededNativeDllAndGetPath()
        {

            var path = System.IO.Path.Combine(Config.Current.GetCacheDirectory(), "NetWrapper.native");
            /*
             * Todo9: Date of the created file seems wrong (hasn't changed for more than 1h, even after i deleted the file it was re-created with the wrong date)... Investiage?!
             * 
             * 
            System.IO.File.Exists(path).ToString().Log();
            System.IO.File.Delete(path);
            System.IO.File.Exists(path).ToString().Log();
            path.Log();
             * 
             */
            var data = System.Reflection.Assembly.GetExecutingAssembly().GetManifestResourceStream("LoadOnDemand.EmbededResources.NetWrapper.dll");
            System.IO.File.WriteAllBytes(path, new System.IO.BinaryReader(data).ReadBytes((int)data.Length));
            return path;
            /*
            IntPtr lib = LoadLibrary(path);
            if (lib == IntPtr.Zero)
                throw new Exception("LoadOnDemand failed loaded its native library:" + path);
            else
            {
                ("LoadOnDemand has loaded its native library: " + path).Log();
            }*/
        }



        /// <summary>
        /// We "abuse" this texture object as our device pointer!
        /// </summary>
        static Texture2D DummyTexture;

        static unsafe void TrySetupLegacyBridge()
        {
            NlodSetup = Legacy.LegacyNativeWrapper.Setup;
            NlodRegisterTexture = Legacy.LegacyNativeWrapper.RegisterTexture;
            NlodRequestTextureLoad = Legacy.LegacyNativeWrapper.RequestTextureLoad;
            NlodCancelTextureLoad = Legacy.LegacyNativeWrapper.CancelTextureLoad;
            NlodRequestTextureUnload = Legacy.LegacyNativeWrapper.RequestTextureUnload;
            NlodRequestedUpdate = Legacy.LegacyNativeWrapper.RequestedUpdate;
            NlodStartSignalMessages = Legacy.LegacyNativeWrapper.StartSignalMessages;
            throw new NotImplementedException("DebugDumpTexture stil used? not implemented, at least!");
        }
        static void SetupNativeBridge()
        {
            try
            {

                ("Device: " + SystemInfo.graphicsDeviceVersion).Log();
                if (!SystemInfo.graphicsDeviceVersion.StartsWith("Direct3D 9.0c"))
                {
                    // Todo7: Support non-DX9
                    throw new PlatformNotSupportedException("This mod currently only supports Direct3D 9.0c.");
                }


                var lib = new UnmanagedLibrary(CopyEmbededNativeDllAndGetPath()).NotNull();
                NlodSetup = lib.GetUnmanagedFunction<PInvokeDelegates.Setup>("NlodSetup").NotNull();
                NlodRegisterTexture = lib.GetUnmanagedFunction<PInvokeDelegates.RegisterTexture>("NlodRegisterTexture").NotNull();
                NlodRequestTextureLoad = lib.GetUnmanagedFunction<PInvokeDelegates.RequestTextureLoad>("NlodRequestTextureLoad").NotNull();
                NlodCancelTextureLoad = lib.GetUnmanagedFunction<PInvokeDelegates.CancelTextureLoad>("NlodCancelTextureLoad").NotNull();
                NlodRequestTextureUnload = lib.GetUnmanagedFunction<PInvokeDelegates.RequestTextureUnload>("NlodRequestTextureUnload").NotNull();
                NlodRequestedUpdate = lib.GetUnmanagedFunction<PInvokeDelegates.RequestedUpdate>("NlodRequestedUpdate").NotNull();
                NlodStartSignalMessages = lib.GetUnmanagedFunction<PInvokeDelegates.StartSignalMessages>("NlodStartSignalMessages").NotNull();

                NlodDebug_DumpTexture = lib.GetUnmanagedFunction<PInvokeDelegates.DumpTexture>("NlodDebug_DumpTexture");

                libraryRef = lib;


                DummyTexture = new Texture2D(2, 2, TextureFormat.RGB24, false);
                DummyTexture.Apply(true, true);

            }
            catch (Exception err)
            {
                try
                {
                    throw new NotImplementedException("Legacy mode isn't even close to a working state!");
                    TrySetupLegacyBridge();
                    err.Log("Failed to set up native module, using LegacyWrapper instead (horrible perf!)");
                    Logic.ActivityGUI.SetText("Failed to set up native module, using LegacyWrapper instead (horrible perf!)");
                }
                catch (Exception fallbackErr)
                {
                    var bothErr = new AggregateException("Failed to create NativeBridge, even legacy mode!", new[] { err, fallbackErr });
                    Logic.ActivityGUI.SetError(bothErr);
                    throw bothErr;
                }
            }
        }

        /// <summary>
        /// General-Setup
        /// </summary>
        /// <param name="url"></param>
        public static unsafe void Setup(string cache_directory)
        {
            SetupNativeBridge();




            otu = OnThumbnailLoaded_Callback;
            otl = OnTextureUpdated_Callback;
            osu = OnStatusUpdated_Callback;
            orku = OnCallbackFromKspThreadRequested;
            osti = OnSignalThreadIdle_Callback;
            NlodSetup(
                cache_directory,
                Marshal.GetFunctionPointerForDelegate(otu).ToPointer(),
                Marshal.GetFunctionPointerForDelegate(otl).ToPointer(),
                Marshal.GetFunctionPointerForDelegate(osu).ToPointer(),
                Marshal.GetFunctionPointerForDelegate(orku).ToPointer(),
                Marshal.GetFunctionPointerForDelegate(osti).ToPointer()
                );
            var t = new System.Threading.Thread(() =>
            {
                NlodStartSignalMessages();
            });
            t.Name = "Net4ToMonoMessageThread";
            t.IsBackground = true;
            t.Start();
        }

        //static int i = 0;
        static void OnSignalThreadIdle_Callback()
        {
            //i++;
            //Logic.DevStuff.Status = i.ToString();
            // regular reverse pinvoke is required so mono can kill this thread if required.
        }
        static void OnThumbnailLoaded_Callback(int textureId)
        {
            // Todo4: Do we want to consume this event?
            Logic.WorkQueue.AddJob(() =>
            {
                OnThumbnailLoaded.TryCall(textureId);
            });
        }
        static void OnTextureUpdated_Callback(int textureId, IntPtr nativePtr)
        {
            Logic.WorkQueue.AddJob(() =>
            {
                OnTextureLoaded.TryCall(textureId, nativePtr);
            });
        }
        static void OnStatusUpdated_Callback(string newStatus)
        {
            // Todo8: Still needed? Get rid of it if not
            Logic.DevStuff.Status = newStatus;
            //Logic.DevStuff.Status = System.Runtime.InteropServices.Marshal.PtrToStringAnsi(payload);
            //System.Runtime.InteropServices.Marshal.FreeHGlobal(payload);
        }
        static unsafe void OnCallbackFromKspThreadRequested()
        {
            Logic.WorkQueue.AddJob(() =>
            {
                if (NlodRequestedUpdate(DummyTexture.GetNativeTexturePtr().ToPointer()))
                {
                    OnCallbackFromKspThreadRequested();
                }
            });
        }
        public unsafe static int RegisterTextureAndRequestThumbLoad(string source_file, string cacheId, IntPtr thumbTextureTarget, bool isNormalMap)
        {
            return NlodRegisterTexture(
                source_file,
                cacheId,
                thumbTextureTarget.ToPointer(),
                isNormalMap
                );
        }
        public static void RequestTextureLoad(int id)
        {
            if (id < 0)
            {
                throw new Exception("Cannot loade texture with index < 0. Texture was most likely not yet initialized.");
            }
            NlodRequestTextureLoad(id);
        }
        public static bool CancelTextureLoad(int id)
        {
            return NlodCancelTextureLoad(id);
        }
        public static void RequestTextureUnload(int id)
        {
            NlodRequestTextureUnload(id);
        }

        public static event Action<int, IntPtr> OnTextureLoaded;
        public static event Action<int> OnThumbnailLoaded;
        
        public static void Debug_DumpTextureNative(Texture2D texture, String fileName)
        {
            NlodDebug_DumpTexture(texture.GetNativeTexturePtr(), fileName);
        }
        
    }
}
