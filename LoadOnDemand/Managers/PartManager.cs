using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace LoadOnDemand.Managers
{
    public static class PartManager
    {
        public class PartData
        {
            public string Name;
            public AvailablePart Part;
            public GameDatabase.TextureInfo[] Textures;
            public string[] Internals;
        }
        internal static Dictionary</*AvailablePart*/ string, PartData> iManagedParts = new Dictionary<string, PartData>();
        /// <summary>
        /// A collection of all parts currently known to PartManager.
        /// </summary>
        public static IEnumerable<AvailablePart> ManagedParts
        {
            get
            {
                //return iManagedParts.Keys;
                return iManagedParts.Values.Select(el => el.Part);
            }
        }
        /// <summary>
        /// Get resource incl subs or null if unknown
        /// </summary>
        public static Resources.IResource Get(AvailablePart part, bool withInterior)
        {
            PartData data;
            if (iManagedParts.TryGetValue(part.name, out data))
            {
                if (withInterior)
                {
                    return new Resources.ResourceCollection(
                        data.Textures.Select(tex => TextureManager.GetOrThrow(tex)).Union(
                            data.Internals.Select(intern => InternalManager.GetOrThrow(intern))
                            ));
                }
                else
                {
                    return new Resources.ResourceCollection(data.Textures.Select(tex => TextureManager.GetOrThrow(tex)));
                }
            }
            else
            {
                return null;
            }
        }
        /// <summary>
        /// Throws on not-found...
        /// </summary>
        public static Resources.IResource GetOrThrow(AvailablePart part, bool withInterior = true)
        {
            var res = Get(part, withInterior);
            if (res == null)
            {
                var errTxt = "Part [" + part.name + "] is not resource-managed. Currently managed parts: ["
                    + (iManagedParts.Any() ? String.Join(", ", (iManagedParts.Select(p => p.Value.Part.name).ToArray())) : "NONE!!") + "], PartLoader: ["
                    + (PartLoader.LoadedPartsList.Any() ? String.Join(", ", PartLoader.LoadedPartsList.Select(el => el.name).ToArray()) : "NONE") + "]";
                errTxt.Log();
                throw new Exception(errTxt);
            }
            return res;
        }
        /// <summary>
        /// Always returns a valid handle, a dummy in case part is unknown.
        /// </summary>
        public static Resources.IResource GetSafe(AvailablePart part, bool withInterior = true)
        {
            return Get(part, withInterior) ?? new Resources.ResourceDummy();
        }
        public static void Setup(PartData part)
        {
            iManagedParts[part.Part.name] = part;
        }
        /*public static void Setup(IEnumerable<PartData> data)
        {
            var cnt = 0;
            foreach (var part in data)
            {
                iManagedParts[part.Part] = part;
                cnt++;
            }
        }*/
    }
}
