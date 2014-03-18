using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

namespace LoadOnDemand
{
    static class ConfigExtensions
    {
        public static T GetValue<T>(this ConfigNode self, string name, Func<string, T> converter, T defaultValue)
        {
            return self.HasValue(name) ? converter(self.GetValue(name)) : defaultValue;
        }
    }
    class Config
    {
        public static Config Current { get; private set; }
        System.IO.FileInfo cfgFileLocation;
        private Config() {
            // defaults:
            
            ThumbnailEnabled = true;
            ThumbnailWidth = 32;
            ThumbnailHeight = 32;
            ThumbnailFormat = TextureFormat.ARGB32;

            CompressTextures = true;
        }
        /*
        * 
        * LOD_CONFIG
        * 
        *      Thumb
         *         Enabled = true // turn of thumb usage... though no fallback for editor!
        *          Width  = 16
        *          Height = 16
        *          Format = RGBA32 // dont rly think others make sense, though...
        *          
        *      CompressTextures = true // Compress images to DXT1/5... creates 
        *      
        *      Cache
         *          key
        *               Url
        *               Size
        *               Date
        *               // Original_Hash?
        *               // ProcessedDataIsNormal
        *               // CacheName (no path, no extension, can be used for both thumb and)
        *          
        *      // SPECIAL_IMG
        *          // kinda same as IMG, but for embeded images... we cannot improve loading times here, 
        *          // but could reduce or compress them... caching might be an idea, but a whitelist seems necessary?
        * 
        * 
        */
        static Config()
        {
            Current = new Config();
            var gameData = System.IO.Path.Combine(UrlDir.ApplicationRootPath, "GameData");
            var lodDir = System.IO.Path.Combine(gameData, "LoadOnDemand");
            System.IO.Directory.CreateDirectory(lodDir);
            Current.cfgFileLocation = new System.IO.FileInfo(System.IO.Path.Combine(lodDir, "LoadOnDemand.cfg"));

            if (!Current.cfgFileLocation.Exists)
            {
                ("No config found, starting with defaults.").Log();
            }
            else
            {
                var cfgNode = ConfigNode.Load(Current.cfgFileLocation.FullName);
                if (cfgNode.HasNode("Thumbnail"))
                {
                    var thumbNode = cfgNode.GetNode("Thumbnail");
                    Current.ThumbnailEnabled = thumbNode.GetValue("Enabled", s => bool.Parse(s), Current.ThumbnailEnabled);
                    Current.ThumbnailWidth = thumbNode.GetValue("Width", s => int.Parse(s), Current.ThumbnailWidth);
                    Current.ThumbnailHeight = thumbNode.GetValue("Height", s => int.Parse(s), Current.ThumbnailHeight);
                    Current.ThumbnailFormat = thumbNode.GetValue("Format", s =>
                    {
                        switch (s)
                        {
                            case "DXT5":
                                return TextureFormat.DXT5;
                            case "ARGB32":
                            default:
                                return TextureFormat.ARGB32;
                        }
                    }, Current.ThumbnailFormat);
                }

                Current.CompressTextures = cfgNode.GetValue("CompressTextures", s => bool.Parse(s), Current.CompressTextures);

                if (cfgNode.HasNode("Cache"))
                {
                    var cache = cfgNode.GetNode("Cache");
                    foreach (ConfigNode node in cache.nodes)
                    {
                        var el = new CacheItem()
                        {
                            Key = node.name,
                            Url = node.GetValue("Url"),
                            FileSize = long.Parse(node.GetValue("Size")),
                            LastChanged = DateTime.Parse(node.GetValue("Date"))
                        };
                        Current.CachedDataPerKey[el.Key] = el;
                        Current.CachedDataPerResUrl[el.Url] = el;
                    }
                }
            }


            ("Todo19: Consider a mechanism to remove old/unsued cached files! (both dead refs and unused cfgs)").Log();
        }
        void Save()
        {
            var cfg = new ConfigNode("LOD_CONFIG");

            var thumb = cfg.AddNode("Thumbnail");
            thumb.AddValue("Enabled", ThumbnailEnabled.ToString());
            thumb.AddValue("Width", ThumbnailWidth.ToString());
            thumb.AddValue("Height", ThumbnailHeight.ToString());
            thumb.AddValue("Format", ThumbnailFormat.ToString());

            cfg.AddValue("CompressTextures", CompressTextures.ToString());

            var cache = cfg.AddNode("Cache");

            foreach (var el in CachedDataPerKey)
            {
                var node = cache.AddNode(el.Key);
                node.AddValue("Url", el.Value.Url);
                node.AddValue("Size", el.Value.FileSize.ToString());
                node.AddValue("Date", el.Value.LastChanged.ToString());
            }

            cfg.Save(cfgFileLocation.FullName);
            IsDirty = false;
        }

