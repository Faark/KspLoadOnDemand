using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

namespace LoadOnDemand.Managers
{
    public static class TextureManager
    {
        // Todo11: GetNativeTexturePtr seems to be rather expensive... think about using it less frequent?!
        internal class TextureData
        {
            public WeakReference LoadedTextureRef = null;
            public IntPtr UnloadedTexture;
            public GameDatabase.TextureInfo Info;
            public UrlDir.UrlFile File;
            public int NativeId = -1;

            public TextureData(UrlDir.UrlFile file)
            {
                File = file;
                Info = new GameDatabase.TextureInfo(CreateEmptyThumbnailTexture(), false, true, false);

                Info.name = file.url;
            }
        }
        /// <summary>
        /// This is a singelton-per-texture class. It is not allowed to exist more than once per Texture/TextureData!
        /// All existing instance have to be referenced via corresponding LoadedTextureRef WeakRefs.
        /// </summary>
        private class TextureResource : Resources.IResource
        {
            public Managers.TextureManager.TextureData Data { get; private set; }
            public bool Loaded
            {
                get
                {
                    // Todo11: Replace this for perf!
                    // Todo18: ...
                    return Data.Info.texture.GetNativeTexturePtr() != Data.UnloadedTexture;
                }
            }
            public TextureResource(Managers.TextureManager.TextureData data)
            {
                Data = data;
                if (data.Info.texture.GetNativeTexturePtr() == data.UnloadedTexture)
                {
                    ("Texture referenced, requesting load: " + data.NativeId).Log();
                    NativeBridge.RequestTextureLoad(data.NativeId);
                }
                else
                {
                    ("Texture referenced, highdetail still loaded: " + data.NativeId).Log();
                }
            }
            ~TextureResource()
            {
                // Todo15: This was supposed to free memory as required, ideally on GC, instantly so more can be used.
                // Atm it's done delayed (since GC thread can't do shit), thus killing that initial concept. revive it think about dealing with it.
                // It at least makes it harder to get close to the memory limit.
                ReleaseTextureDelayed(Data);
            }
        }
        internal static Dictionary<GameDatabase.TextureInfo, TextureData> iManagedTextures = new Dictionary<GameDatabase.TextureInfo, TextureData>();
        public static IEnumerable<GameDatabase.TextureInfo> ManagedTextures
        {
            get
            {
                return iManagedTextures.Keys;
            }
        }

        static TextureData[] nativeIdToDataLookup;
        public static Resources.IResource Get(GameDatabase.TextureInfo textureObject)
        {
            TextureData textureData;
            if (iManagedTextures.TryGetValue(textureObject, out textureData))
            {
                Resources.IResource textureReference;
                if (textureData.LoadedTextureRef == null || !textureData.LoadedTextureRef.IsAlive)
                {
                    textureReference = new TextureResource(textureData);
                    textureData.LoadedTextureRef = new WeakReference(textureReference);
                }
                else
                {
                    // Todo: Potential race condition. Hope we asap get .net 4 and thus WeakRef.TryGet... don't see another way to fix that if-else
                    textureReference = (TextureResource)textureData.LoadedTextureRef.Target;
                }
                return textureReference;
            }
            else
            {
                return null;
            }
        }
        public static Resources.IResource GetOrThrow(GameDatabase.TextureInfo textureObject)
        {
            var res = Get(textureObject);
            if (res == null)
            {
                throw new ArgumentOutOfRangeException("TextureManager does not manage this GameDatabase.TextureInfo[Name:" + textureObject.name.ToString() + "]");
            }
            return res;
        }

        /// <summary>
        /// Called from the GC-Thread & TextureResource's finalizer... => delay deletion
        /// </summary>
        /// <param name="data"></param>
        static void ReleaseTextureDelayed(TextureData data)
        {
            Logic.WorkQueue.AddJob(() =>
            {
                if (data.LoadedTextureRef.IsAlive)
                {
                    // object was revived before this one was killed... unload request can be scrapped?!
                }
                else
                {
                    var currentNative = data.Info.texture.GetNativeTexturePtr();
                    if (currentNative == data.UnloadedTexture)
                    {
                        ("Texture reference " + data.NativeId + " dropped! (not loaded anyway, doing nothing)").Log();
                    }
                    else
                    {
                        ("Texture reference " + data.NativeId + " dropped, unloading texture!").Log();
                        data.Info.texture.UpdateExternalTexture(data.UnloadedTexture);
                        NativeBridge.RequestTextureUnload(data.NativeId);
                    }
                }
            });
        }

