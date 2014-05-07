using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace LoadOnDemand.Managers
{
    public static class InternalManager
    {
        public class InternalData
        {
            public string Name;
            public GameDatabase.TextureInfo[] Textures;
        }
        internal static Dictionary<string, InternalData> iManagedInternals = new Dictionary<string, InternalData>();
        public static IEnumerable<string> ManagedInternals
        {
            get
            {
                return iManagedInternals.Keys;
            }
        }

        public static Resources.IResource Get(string name)
        {
            InternalData data;
            if (iManagedInternals.TryGetValue(name, out data))
            {
                return new Resources.ResourceCollection(data.Textures.Select(tex => TextureManager.GetOrThrow(tex)));
            }
            else
            {
                return null;
            }
        }
        public static Resources.IResource GetOrThrow(string name)
        {
            var res = Get(name);
            if (res == null)
            {
                throw new ArgumentException("Could not find a managed internal called [" + name + "]");
            }
            return res;
        }
        public static void Setup(IEnumerable<InternalData> data)
        {
            foreach (var id in data)
            {
                iManagedInternals.Add(id.Name, id);
            }
        }


        public class TexturesOfObj
        {
            public IEnumerable<GameDatabase.TextureInfo> this[string internalName]
            {
                get
                {
                    return iManagedInternals[internalName].Textures;
                }
                set
                {
                    // Todo: validation
                    iManagedInternals[internalName].Textures = value.ToArray();
                }
            }
        }
        public static readonly TexturesOfObj TexturesOf = new TexturesOfObj();
    }
}
