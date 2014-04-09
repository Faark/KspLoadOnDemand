using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

namespace LoadOnDemand.Logic
{
    
    /// <summary>
    /// Post load event... 
    /// </summary>
    [KSPAddon(KSPAddon.Startup.MainMenu, true)]
    public class StartupDelayed: MonoBehaviour
    {
        public static List<Managers.PartManager.PartData> PartsToManage;

        public void Awake()
        {
            if (Config.Disabled)
            {
                return;
            }
            // Finds loaded instances and remove all models that were not properly loaded on the fly...
            foreach (var partData in PartsToManage)
            {
                partData.Part = PartLoader.getPartInfoByName(partData.Name);
                if (partData.Part == null)
                {
                    ("PartSkipped, was probably not sucessfully loaded:" + partData.Name).Log();
                }
                else
                {
                    Managers.PartManager.Setup(partData);
                }
            }
            Managers.TextureManager.FinalizeSetup();
            Config.Current.CommitChanges();
            PartsToManage = null;
            ("LoadOnDemand.StartupDelayed done.").Log();
#if true
            // Todo: Lets dumpe this always, for now...
            ("Dumping collected informations! ").Log();
            foreach (var i in Managers.InternalManager.iManagedInternals)
            {
                ("Internal: " + i.Key).Log();
                foreach (var t in i.Value.Textures)
                {
                    ("  - " + t.name).Log();
                }
            }
            foreach (var p in Managers.PartManager.iManagedParts)
            {
                ("Part: " + p.Key + ", " + p.Value.Part.partPath).Log();
                ("  - Internals: " + String.Join(", ", p.Value.Internals)).Log();
                foreach (var t in p.Value.Textures)
                {
                    ("  - " + t.name).Log();
                }
            }
            "Dumping missed texture informations.".Log();
            Func<int, TextureFormat, String> bpp = (pixels, fmt) =>
            {
                Func<int, String> toSizeString = bytes => (bytes / (1024f * 1024f)) + " MB";
                switch (fmt)
                {
                    case TextureFormat.RGB24:
                    case TextureFormat.RGBA32:
                    case TextureFormat.ARGB32:
                        return toSizeString(pixels * 4);
                    case TextureFormat.DXT1:
                        return toSizeString(pixels / 2);
                    case TextureFormat.DXT5:
                        return toSizeString(pixels);
                    default:
                        return "N/A";
                }
            };
            foreach (var imgs in GameDatabase.Instance.root.AllFiles.Where(f => f.fileType == UrlDir.FileType.Texture))
            {
                try
                {
                    var tex = GameDatabase.Instance.GetTexture(imgs.url, false);
                    ("MISSED TEXTURE: " + imgs.fullPath + " (" + tex.width + "x" + tex.height + "@" + tex.format.ToString() + "=> " + bpp(tex.width * tex.height, tex.format) + ")").Log();
                }
                catch (Exception) { }
            }

            /*
            foreach (var el in UnityEngine.Resources.FindObjectsOfTypeAll(typeof(UnityEngine.Object)))
            {
                ("RES " + el.GetType().FullName + " aka " + el.name).Log();
            }*/


#endif
        }
    }
}
