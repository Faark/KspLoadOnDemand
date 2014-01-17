using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

namespace OnDemandLoading
{
    public class CustomPartData{
        public GameDatabase.TextureInfo[] Textures;
        public string[] Internals;
    }
    public class CustomPartManager: CustomRefCountManagerTemplate<AvailablePart, CustomPartData>
    {
        public CustomPartManager(): base(LoD.ReplacedPartData.ToDictionary(
            el => PartLoader.LoadedPartsList.Single(ap => ap.name == el.Key),
            el => new CustomPartData()
            {
                Textures = el.Value.TextureFiles.Select(tex => GameDatabase.Instance.GetTextureInfo(tex.url)).ToArray(),
                Internals = el.Value.Internals
            }))
        {
        }


        protected override void StartLoadObject(AvailablePart key, CustomPartData data)
        {
            Debug.Log("Started to load " + key.name + " with " + data.Textures.Count() + " textures and " + data.Internals.Count() + " internals.");
            var loadingState = new
            {
                missingTextures = new HashSet<GameDatabase.TextureInfo>(),
                missingInternals = new HashSet<string>(),
            };
            Action checkDone = () =>
            {
                if (loadingState.missingInternals.Count + loadingState.missingTextures.Count <= 0)
                    AnnounceLoadFinished(key);
            };
            foreach (var tex in data.Textures)
            {
                if (!CustomTextureManager.IsLoaded(tex))
                {
                    loadingState.missingTextures.Add(tex);
                    CustomTextureManager.RequestLoad(tex, () =>
                    {
                        loadingState.missingTextures.Remove(tex);
                        checkDone();
                    });
                }
            }
            foreach (var intern in data.Internals)
            {
                if (!CustomManagers.Internals.IsLoaded(intern))
                {
                    Debug.Log("Requesting Internal: " + intern);
                    loadingState.missingInternals.Add(intern);
                    CustomManagers.Internals.IncrementUsage(intern, () =>
                    {
                        Debug.Log("Internal loaded: " + intern);
                        loadingState.missingInternals.Remove(intern);
                        checkDone();
                    });
                }
                else
                {
                    Debug.Log("Internal already loaded: " + intern);
                }
            }
            checkDone();
        }
        protected override void UnloadObject(AvailablePart key, CustomPartData data)
        {
            foreach (var tex in data.Textures)
            {
                CustomTextureManager.RequestUnload(tex);
            }
            foreach (var i in data.Internals)
            {
                CustomManagers.Internals.DecrementUsage(i);
            }
        }

    }
}
