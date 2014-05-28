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