        public static bool IsSupported(UrlDir.UrlFile file)
        {
            switch (file.fileExtension.ToUpper())
            {
                    /*
                     * KSPs own file format... got a custom loader
                     */
                case "MBM":
            /*
             * For legacy files do we use Image.FromStream, that should recognize pretty much every thing in system.drawing.imaging.imageformat
             * ( http://msdn.microsoft.com/en-us/library/system.drawing.imaging.imageformat%28v=vs.110%29.aspx )
             * 
             * Lets add a few extensions for now... hope they work
             */
                case "PNG":
                case "BMP":
                case "EMF":
                case "GIF":
                case "ICO":
                case "JPEG":
                case "JPG":
                case "TIFF":
                case "TIF":
                case "WMF":
                //case "TGA": Todo16: Well, it doesn't support TGA :)
                    return true;
                default:
                    ("LOD Unsupported Texture: " + file.fullPath).Log();
                    return false;
            }
        }
        static TextureManager()
        {
            var cfg = Config.Current;
            if (cfg.ThumbnailFormat != TextureFormat.ARGB32)
                throw new NotSupportedException("Currnet thumbnails have to be ARGB32!");
            NativeBridge.Setup(cfg.GetCacheDirectory());
            NativeBridge.OnTextureLoaded += NativeBridge_OnTextureLoaded;
            //NativeBridge.OnThumbnailLoaded += NativeBridge_OnThumbnailLoaded;
        }
        /*
        static void NativeBridge_OnThumbnailLoaded(int nativeId, IntPtr loadedTexturePtr)
        {
            // Todo: setPtr? Or do we assign it from native to the actual tmp texture?
            // nativeIdToDataLookup[nativeId].ThumbLoaded = true;
        }*/
        static void NativeBridge_OnTextureLoaded(int nativeId, IntPtr loadedTexturePtr)
        {
            var textureData = nativeIdToDataLookup[nativeId];
            if (textureData.LoadedTextureRef.IsAlive)
            {
                //NativeBridge.DumpTextureNative(textureData.Info.texture, "TexEx_" + DateTime.Now.Ticks + "_" + nativeId + "_ORIGINAL.png");
                //textureData.Info.texture.UpdateExternalTexture(textureData.Info.texture.GetNativeTexturePtr());
                textureData.Info.texture.UpdateExternalTexture(loadedTexturePtr);
                //NativeBridge.DumpTextureNative(textureData.Info.texture, "TexEx_" + DateTime.Now.Ticks + "_" + nativeId + "_REPLACEMENT.png");
            }
            else
            {
                NativeBridge.RequestTextureUnload(nativeId);
            }
        }

        static bool DelayLoading = true;
        static void SetupNative(TextureData texture)
        {
            ("SetupNative on texture: " + texture.Info.name).Log();
            if (texture.Info.texture == null)
            {
                throw new Exception("Texture null. Todo21: Probably related.");
            }
            /*if ("ThunderAerospace/TacLifeSupportContainers/FoodTexture" == texture.Info.name)
            {
                (texture.Info== null ? "INFO NULL": "INFO OK").Log();
                (texture.Info.texture == null ? "TEX NULL" : "TEX OK").Log();
                (texture.Info.texture.name).Log();
                (texture.Info.texture.format.ToString()).Log();
                texture.Info.texture.width.ToString().Log();
            }*/
            texture.UnloadedTexture = texture.Info.texture.GetNativeTexturePtr();
            texture.NativeId = NativeBridge.RegisterTextureAndRequestThumbLoad(
                new System.IO.FileInfo(texture.File.fullPath).FullName,
                Config.Current.GetCacheFileFor(texture.File),
                texture.UnloadedTexture,
                texture.Info.isNormalMap);
            texture.Info.isReadable = false;
            ("Native ID received: " + texture.NativeId + " for " + texture.Info.name).Log();
        }
        public static GameDatabase.TextureInfo Setup(UrlDir.UrlFile file)
        {
            var url = file.url;
            var existing = GameDatabase.Instance.GetTextureInfo(url);
            if (existing != null)
            {
                if (iManagedTextures.ContainsKey(existing))
                {
                    "Texture already managed!".Log();
                    return existing;
                }
                else
                {
                    // Todo: Do we unload it? How to get the actual file? (Well, we have the URlFile, so thats not a problem)
                    throw new NotImplementedException("Texture was created by external code. Atm not suppored!");
                }
            }
            else
            {
                var data = new TextureData(file);
                GameDatabase.Instance.databaseTexture.Add(data.Info);
                ("Texture created." + (DelayLoading ? " (Delayed)" : "(Forwarded)")).Log();
                if (!DelayLoading)
                {
                    SetupNative(data);
                    // Todo14: This doesn't update nativeIdDataLookup and users has to call FinalizeSetup manually. While fine, we might be able to improve that API without killink lots of Setup(...)-perf on lots of textures
                }
                iManagedTextures.Add(data.Info, data);
                return data.Info;
            }
        }
        public static void FinalizeSetup()
        {
            ("FinSetup: " + DelayLoading).Log();
            if (DelayLoading)
            {
                "Sending Texture Info to net4".Log();
                foreach (var tex in iManagedTextures)
                {
                    SetupNative(tex.Value);
                }
                DelayLoading = false;
            }
            nativeIdToDataLookup = new TextureData[iManagedTextures.Max(tex => tex.Value.NativeId) + 1];
            foreach (var tex in iManagedTextures)
            {
                nativeIdToDataLookup[tex.Value.NativeId] = tex.Value;
            }
        }
        static Texture2D CreateEmptyThumbnailTexture()
        {
            var w = Config.Current.ThumbnailWidth;
            var h = Config.Current.ThumbnailHeight;
            var tex = new Texture2D(w, h, TextureFormat.RGBA32, false);
            var startsBlack = true;
            for (var x = 0; x < w; x++)
            {
                var isBlack = (startsBlack = !startsBlack);
                for (var y = 0; y < h; y++)
                {
                    tex.SetPixel(x, y, isBlack ? Color.black : Color.magenta);
                    isBlack = !isBlack;
                }
            }
            tex.Apply(true, false);
            return tex;
        }

    }
}
