using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

namespace LoadOnDemand.Legacy
{
    abstract class LegacyTextureLoaderBase
    {
        public abstract bool Ready { get; }
        public abstract Texture2D GetTexture();
    }
    public static class LegacyLoader
    {
        public static Texture2D LoadImage(System.IO.FileInfo file, ref bool isNormal, bool shouldBeNormal)
        {
            var ext = file.Extension.ToUpper();
            var withoutExt = file.FullName.Substring(0, file.FullName.Length - ext.Length).ToUpper();
            shouldBeNormal = shouldBeNormal || withoutExt.EndsWith("NRM");
            Texture2D tex = null;
            isNormal = false;
            switch (ext)
            {
                case ".TGA":
                    var ti = new TGAImage();
                    if (ti.ReadImage(file))
                    {
                        tex = ti.CreateTexture();
                    }
                    else
                    {
                        return null;
                    }
                    break;
                case ".MBM":
                    tex = MBMReader.Read(file.FullName, out isNormal);
                    break;
                default:
                    var www = new WWW("file://" + file.FullName);
                    while (!www.isDone)
                    {
                        System.Threading.Thread.Sleep(100);
                    }
                    tex = www.texture;
                    break;
            }
            if (shouldBeNormal && !isNormal)
            {
                isNormal = true;
                return GameDatabase.BitmapToUnityNormalMap(tex);
            }
            else
            {
                return tex;
            }
        }
    }
    /*
    class UnityLoader: LegacyTextureLoaderBase {
        WWW www;
        bool normal;
        public override bool Ready
        {
            get { return www.isDone; }
        }
        public override Texture2D GetTexture()
        {
            while (!Ready)
            {
                System.Threading.Thread.Sleep(100);
            }
            return normal ? GameDatabase.BitmapToUnityNormalMap(www.texture) : GameDatabase. www.LoadImageIntoTexture
            throw new NotImplementedException();
        }
        public UnityLoader(System.IO.FileInfo fi, bool isNormal)
        {
            www = new WWW("file://" + fi.FullName);
            normal = isNormal || fi.Name;
        }
    }*/
}
