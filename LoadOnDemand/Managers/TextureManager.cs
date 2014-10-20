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
            public bool Requested;
            public WeakReference LoadedTextureResource = null;
            public IntPtr UnloadedTexture;
            public GameDatabase.TextureInfo Info;
            public UrlDir.UrlFile File;
            public int NativeId = -1;

            public TextureData(UrlDir.UrlFile file)
            {
                File = file;
                Info = new GameDatabase.TextureInfo(CreateEmptyThumbnailTexture(), false, true, false);
                Info.texture.name = Info.name = file.url;
            }
            public TextureData(GameDatabase.TextureInfo info, UrlDir.UrlFile file)
            {
                File = file;
                Info = info;
                Info.texture = CreateEmptyThumbnailTexture();
                Info.texture.name = Info.name = file.url;
            }
        }

        /// <summary>
        /// This object represents a loaded texture. It should be ref-counted, but GC'ed in case someone fcks it up...
        /// 
        /// 
        /// 
        /// This is a singelton-per-texture class. It is not allowed to exist more than once per Texture/TextureData!
        /// All existing instance have to be referenced via "live" resource references or corresponding LoadedTextureRef WeakRefs.
        /// </summary>
        private class TextureResource
        {
            public bool Loaded
            {
                get
                {
                    // Todo11: Replace this for perf!
                    // Todo18: ...
                    return Data.Info.texture.GetNativeTexturePtr() != Data.UnloadedTexture;
                }
            }
            public TextureData Data { get; private set; }
            public int RefCount;
#if true
            public List<WeakReference> ReferencedBy = new List<WeakReference>();
#endif
            public void Ref(TextureReference r)
            {
                RefCount++;
#if true
                ReferencedBy.Add(new WeakReference(r));
                ("Referenced " + Data.Info.name + ", new cnt is " + RefCount).Log();
#endif
            }
            public void Dec(TextureReference r)
            {
                RefCount--;
#if true
                ReferencedBy.RemoveAll(el => !el.IsAlive || (el.Target == r));
                ("Dropped reference to " + Data.Info.name + ", new cnt is " + RefCount).Log();
#endif
                if (RefCount == 0)
                {
                    // Todo15: This was supposed to free memory as required, ideally on GC, instantly so more can be used.
                    // Atm it's done delayed (since GC thread can't do shit), thus killing that initial concept. revive it think about dealing with it.
                    // It at least makes it harder to get close to the memory limit.
                    ReleaseTextureDelayed(Data);
                }
            }
            public override string ToString()
            {
#if true
                var sb = new StringBuilder(Data.Info.name);
                sb.Append(", Active references: ").Append(RefCount);
                sb.AppendLine(", Lifetime references:");
                var any = false;
                foreach (var currentRef in ReferencedBy)
                {
                    if (currentRef.IsAlive)
                    {
                        any = true;
                        sb.AppendLine("  ----------------------------------------");
                        sb.AppendLine(((TextureReference)currentRef.Target).CreationLog);
                    }
                }
                if (!any)
                {
                    sb.AppendLine("  NONE!");
                }
                return sb.ToString();
#else
                return Data.Info.name;
#endif
            }
            public TextureResource(TextureData data)
            {
                Data = data;
                if (!data.Requested)
                {
                    if (data.Info.texture.GetNativeTexturePtr() == data.UnloadedTexture)
                    {
                        ("Texture referenced, requesting load: " + data.NativeId).Log();
                        data.Requested = true;
                        Logic.ActivityGUI.HighResStarting();
                        NativeBridge.RequestTextureLoad(data.NativeId);
                    }
                    else
                    {
                        ("Texture referenced, highdetail still loaded: " + data.NativeId).Log();
                    }
                }
                else
                {
                    ("Texture referenced, but highdetails seems already requested: " + data.NativeId).Log();
                }
            }
            ~TextureResource()
            {
                if (RefCount > 0)
                {
                    // someone fck up releasing resources...
                    RefCount = 0;
                    ReleaseTextureDelayed(Data);
                }
            }
        }
        /// <summary>
        /// A public reference to a texture resource. Should be released once you don't need it anymore!
        /// </summary>
        private class TextureReference : Resources.IResource
        {
            public TextureResource Resource { get; private set; }
            public bool Loaded
            {
                get
                {
                    return Resource.Loaded;
                }
            }
#if true
            public string CreationLog;
#endif

            public void Release()
            {
                if (Resource != null)
                {
                    Resource.Dec(this);
                    Resource = null;
                }
            }
            public Resources.IResource Clone(bool throwIfDead)
            {
                if (Resource == null && throwIfDead)
                {
                    throw new InvalidOperationException();
                }
                return new TextureReference(Resource);
            }
            public override string ToString()
            {
                if (Resource == null)
                {
                    return "Released Texture";
                }
                else
                {
                    return "Reference to " + Resource.ToString();
                }
            }

            public TextureReference(TextureResource fromResource)
            {
                var res = Resource = fromResource;
                if (res != null)
                {
#if true
                    CreationLog = String.Join(Environment.NewLine + "  ", Environment.StackTrace.Split(new[] { Environment.NewLine }, StringSplitOptions.None));
#endif
                    Resource.Ref(this);
                }
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
                if ((textureData.NativeId == -1) || (textureData.File == null))
                {
                    return new Resources.ResourceDummy();
                }
                TextureResource textureResource;
                if (textureData.LoadedTextureResource == null || !textureData.LoadedTextureResource.IsAlive)
                {
                    textureResource = new TextureResource(textureData);
                    textureData.LoadedTextureResource = new WeakReference(textureResource);
                }
                else
                {
                    // Todo: Potential race condition. Hope we asap get .net 4 and thus WeakRef.TryGet... don't see another way to fix that if-else
                    textureResource = (TextureResource)textureData.LoadedTextureResource.Target;
                }
                return new TextureReference(textureResource);
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
                if (data.LoadedTextureResource != null && data.LoadedTextureResource.IsAlive)
                {
                    var currentResource = (TextureResource)data.LoadedTextureResource.Target;
                    if (currentResource.RefCount > 0)
                    {
                        // obj seems alive again, probably ref'ed again?
                        return;
                    }
                }
                data.LoadedTextureResource = null; // lets call it dead...

                var currentNative = data.Info.texture.GetNativeTexturePtr();
                if (currentNative == data.UnloadedTexture)
                {
                    if (NativeBridge.CancelTextureLoad(data.NativeId))
                    {
                        ("Texture reference " + data.NativeId + " dropped! (not loaded anyway & load canceled)").Log();
                        Logic.ActivityGUI.HighResCanceled();
                        data.Requested = false;
                    }
                    else
                    {
                        ("Texture reference " + data.NativeId + " dropped! (not loaded anyway, doing nothing)").Log();
                    }
                }
                else
                {
                    ("Texture reference " + data.NativeId + " dropped, unloading texture!").Log();
                    data.Info.texture.UpdateExternalTexture(data.UnloadedTexture);
                    NativeBridge.RequestTextureUnload(data.NativeId);
                }
            });
        }

        public static bool IsManaged(GameDatabase.TextureInfo textureObject)
        {
            return iManagedTextures.ContainsKey(textureObject);
        }
        public static bool IsSupported(UrlDir.UrlFile file, Boolean logUnsupported = true)
        {
            switch (file.fileExtension.ToUpper())
            {
                /*
                 * KSPs own file format... got a custom loader
                 */
                case "MBM":
                // God bless http://www.codeproject.com/Articles/31702/NET-Targa-Image-Reader for doing the heavy lifting,
                // though i would have prefered KSP to never support TGAs
                case "TGA":
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
                    return true;
                default:
                    if (logUnsupported)
                    {
                        ("LOD Unsupported Texture: " + file.fullPath).Log();
                    }
                    return false;
            }
        }
        static TextureManager()
        {
            NativeBridge.OnTextureLoaded += NativeBridge_OnTextureLoaded;
            NativeBridge.OnThumbnailLoaded += NativeBridge_OnThumbnailLoaded;
            NativeBridge.OnTexturePrepared += NativeBridge_OnTexturePrepared;
        }

        static void NativeBridge_OnTexturePrepared()
        {
            Logic.ActivityGUI.PrepareFinished();
        }
        /*
        static void NativeBridge_OnThumbnailLoaded(int nativeId, IntPtr loadedTexturePtr)
        {
            // Todo: setPtr? Or do we assign it from native to the actual tmp texture?
            // nativeIdToDataLookup[nativeId].ThumbLoaded = true;
        }*/
        static void NativeBridge_OnThumbnailLoaded(int nativeId, IntPtr loadedThumbPtr)
        {
            if (loadedThumbPtr == IntPtr.Zero)
            {

            }
            else
            {
                var textureData = nativeIdToDataLookup[nativeId];
                if (textureData.Info.texture.GetNativeTexturePtr() == textureData.UnloadedTexture)
                {
                    textureData.Info.texture.UpdateExternalTexture(loadedThumbPtr);
                }
                UnloadEmptyThumbnailTexture(textureData.UnloadedTexture);
                textureData.UnloadedTexture = loadedThumbPtr;
            }
        }
        static void NativeBridge_OnTextureLoaded(int nativeId, IntPtr loadedTexturePtr)
        {
            Logic.ActivityGUI.HighResFinished();
            var textureData = nativeIdToDataLookup[nativeId];
            textureData.Requested = false;
            if ((textureData.LoadedTextureResource != null) && textureData.LoadedTextureResource.IsAlive && (((TextureResource)textureData.LoadedTextureResource.Target).RefCount > 0))
            {
                ("Texture " + nativeId + " loaded, assigning.").Log();
                //NativeBridge.DumpTextureNative(textureData.Info.texture, "TexEx_" + DateTime.Now.Ticks + "_" + nativeId + "_ORIGINAL.png");
                //textureData.Info.texture.UpdateExternalTexture(textureData.Info.texture.GetNativeTexturePtr());
                textureData.Info.texture.UpdateExternalTexture(loadedTexturePtr);
                //NativeBridge.DumpTextureNative(textureData.Info.texture, "TexEx_" + DateTime.Now.Ticks + "_" + nativeId + "_REPLACEMENT.png");
            }
            else
            {
                ("Texture " + nativeId + " loaded but not of use anymore, unloading.").Log();
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
            /*bool isNormal = false;
            var tex = Legacy.LegacyLoader.LoadImage(new System.IO.FileInfo(texture.File.fullPath), ref isNormal, texture.Info.isNormalMap);
            System.IO.File.WriteAllBytes(
                texture.File.fullPath + ".png.stock",
                tex.EncodeToPNG());

            //using (var text = System.IO.File.CreateText(texture.File.fullPath + ".stock.txt"))
            using (var text2 = System.IO.File.CreateText(texture.File.fullPath + ".stockbyte.txt"))
            {
                for (int y = tex.height - 1; y >= 0; y--) // Unity textures are upside down.
                {
                    for (int x = 0; x < tex.width; x++)
                    {
                        var c = tex.GetPixel(x, y);
                        Color32 c2 = c;
                        //text.Write("{" + c.a + "|" + c.r + "|" + c.g + "|" + c.b + "}");
                        text2.Write("{" + c2.a + "|" + c2.r + "|" + c2.g + "|" + c2.b + "}");
                    }
                    //text.WriteLine();
                    text2.WriteLine();
                }
            }*/
            /*
            var oldUnloadedPtr = texture.Info.texture.GetNativeTexturePtr();
            texture.Info.texture.UpdateExternalTexture(tex.GetNativeTexturePtr());
            tex.UpdateExternalTexture(oldUnloadedPtr);
            return;*/
            var imgConfig = Config.Current.GetImageConfig(texture.File);
            var imgSettings = ImageSettings.Combine(imgConfig.ImageSettings, Config.Current.DefaultImageSettings);
            Logic.ActivityGUI.PrepareStarting();
            ("Sending img settings for texture: {HRC=" + imgSettings.HighResCompress + ", TC=" + imgSettings.ThumbnailCompress + ", TE=" + imgSettings.ThumbnailEnabled + ",TW=" + imgSettings.ThumbnailWidth + ",TH=" + imgSettings.ThumbnailHeight + "}").Log();
            texture.NativeId = NativeBridge.RegisterTextureAndRequestThumbLoad(
                new System.IO.FileInfo(texture.File.fullPath).FullName,
                imgConfig.CacheKey,
                //texture.UnloadedTexture,
                texture.Info.isNormalMap,
                imgSettings
                );
            if (!imgSettings.HighResEnabled.Value)
            {
                texture.File = null;
            }
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
                    /*if (data.NativeId >= nativeIdToDataLookup.Length)
                    {
                        Array.Resize(ref nativeIdToDataLookup, data.NativeId + 1);
                    }
                    nativeIdToDataLookup[data.NativeId] = data;*/
                    // Todo14: This doesn't update nativeIdDataLookup and users has to call FinalizeSetup manually. While fine, we might be able to improve that API without killink lots of Setup(...)-perf on lots of textures. Suggestion: "Bulk" call option
                }
                "Setup adding texture".Log();
                iManagedTextures.Add(data.Info, data);
                return data.Info;
            }
        }
        public static void FinalizeSetup() // must be called sync after setups!
        {
            List<Exception> failedLoads = null;
            ("FinSetup: " + DelayLoading).Log();
            var loaded = 0;
            if (DelayLoading)
            {
                "Sending Texture Info to net4".Log();
                foreach (var tex in iManagedTextures)
                {
                    try
                    {
                        SetupNative(tex.Value);
                        loaded++;
                    }
                    catch (Exception err)
                    {
                        (failedLoads ?? (failedLoads = new List<Exception>())).Add(err);
                    }
                }
                DelayLoading = false;
            }
            nativeIdToDataLookup = new TextureData[iManagedTextures.Count > 0 ? iManagedTextures.Max(tex => tex.Value.NativeId) + 1 : 0];
            foreach (var tex in iManagedTextures)
            {
                if (tex.Value.NativeId >= 0)
                {
                    nativeIdToDataLookup[tex.Value.NativeId] = tex.Value;
                }
            }
            if (failedLoads != null)
            {
                throw new AggregateException("Failed to load " + failedLoads.Count + " of " + (loaded + failedLoads.Count) + " textures!", failedLoads);
            }
        }
        static Texture2D CreateEmptyThumbnailTexture()
        {
            var w = Config.Current.DefaultImageSettings.ThumbnailWidth.Value;
            var h = Config.Current.DefaultImageSettings.ThumbnailHeight.Value;
            var tex = new Texture2D(w, h, TextureFormat.RGBA32, false);
            var startsBlack = true;
            var b = Color.black;
            var m = Color.magenta;
            b.a = m.a = 0.5f;
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
        static void UnloadEmptyThumbnailTexture(IntPtr texture)
        {
            // Todo: Find out whether this is actually a good way to unload textures.
            var t = Texture2D.CreateExternalTexture(Config.Current.DefaultImageSettings.ThumbnailWidth.Value, Config.Current.DefaultImageSettings.ThumbnailHeight.Value, TextureFormat.RGBA32, false, false, texture);
            UnityEngine.Object.Destroy(t);
       }

        // Makes lod taking over control over an already loaded texture.
        public static void ForceManage(GameDatabase.TextureInfo info)
        {
            // this one is experimental for now.
            // it tries to resolve the source file by the textures name (yea, kinda stupid)
            // it unloads the currently laoded texture
            // it places an unloaded "dummy"
            
            // warning, this might not work with derived TextureInfos! Or think about this replacing TextureInfo in the first place

            if(iManagedTextures.ContainsKey(info)){
                "Texture already managed!".Log();
                return;
            }

            UrlDir.UrlFile file = Stuff.ResolveTextureInfo(info); // Todo: Resolve file...
            if (file == null)
            {
                throw new NotSupportedException("Could not find a source file for this texture. This kinda textures are not currently supported.");
            }
            if (info.texture != null)
            {
                UnityEngine.Resources.UnloadAsset(info.texture);
            }

            var data = new TextureData(info, file);
            if(!DelayLoading){
                SetupNative(data);
                // Todo14: See above
            }
            "ForceManage, Adding texture".Log();
            iManagedTextures.Add(data.Info, data);
        }

    }
}
