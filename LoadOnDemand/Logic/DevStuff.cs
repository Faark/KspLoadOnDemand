using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using UnityEngine;
using System.Threading;

namespace LoadOnDemand.Logic
{

    public static class Test
    {
        public static Func<GameDatabase.TextureInfo, object> GetTextureResource { get; private set; }
        static Test()
        {
            var lodTexManager = Type.GetType("LoadOnDemand.Managers.TextureManager");
            if (lodTexManager == null)
            {
                GetTextureResource = key => null;
            }
            else
            {
                GetTextureResource = (Func<GameDatabase.TextureInfo, object>)Delegate.CreateDelegate(lodTexManager, lodTexManager.GetMethod("Get"));
            }
        }
    }

    [KSPAddon(KSPAddon.Startup.EveryScene, false)]
    public class DevStuff : MonoBehaviour
    {
        int winId = UnityEngine.Random.Range(0, int.MaxValue);
        static Rect pos = new Rect(100, 100, 80, 160);
        string ResourceText;
        public void OnGUI()
        {
            pos = GUI.Window(winId, pos, WinFunc, "LOD");
        }
        public static string Status;
        Resources.ResourceCollection refRes;
        void WinFunc(int id)
        {
            if (refRes == null)
            {
                if (GUILayout.Button("Ref All"))
                {
                    var list = Managers.TextureManager.ManagedTextures.Select(key => Managers.TextureManager.GetOrThrow(key)).ToList();
                    ("Referencing " + list.Count + " textures.").Log();
                    refRes = new Resources.ResourceCollection(list);
                }
            }
            else
            {
                //Temp: Disabled for perf?! GUILayout.Label(refRes.Resources.Count(el => el.Loaded) + "/" + refRes.Resources.Count() + " loaded");
                if (GUILayout.Button("Drop Ref"))
                {
                    refRes = null;
                }
            }
            if (GUILayout.RepeatButton("Info"))
            {
                if (ResourceText == null)
                {
                    ResourceText = String.Join(Environment.NewLine, Managers.TextureManager.iManagedTextures
                        .Where(el => el.Value.LoadedTextureRef != null && el.Value.LoadedTextureRef.IsAlive)
                        .Select(el => el.Value.LoadedTextureRef.Target.ToString())
                        .ToArray());
                    ResourceText.Log();
                    pos.width = 560;
                    pos.height = 640;
                }
            }
            else if (Event.current.type == EventType.Repaint)
            {
                pos.width = 80;
                pos.height = 160;
                ResourceText = null;
            }
            if (ResourceText != null)
            {
                GUILayout.Label(ResourceText);
            }
            GUILayout.Label(Status);
            if (GUILayout.Button("GC"))
            {
                System.GC.Collect(System.GC.MaxGeneration, GCCollectionMode.Forced);
            }
            GUILayout.Label(Managers.TextureManager.iManagedTextures.Count(el => el.Value.LoadedTextureRef != null && el.Value.LoadedTextureRef.IsAlive) + "/" + Managers.TextureManager.iManagedTextures.Count + " alive");
            GUI.DragWindow();
        }



        static void WaitForContinuation(System.Collections.IEnumerator state)
        {
            var stack = new Stack<System.Collections.IEnumerator>();
            stack.Push(state);
            while (stack.Count > 0)
            {
                var current = stack.Pop();
                if (current.MoveNext())
                {
                    stack.Push(current);
                    if (current.Current != null)
                    {
                        var asColl = current.Current as System.Collections.IEnumerator;
                        if (asColl != null)
                        {
                            stack.Push(asColl);
                        }
                        else
                        {
                            var asWWW = current.Current as WWW;
                            if (asWWW != null)
                            {
                                while (!asWWW.isDone)
                                {
                                    System.Threading.Thread.Sleep(50);
                                }
                            }
                            else
                            {
                                throw new NotImplementedException("No handler for type " + current.Current.GetType().FullName);
                            }
                        }
                    }
                }
                else
                {
                    // this one is done, continue...
                }
            }
            return;
        }
        public static Dictionary<string, DatabaseLoader<T>> GetLegacyLoadersFor<T>()
            where T : class
        {
            return AppDomain
                .CurrentDomain
                .GetAssemblies()
                .SelectMany(assembly => assembly
                    .GetTypes()
                    .Where(t => t.IsSubclassOf(typeof(DatabaseLoader<T>)) && t.GetCustomAttributes(typeof(DatabaseLoaderAttrib), true).Any()))
                .SelectMany(t =>
                {
                    var loader = (DatabaseLoader<T>)Activator.CreateInstance(t);
                    return loader.extensions.Select(ext => ext.ToUpper()).Distinct().Select(ext => new { ext = ext, loader = loader });
                }).ToDictionary(el => el.ext, el => el.loader);
        }
        static Dictionary<Type, System.Object> createdLoaders = new Dictionary<Type, object>();

