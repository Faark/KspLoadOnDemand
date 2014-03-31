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
#if DEBUG
            ("Dumping collected informations! ").Log();
            foreach(var i in Managers.InternalManager.iManagedInternals){
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
            foreach (var imgs in GameDatabase.Instance.root.AllFiles.Where(f => f.fileType == UrlDir.FileType.Texture))
            {
                try
                {
                    var tex = GameDatabase.Instance.GetTexture(imgs.url, false);
                    ("MISSED TEXTURE: " + imgs.fullPath + " (" + tex.width + "x" + tex.height + "@" + tex.format.ToString() + ")").Log();
                }
                catch (Exception) { }
            }
#endif
        }
    }
}
