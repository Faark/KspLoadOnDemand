using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

namespace OnDemandLoading
{
    public class CustomInternalData
    {
        public GameDatabase.TextureInfo[] Textures;
    }
    public class CustomInternalManager : CustomRefCountManagerTemplate<string, CustomInternalData>
    {
        public CustomInternalManager()
            : base(LoD.ReplacedInternalData.ToDictionary(
                el => el.Key,
                el => new CustomInternalData()
                {
                    Textures = el.Value.TextureFiles.Select(tex => GameDatabase.Instance.GetTextureInfo(tex.url)).ToArray(),
                }))
        {
        }


        protected override void StartLoadObject(String key, CustomInternalData data)
        {
            var missingTextures = new HashSet<GameDatabase.TextureInfo>();
            foreach (var tex in data.Textures)
            {
                if (!CustomTextureManager.IsLoaded(tex))
                {
                    missingTextures.Add(tex);
                    CustomTextureManager.RequestLoad(tex, () =>
                    {
                        missingTextures.Remove(tex);
                        if (missingTextures.Count <= 0)
                        {
                            Debug.Log("Intern done");
                            AnnounceLoadFinished(key);
                        }
                    });
                }
            }
            Debug.Log("Intern started with " + missingTextures.Count.ToString());
            if (missingTextures.Count <= 0)
            {
                AnnounceLoadFinished(key);
            }
        }
        protected override void UnloadObject(String key, CustomInternalData data)
        {
            foreach (var tex in data.Textures)
            {
                CustomTextureManager.RequestUnload(tex);
            }
        }

    }
}