        public static Texture2D ReadMBM(String file, out bool IsNormal)
        {
            var bytes = File.ReadAllBytes(file);
            using (var ms = new MemoryStream(bytes))
            using (var bs = new BinaryReader(ms))
            {
                bs.ReadString();
                var Width = bs.ReadInt32();
                var Height = bs.ReadInt32();
                IsNormal = bs.ReadInt32() == 1;
                var bitPerPixel = bs.ReadInt32();
                bool hasAlpha;
                int BytesPerPixel;
                switch (bitPerPixel)
                {
                    case 32:
                        BytesPerPixel = 4;
                        hasAlpha = true;
                        break;
                    case 24:
                        hasAlpha = false;
                        BytesPerPixel = 3;
                        break;
                    default:
                        throw new FormatException("Unsupported pixel format!");
                }
                ("Loading MBM " + (hasAlpha ? "with alpha" : "without alpha") + ": " + file).Log();
                var RawByteHeaderOffset = (int)ms.Position;
                if (bytes.Length != ((Width * Height * BytesPerPixel) + RawByteHeaderOffset))
                {
                    throw new FormatException("Invalid data size. Expected " + (RawByteHeaderOffset + (Width * Height * BytesPerPixel)) + " byte, got " + bytes.Length + " byte (both including a " + RawByteHeaderOffset + " byte header)");
                }


                var tex = new Texture2D(Width, Height, TextureFormat.RGBA32, false);
                var pos = RawByteHeaderOffset;
                for (var y = 0; y < Height; y++)
                {
                    for (var x = 0; x < Width; x++)
                    {
                        var col = new UnityEngine.Color32(bytes[pos++], bytes[pos++], bytes[pos++], 255);
                        if (hasAlpha)
                            col.a = bytes[pos++];
                        tex.SetPixel(x, y, col);
                    }
                }

                /* Slow version: 
                 *  does this rly work? have they flipped X/Y as well!?!
                for (var x = 0; x < Width; x++)
                {
                    for (var y = 0; y < Height; y++)
                    {
                        // SetPIxel is a slow bitch. Replace it! Maybe: http://www.codeproject.com/Tips/240428/Work-with-bitmap-faster-with-Csharp
                        var a = BytesPerPixel == 4 ? RawBytes[pos++] : 255;
                        bm.SetPixel(x, y, System.Drawing.Color.FromArgb(r, g, b, a));
                    }
                }*/

                tex.Apply(true, false);
                return tex;

            }
        }
        // Only works on ARGB32, RGB24 and Alpha8 textures that are marked readable



        // from unitywiki, just for test reasons:
        public class TextureScale
        {
            public class ThreadData
            {
                public int start;
                public int end;
                public ThreadData(int s, int e)
                {
                    start = s;
                    end = e;
                }
            }

            private static Color[] texColors;
            private static Color[] newColors;
            private static int w;
            private static float ratioX;
            private static float ratioY;
            private static int w2;
            private static int finishCount;
            private static Mutex mutex;

            public static void Point(Texture2D tex, int newWidth, int newHeight)
            {
                ThreadedScale(tex, newWidth, newHeight, false);
            }

            public static void Bilinear(Texture2D tex, int newWidth, int newHeight)
            {
                ThreadedScale(tex, newWidth, newHeight, true);
            }

            private static void ThreadedScale(Texture2D tex, int newWidth, int newHeight, bool useBilinear)
            {
                texColors = tex.GetPixels();
                newColors = new Color[newWidth * newHeight];
                if (useBilinear)
                {
                    ratioX = 1.0f / ((float)newWidth / (tex.width - 1));
                    ratioY = 1.0f / ((float)newHeight / (tex.height - 1));
                }
                else
                {
                    ratioX = ((float)tex.width) / newWidth;
                    ratioY = ((float)tex.height) / newHeight;
                }
                w = tex.width;
                w2 = newWidth;
                var cores = Mathf.Min(SystemInfo.processorCount, newHeight);
                var slice = newHeight / cores;

                finishCount = 0;
                if (mutex == null)
                {
                    mutex = new Mutex(false);
                }
                if (cores > 1)
                {
                    int i = 0;
                    ThreadData threadData;
                    for (i = 0; i < cores - 1; i++)
                    {
                        threadData = new ThreadData(slice * i, slice * (i + 1));
                        ParameterizedThreadStart ts = useBilinear ? new ParameterizedThreadStart(BilinearScale) : new ParameterizedThreadStart(PointScale);
                        Thread thread = new Thread(ts);
                        thread.Start(threadData);
                    }
                    threadData = new ThreadData(slice * i, newHeight);
                    if (useBilinear)
                    {
                        BilinearScale(threadData);
                    }
                    else
                    {
                        PointScale(threadData);
                    }
                    while (finishCount < cores)
                    {
                        Thread.Sleep(1);
                    }
                }
                else
                {
                    ThreadData threadData = new ThreadData(0, newHeight);
                    if (useBilinear)
                    {
                        BilinearScale(threadData);
                    }
                    else
                    {
                        PointScale(threadData);
                    }
                }

                tex.Resize(newWidth, newHeight);
                tex.SetPixels(newColors);
                tex.Apply();
            }

