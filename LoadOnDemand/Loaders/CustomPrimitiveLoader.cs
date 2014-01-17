using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OnDemandLoading.Loaders
{
    /*
     * PartLoader isn't very mod-friendly, so we have to re-do a bunch of work...
     */
    public class CustomPrimitiveLoader
    {
        /// <summary>
        /// A simple wrapper around unity coroutines, so we can check for "done" and get the "return value"
        /// 
        /// TODO: Error handling?
        /// </summary>
        /// <typeparam name="T"></typeparam>
        public class AsyncState<T> where T : class
        {
            public T Data { get; private set; }
            public bool IsDone { get; private set; }
            System.Collections.IEnumerator loader_sub_coroutine;
            DatabaseLoader<T> dbLaoder;
            UnityEngine.MonoBehaviour mbHost;
            public AsyncState(UnityEngine.MonoBehaviour host, DatabaseLoader<T> loader, System.Collections.IEnumerator loader_coroutine)
            {
                Data = default(T);
                IsDone = false;
                dbLaoder = loader;
                mbHost = host;
                loader_sub_coroutine = loader_coroutine;
                host.StartCoroutine(awaiting_coroutine());
            }
            System.Collections.IEnumerator awaiting_coroutine()
            {
                yield return mbHost.StartCoroutine(loader_sub_coroutine);
                Data = dbLaoder.obj;
                IsDone = true;
                loader_sub_coroutine = null;
                dbLaoder = null;
                mbHost = null;
            }
        }
        public static Dictionary<string, DatabaseLoader<T>> GetLoadersFor<T>()
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
        public static Dictionary<string, DatabaseLoader<GameDatabase.TextureInfo>> TextureLoaders;
        public static Dictionary<string, DatabaseLoader<UnityEngine.AudioClip>> AudioLoaders;
        public static Dictionary<string, DatabaseLoader<UnityEngine.GameObject>> ModelLoaders;
        static CustomPrimitiveLoader()
        {
            // Todo: Problem... this can not find stuff on assemblies later loaded... though this is only a problem when supporting loaders from mods.
            TextureLoaders = GetLoadersFor<GameDatabase.TextureInfo>();
            TextureLoaders["MBM"] = new CustomMBMLoader(); // replace default MBM loader with this improved one
            AudioLoaders = GetLoadersFor<UnityEngine.AudioClip>();
            ModelLoaders = GetLoadersFor<UnityEngine.GameObject>();
        }
        static AsyncState<T> LoadByExtension<T>(Dictionary<string, DatabaseLoader<T>> loaders, UnityEngine.MonoBehaviour host, UrlDir.UrlFile url, System.IO.FileInfo info)
            where T: class
        {
            var loader = loaders[url.fileExtension.ToUpper()];
            return new AsyncState<T>(host, loader, loader.Load(url, info));
        }
        public static AsyncState<GameDatabase.TextureInfo> LoadTexture(UnityEngine.MonoBehaviour host, UrlDir.UrlFile url, System.IO.FileInfo info)
        {
            return LoadByExtension(TextureLoaders, host, url, info);
        }
        public static AsyncState<UnityEngine.AudioClip> LoadAudio(UnityEngine.MonoBehaviour host, UrlDir.UrlFile url, System.IO.FileInfo info)
        {
            return LoadByExtension(AudioLoaders, host, url, info);
        }
        public static AsyncState<UnityEngine.GameObject> LoadModel(UnityEngine.MonoBehaviour host, UrlDir.UrlFile url, System.IO.FileInfo info)
        {
            return LoadByExtension(ModelLoaders, host, url, info);
        }
    }
}
