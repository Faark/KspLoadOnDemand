using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

namespace LoadOnDemand.Legacy
{
    [KSPAddon(KSPAddon.Startup.EveryScene, false)]
    public class LegacyInterface: MonoBehaviour
    {
        public class InternalTexture{
            public string SourceFile;
            public string CacheKey;
            public IntPtr ThumbTexturePtr;
            public bool IsNormal;
        }
        public static Queue<InternalTexture> TexturesToPrepare = new Queue<InternalTexture>();
        public static List<InternalTexture> Textures = new List<InternalTexture>();
        public static string CacheDirectory;


        void Update()
        {
            if(TexturesToPrepare.Count > 0){
                var texToPrep = TexturesToPrepare.Dequeue();



                

            }else{

            }
        }
    }
}
