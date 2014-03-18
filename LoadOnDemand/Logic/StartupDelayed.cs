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
        }
    }
}
