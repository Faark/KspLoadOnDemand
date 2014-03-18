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
        /*
         * NotImplementedException / Todo6: Make sure no interop between mono and net4 is leaking! E.g. passing strings...
         */

        static NativeBridge()
        {
            ("Device: " + SystemInfo.graphicsDeviceVersion).Log();
            if (!SystemInfo.graphicsDeviceVersion.StartsWith("Direct3D 9.0c"))
            {
                // Todo7: Support non-DX9
                throw new PlatformNotSupportedException("This mod currently only supports Direct3D 9.0c.");
            }

            DummyTexture = new Texture2D(2, 2, TextureFormat.RGB24, false);
            DummyTexture.Apply(true, true);
        }

        #region Callbacks
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void OnThumbnailUpdatedDelegate(int textureId);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void OnTextureLoadedDelegate(int textureId, IntPtr nativePtr);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        delegate void OnStatusUpdatedDelegate([MarshalAs(UnmanagedType.LPStr)] string text);
	    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	    delegate void OnRequestKspUpdateDelegate();

        static OnThumbnailUpdatedDelegate otu;
        static OnTextureLoadedDelegate otl;
        static OnStatusUpdatedDelegate osu;
        static OnRequestKspUpdateDelegate orku;
        #endregion
        #region PInvokes

        // Todo5: Switch to dynamic pinvoke, so we can support non-windows and use a proper static constructor. Shouldn't have a considerable perf impact.

        [DllImport("NetWrapper.native")]
        extern static unsafe void NlodSetup(
            [MarshalAs(UnmanagedType.LPStr)] string cacheDirectory,
            void* thumbUpdateCallback,
            void* textureLoadedCallback,
            void* statusUpdatedCallback, 
            void* requestUpdateFromKspThreadCallback
            );

        [DllImport("NetWrapper.native")]
        extern static unsafe int NlodRegisterTexture(
            [MarshalAs(UnmanagedType.LPStr)] string file,
            [MarshalAs(UnmanagedType.LPStr)] string key,
            void* thumbTexturePtr,
            bool isNormalMap
            );

        [DllImport("NetWrapper.native")]
        extern static void NlodRequestTextureLoad(int nativeId);

        [DllImport("NetWrapper.native")]
        extern static void NlodRequestTextureUnload(int nativeId);

        [DllImport("NetWrapper.native")]
        extern static unsafe bool NlodRequestedUpdate(void* deviceRefTexturePtr);

        [DllImport("NetWrapper.native")]
        extern static void NlodDebug_DumpTexture(IntPtr texturePtr, [MarshalAs(UnmanagedType.LPStr)]string file_name);

        #endregion




        /// <summary>
        /// We "abuse" this texture object as our device pointer!
        /// </summary>
        static Texture2D DummyTexture;


        /// <summary>
        /// General-Setup
        /// </summary>
        /// <param name="url"></param>
        public static unsafe void Setup(string cache_directory)
        {
            otu = OnThumbnailLoaded_Callback;
            otl = OnTextureUpdated_Callback;
            osu = OnStatusUpdated_Callback;
            orku = OnCallbackFromKspThreadRequested;
            NlodSetup(
                cache_directory,
                Marshal.GetFunctionPointerForDelegate(otu).ToPointer(),
                Marshal.GetFunctionPointerForDelegate(otl).ToPointer(),
                Marshal.GetFunctionPointerForDelegate(osu).ToPointer(),
                Marshal.GetFunctionPointerForDelegate(orku).ToPointer()
                );
        }

        static void OnThumbnailLoaded_Callback(int textureId)
        {
            // Todo4: Do we want to consume this event?
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
        public static void RequestTextureUnload(int id)
        {
            NlodRequestTextureUnload(id);
        }

        public static event Action<int, IntPtr> OnTextureLoaded;
        //public static event Action<int, IntPtr> OnThumbnailLoaded;

        public static void Debug_DumpTextureNative(Texture2D texture, String fileName)
        {
            NlodDebug_DumpTexture(texture.GetNativeTexturePtr(), fileName);
        }
    }
}
