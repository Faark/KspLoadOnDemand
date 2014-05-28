using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace LoadOnDemand.Legacy
{
    /// <summary>
    /// NativeBridge currently is designed to "go native".
    /// This class un-does this...
    /// </summary>
    class LegacyNativeWrapper
    {
        internal static NativeBridge.Callbacks.OnRequestKspUpdateDelegate OnRequestKspUpdate;
        internal static NativeBridge.Callbacks.OnSignalThreadIdleDelegate OnSignalThreadIdle;
        internal static NativeBridge.Callbacks.OnStatusUpdatedDelegate OnStatusUpdated;
        internal static NativeBridge.Callbacks.OnTextureLoadedDelegate OnTextureLoaded;
        internal static NativeBridge.Callbacks.OnThumbnailUpdatedDelegate OnThumbnailUpdated;



        internal static unsafe void Setup(
            string cacheDirectory,
            void* thumbUpdateCallback,
            void* textureLoadedCallback,
            void* statusUpdatedCallback,
            void* requestUpdateFromKspThreadCallback,
            void* onSignalThreadIdlleCallback
            )
        {
            Stuff.SetDelegateForFunctionPointer(ref OnRequestKspUpdate, requestUpdateFromKspThreadCallback);
            Stuff.SetDelegateForFunctionPointer(ref OnSignalThreadIdle, onSignalThreadIdlleCallback);
            Stuff.SetDelegateForFunctionPointer(ref OnStatusUpdated, statusUpdatedCallback);
            Stuff.SetDelegateForFunctionPointer(ref OnTextureLoaded, textureLoadedCallback);
            Stuff.SetDelegateForFunctionPointer(ref OnThumbnailUpdated, thumbUpdateCallback);

            LegacyInterface.CacheDirectory = cacheDirectory;
        }
        internal static unsafe int RegisterTexture(string file, string key, void* thumbTexturePtr, bool isNormalMap)
        {
            var internalId = LegacyInterface.Textures.Count;
            var tex = new LegacyInterface.InternalTexture(){ CacheKey = key, SourceFile = file, IsNormal = isNormalMap, ThumbTexturePtr = new IntPtr(thumbTexturePtr)};
            LegacyInterface.Textures.Add(tex);
            LegacyInterface.TexturesToPrepare.Enqueue(tex);
            return internalId;
        }
        internal static void RequestTextureLoad(int nativeId)
        {
            throw new NotImplementedException();
       } 
        internal static bool CancelTextureLoad(int nativeId)
        {
            throw new NotImplementedException();
        }
        internal static void RequestTextureUnload(int nativeId)
        {
            throw new NotImplementedException();
        }
        internal static unsafe bool RequestedUpdate(void* deviceRefTexturePtr)
        {
            throw new NotImplementedException();
        }
        internal static bool StartSignalMessages()
        {
            throw new NotImplementedException();
        }
    }
}