        public bool ThumbnailEnabled { get; private set; }
        public int ThumbnailWidth { get; private set; }
        public int ThumbnailHeight { get; private set; }
        public TextureFormat ThumbnailFormat { get; private set; }

        public bool CompressTextures { get; private set; }

        public bool IsDirty { get; private set; }

        class CacheItem
        {
            public string Url;
            public string Key;
            public long FileSize;
            public DateTime LastChanged;
            public bool isCacheValid(UrlDir.UrlFile file)
            {
                if ((file.fileTime - LastChanged).TotalMinutes > 1)
                {
                    ("Cache miss: " + Url + "; Date [" + file.fileTime.ToString() + "(aka " + file.fileTime.Ticks + ") vs " + LastChanged.ToString() + " (aka " + LastChanged.Ticks + "]").Log();
                }
                else if (new System.IO.FileInfo(file.fullPath).Length != FileSize)
                {
                    ("Cache miss (size): " + Url).Log();
                }
                else
                {
                    return true;
                }
                return false;
            }
            public void updateVerificationData(UrlDir.UrlFile file)
            {
                LastChanged = file.fileTime;
                FileSize = new System.IO.FileInfo(file.fullPath).Length;
            }
            public void clearCache(string directory)
            {
                var di = new System.IO.DirectoryInfo(directory);
                foreach (var oldFile in di.GetFiles(Key + ".*"))
                {
                    ("Deleting old cache file: " + oldFile.FullName).Log();
                    oldFile.Delete();
                }
            }
            public CacheItem() { }
            public CacheItem(string cacheKey, UrlDir.UrlFile fromFile)
            {
                Key = cacheKey;
                Url = fromFile.url;
                updateVerificationData(fromFile);
            }
        }
        Dictionary<string, CacheItem> CachedDataPerKey = new Dictionary<string, CacheItem>();
        Dictionary<string, CacheItem> CachedDataPerResUrl = new Dictionary<string, CacheItem>();

        /// <summary>
        /// Path of the image cache directory, ready to slap a cachefile-string onto it.
        /// </summary>
        /// <returns></returns>
        public string GetCacheDirectory()
        {
            return cfgFileLocation.Directory.FullName;
        }

        void addFileToCache(string key, UrlDir.UrlFile file)
        {
            var item = new CacheItem(key, file);
            item.clearCache(GetCacheDirectory());
            CachedDataPerKey.Add(key, item);
            CachedDataPerResUrl.Add(item.Url, item);
            IsDirty = true;
        }

        /// <summary>
        /// Trys to find an cache file for an image or creates a new one
        /// </summary>
        /// <param name="file">file to create the cache key for</param>
        /// <returns>name (neither path, nor extension) of a unique image file</returns>
        public string GetCacheFileFor(UrlDir.UrlFile file)
        {
            CacheItem cachedItem;
            if (CachedDataPerResUrl.TryGetValue(file.url, out cachedItem))
            {
                if (!cachedItem.isCacheValid(file))
                {
                    cachedItem.clearCache(GetCacheDirectory());
                    cachedItem.updateVerificationData(file);
                    IsDirty = true;
                }
                return cachedItem.Key;
            }
            else
            {
                ("New Cache: " + file.url).Log();
            }
            while (true)
            {
                var cacheKey = RandomString();
                if (!CachedDataPerKey.ContainsKey(cacheKey))
                {
                    addFileToCache(cacheKey, file);
                    return cacheKey;
                }
            }
        }

        static string AllowedChars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        static string RandomString(int len = 16)
        {
            var chars = new char[len];
            for (var i = 0; i < len; i++)
            {
                chars[i] = AllowedChars[UnityEngine.Random.Range(0, AllowedChars.Length)];
            }
            return new string(chars);
        }

        /// <summary>
        /// A bunch of changes might have been applied => consider saving.
        /// </summary>
        public void CommitChanges()
        {
            if (IsDirty)
            {
                Save();
            }
        }
    }
}
