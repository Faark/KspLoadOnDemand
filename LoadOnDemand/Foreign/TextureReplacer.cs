using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

namespace LoadOnDemand.Foreign
{
#if false
    // Todo: Remove this, since we most likely release a LOD-compatible TR fork anyway!

    /// <summary>
    /// Disables TextureReplacers compression feature, since this would break this mod. Use ATM instead, if you want to compress textures no managed by LOD.
    /// </summary>
    [KSPAddon(KSPAddon.Startup.Instantly, true)]
    class TextureReplacer: MonoBehaviour
    {
        public void Awake()
        {
            // Todo: It would still break on no TR config (or no TR cfg with isCompressionEnabled)
            // Todo: This would still break if TR is loaded prior of LOD. Might switch to a retor-spec reflection based approach?
            foreach (UrlDir.UrlConfig file in GameDatabase.Instance.GetConfigs("TextureReplacer"))
            {
                if (file.config.HasValue("isCompressionEnabled"))
                {
                    // should not be saved, since afaik TR doesn't save its config...
                    file.config.SetValue("isCompressionEnabled", "never");
                }
                if (file.config.HasValue("isMipmapGenEnabled"))
                {
                    // should not be saved, since afaik TR doesn't save its config...
                    file.config.SetValue("isMipmapGenEnabled", "never");
                }
            }
        }
    }
#endif
}
