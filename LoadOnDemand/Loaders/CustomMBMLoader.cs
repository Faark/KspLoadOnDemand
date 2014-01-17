using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OnDemandLoading.Loaders
{
    /// <summary>
    /// MBM is a piece of shit in nearly any way:
    /// - It appears to be uncompressed pixel data
    /// - The loader produces textures that are not readable (a pain for us)
    /// - The loader isn't async... (even less than the others...)
    /// => We have to create our own loader :(
    /// 
    /// Todo: Error handling?
    /// </summary>
    public class CustomMBMLoader : DatabaseLoader<GameDatabase.TextureInfo>
    {
        public override System.Collections.IEnumerator Load(UrlDir.UrlFile urlFile, System.IO.FileInfo file)
        {
            var www = new UnityEngine.WWW("file://" + file.FullName);
            yield return www;
            if (www.error != null)
            {
                successful = false;
                yield break;
            }
            obj = LoadMBM(www.bytes);
            successful = true;
        }
        public static GameDatabase.TextureInfo LoadMBM(byte[] fileData)
        {

            using (var stream = new System.IO.MemoryStream(fileData))
            using (var reader = new System.IO.BinaryReader(stream))
            {
                reader.ReadString();
                var w = reader.ReadInt32();
                var h = reader.ReadInt32();
                var pixels = w * h;
                var isNormalMap = PartToolsLib.TextureType.NormalMap == (PartToolsLib.TextureType)reader.ReadInt32();
                var bpp = reader.ReadInt32();
                UnityEngine.Texture2D texture;
                var data = new UnityEngine.Color32[pixels];
                if (isNormalMap || (bpp == 32))
                {
                    texture = new UnityEngine.Texture2D(w, h, isNormalMap ? UnityEngine.TextureFormat.ARGB32 : UnityEngine.TextureFormat.RGBA32, true);
                    for (var i = 0; i < pixels; i++)
                    {
                        data[i] = new UnityEngine.Color32(reader.ReadByte(), reader.ReadByte(), reader.ReadByte(), reader.ReadByte());
                    }
                }
                else
                {
                    texture = new UnityEngine.Texture2D(w, h, UnityEngine.TextureFormat.RGB24, true);
                    for (var i = 0; i < pixels; i++)
                    {
                        data[i] = new UnityEngine.Color32(reader.ReadByte(), reader.ReadByte(), reader.ReadByte(), 255);
                    }
                }
                texture.SetPixels32(data);
                texture.Apply(false, false);
                return new GameDatabase.TextureInfo(texture, isNormalMap, true, false);
            }
        }
    }
    /*
     * Is WWW a problem? Could async disc io improve sth?
    public class CustomMBMLoaderAlt : DatabaseLoader<GameDatabase.TextureInfo>
    {
        public override System.Collections.IEnumerator Load(UrlDir.UrlFile urlFile, System.IO.FileInfo file)
        {
            var stream = new System.IO.FileStream(file.FullName, System.IO.FileMode.Open, System.IO.FileAccess.Read, System.IO.FileShare.Read, 10 * 1024, true);
            stream.BeginRead(,0,,,);
            throw new NotImplementedException();
        }
    }*/
}