            public static void BilinearScale(System.Object obj)
            {
                ThreadData threadData = (ThreadData)obj;
                for (var y = threadData.start; y < threadData.end; y++)
                {
                    int yFloor = (int)Mathf.Floor(y * ratioY);
                    var y1 = yFloor * w;
                    var y2 = (yFloor + 1) * w;
                    var yw = y * w2;

                    for (var x = 0; x < w2; x++)
                    {
                        int xFloor = (int)Mathf.Floor(x * ratioX);
                        var xLerp = x * ratioX - xFloor;
                        newColors[yw + x] = ColorLerpUnclamped(ColorLerpUnclamped(texColors[y1 + xFloor], texColors[y1 + xFloor + 1], xLerp),
                                                               ColorLerpUnclamped(texColors[y2 + xFloor], texColors[y2 + xFloor + 1], xLerp),
                                                               y * ratioY - yFloor);
                    }
                }

                mutex.WaitOne();
                finishCount++;
                mutex.ReleaseMutex();
            }

            public static void PointScale(System.Object obj)
            {
                ThreadData threadData = (ThreadData)obj;
                for (var y = threadData.start; y < threadData.end; y++)
                {
                    var thisY = (int)(ratioY * y) * w;
                    var yw = y * w2;
                    for (var x = 0; x < w2; x++)
                    {
                        newColors[yw + x] = texColors[(int)(thisY + ratioX * x)];
                    }
                }

                mutex.WaitOne();
                finishCount++;
                mutex.ReleaseMutex();
            }

            private static Color ColorLerpUnclamped(Color c1, Color c2, float value)
            {
                return new Color(c1.r + (c2.r - c1.r) * value,
                                  c1.g + (c2.g - c1.g) * value,
                                  c1.b + (c2.b - c1.b) * value,
                                  c1.a + (c2.a - c1.a) * value);
            }
        }
        public static GameDatabase.TextureInfo LoadTextureCustom(UrlDir.UrlFile file)
        {
            bool isNormal = false;
            Texture2D tex;
            if (file.fileExtension.ToUpper() == "MBM")
            {
                tex = ReadMBM(new FileInfo(file.fullPath).FullName, out isNormal);
                //tex = MBMReader.Read(new System.IO.FileInfo(file.fullPath).FullName, out isNormal);
            }
            else
            {
                var www = new WWW("file://" + new FileInfo(file.fullPath).FullName);
                while (!www.isDone)
                {
                    System.Threading.Thread.Sleep(50);
                }
                tex = www.texture;
            }
            /*TextureScale.Bilinear(tex, 64, 64);
            var pixels = tex.GetPixels32();
            for (var x = 0; x < pixels.Length; x++)
            {
                /*if (pixels[x].a == 0)
                {
                    pixels[x].b = 0;
                    pixels[x].g = 0;
                    pixels[x].r = 0;
                }*/
/*                pixels[x].a = (byte)UnityEngine.Random.Range(0,256);
            }
            tex.SetPixels32(pixels);
            tex.Apply(true, false);*/
            return new GameDatabase.TextureInfo(tex, isNormal, true, false);
        }
        public static T LoadLegacy<T>(UrlDir.UrlFile file) where T : class
        {
            System.Object storedLoaders;
            Dictionary<string, DatabaseLoader<T>> loaders;
            if (createdLoaders.TryGetValue(typeof(T), out  storedLoaders))
            {
                loaders = (Dictionary<string, DatabaseLoader<T>>)storedLoaders;
            }
            else
            {
                createdLoaders[typeof(T)] = loaders = GetLegacyLoadersFor<T>();
            }
            var loader = loaders[file.fileExtension.ToUpper()];
            var loadState = loader.Load(file, new System.IO.FileInfo(file.fullPath));
            WaitForContinuation(loadState);
            if (!loader.successful)
                throw new Exception("error...");
            return loader.obj;
        }
    }
}