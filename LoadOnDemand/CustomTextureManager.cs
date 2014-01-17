using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

namespace OnDemandLoading
{
    [KSPAddon(KSPAddon.Startup.EveryScene, false)]
    public class CustomTextureManager : MonoBehaviour
    {
        public class TextureData
        {
            public GameDatabase.TextureInfo Info;
            public UrlDir.UrlFile Url;
        }
        public static Dictionary<GameDatabase.TextureInfo, TextureData> UnloadedTextures = new Dictionary<GameDatabase.TextureInfo, TextureData>();
        public static Dictionary<GameDatabase.TextureInfo, TextureData> LoadedManagedTextures = null;
        public class TextureLoadingQueueItem
        {
            public TextureData Texture;
            public List<Action> OnLoadedCallbacks;
        }
        public static Dictionary<GameDatabase.TextureInfo, TextureLoadingQueueItem> TexturesToLoad = new Dictionary<GameDatabase.TextureInfo, TextureLoadingQueueItem>();

        public static TextureLoadingQueueItem currently_loading_target = null;
        public static Loaders.CustomPrimitiveLoader.AsyncState<GameDatabase.TextureInfo> currently_loading_texture = null;

        public static bool IsLoaded(GameDatabase.TextureInfo texture)
        {
            return !UnloadedTextures.ContainsKey(texture);
        }
        public static void RequestLoad(GameDatabase.TextureInfo texture, Action on_done_callback)
        {
            TextureLoadingQueueItem loadingQueueItem;
            if (!TexturesToLoad.TryGetValue(texture, out loadingQueueItem))
            {
                loadingQueueItem = TexturesToLoad[texture] = new TextureLoadingQueueItem()
                {
                    OnLoadedCallbacks = new List<Action>(),
                    Texture = UnloadedTextures[texture]
                };
            }
            loadingQueueItem.OnLoadedCallbacks.Add(on_done_callback);
        }
        public static void RequestUnload(GameDatabase.TextureInfo texture)
        {
            // Todo: No "still used by other" check... ref cnt as well?
            texture.texture.Resize(1, 1, TextureFormat.RGBA32, false);
            texture.texture.SetPixel(0, 0, Color.cyan);
            texture.texture.Apply(true, false);
            UnloadedTextures.Add(texture, LoadedManagedTextures[texture]);
            LoadedManagedTextures.Remove(texture);
        }
        public void Awake()
        {
            if (LoadedManagedTextures == null && GameDatabase.Instance.IsReady())
            {
                Debug.Log("LOD - CustomTextureManager initialized");
                LoadedManagedTextures = new Dictionary<GameDatabase.TextureInfo, TextureData>();
                UnloadedTextures =
                    LoD.ReplacedPartData
                    .SelectMany(p => p.Value.TextureFiles)
                    .Union(LoD.ReplacedInternalData.SelectMany(i => i.Value.TextureFiles))
                    .Select(tf => new
                    {
                        ti = GameDatabase.Instance.GetTextureInfo(tf.url),
                        url = tf
                    }).ToDictionary(el => el.ti, el => new TextureData()
                    {
                        Info = el.ti,
                        Url = el.url
                    });
            }
        }
        public void Update()
        {
            if (currently_loading_texture != null)
            {
                if (currently_loading_texture.IsDone)
                {
                    var loaded = currently_loading_texture.Data;
                    var trgInfo = currently_loading_target.Texture.Info;
                    Debug.Log("LOD - CTM - Texture loaded: " + trgInfo.name);
                    /*
                     * Atm, trgInfo.isNormalMap is always true. Cant use it!
                    if (!loaded.isNormalMap && trgInfo.isNormalMap)
                    {
                        loaded.texture = GameDatabase.BitmapToUnityNormalMap(loaded.texture);
                        loaded.isNormalMap = true;
                    }*/

                    try
                    {
                        trgInfo.texture.Resize(loaded.texture.width, loaded.texture.height, loaded.texture.format, loaded.texture.mipmapCount > 1);
                        trgInfo.texture.SetPixels(loaded.texture.GetPixels());
                        trgInfo.texture.Apply(true, false);
                        trgInfo.texture.Compress(true);
                        trgInfo.isNormalMap = loaded.isNormalMap;
                    }
                    catch (Exception err)
                    {
                        Debug.Log("Failed to copy loaded texture to " + trgInfo.name + " (" + (trgInfo.isNormalMap ? "NormalMap" : "Texture") + ")");
                        Debug.LogException(err);
                    }

                    GameObject.DestroyImmediate(loaded.texture);

                    UnloadedTextures.Remove(trgInfo);
                    LoadedManagedTextures.Add(trgInfo, currently_loading_target.Texture);

                    var callbacks = currently_loading_target.OnLoadedCallbacks;
                    currently_loading_target = null;
                    currently_loading_texture = null;

                    foreach (var callback in callbacks)
                    {
                        callback();
                    }
                }
            }
            if ((currently_loading_texture == null) && (TexturesToLoad.Count > 0))
            {
                var txToLoad = TexturesToLoad.First();
                Debug.Log("LOD - CTM - Start loading texture " + txToLoad.Key.name);
                currently_loading_target = txToLoad.Value;
                currently_loading_texture = Loaders.CustomPrimitiveLoader.LoadTexture(this, txToLoad.Value.Texture.Url, new System.IO.FileInfo(txToLoad.Value.Texture.Url.fullPath));
                TexturesToLoad.Remove(txToLoad.Key);
            }
        }
    }

}
