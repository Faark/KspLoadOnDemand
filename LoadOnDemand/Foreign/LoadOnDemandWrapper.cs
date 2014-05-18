using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace LoadOnDemand.Foreign
{
    /// <summary>
    /// An external Wrapper that allows other plugins to use LoadOnDemand without having a hard buildin dependency.
    /// </summary>
    public static class LoadOnDemand
    {
        private static class Bindings
        {
            public static Action<Object> IResource_Release = obj => { };
            public static Func<Object, bool> IResource_Loaded_Get = obj => true;
            public static Func<AvailablePart, bool, Object> PartManager_Get = (part, withInterio) => null;
            public static Func<IEnumerable<AvailablePart>> PartManager_ManagedParts_Get = () => Enumerable.Empty<AvailablePart>();
            public static Func<AvailablePart, IEnumerable<GameDatabase.TextureInfo>> PartManager_TexturesOf_Get = part => Enumerable.Empty<GameDatabase.TextureInfo>();
            public static Action<AvailablePart, IEnumerable<GameDatabase.TextureInfo>> PartManager_TexturesOf_Set = (part, textures) => { };
            public static Func<AvailablePart, IEnumerable<string>> PartManager_InternalsOf_Get = part => Enumerable.Empty<string>();
            public static Action<AvailablePart, IEnumerable<string>> PartManager_InternalsOf_Set = (part, internals) => { };
            public static Func<string, Object> InternalManager_Get = internalName => null;
            public static Func<IEnumerable<string>> InternalManager_ManagedInternals_Get = () => Enumerable.Empty<string>();
            public static Func<string, IEnumerable<GameDatabase.TextureInfo>> InternalManager_TexturesOf_Get = internalName => Enumerable.Empty<GameDatabase.TextureInfo>();
            public static Action<string, IEnumerable<GameDatabase.TextureInfo>> InternalManager_TexturesOf_Set = (internalName, textures) => { };
            public static Func<GameDatabase.TextureInfo, Object> TextureManager_Get = texture => null;
            public static Func<GameDatabase.TextureInfo, bool> TextureManager_IsManaged = texture => false;
            public static Func<UrlDir.UrlFile, GameDatabase.TextureInfo> TextureManager_Setup = file => null; // todo: This might have to be optimized, eg. to be LOD independend?!
            public static Action<GameDatabase.TextureInfo> TextureManager_ForceManage = file => { };
        }

        public static class Urls
        {
            public static UrlDir DirectoryFromPath(string path)
            {
                throw new NotImplementedException();
            }
        }

        public class Resource
        {
            Object referencedObject;
            public void Release()
            {
                Bindings.IResource_Release(referencedObject);
                referencedObject = null;
            }
            public bool Loaded
            {
                get
                {
                    return Bindings.IResource_Loaded_Get(referencedObject);
                }
            }

            public override string ToString()
            {
                return (referencedObject ?? this).ToString();
            }
            public Resource(Object non_proxy_reference)
            {
                referencedObject = non_proxy_reference;
            }
        }
        public class Internal
        {
            public string Name { get; private set; }
            public Internal(string internalName)
            {
                Name = internalName;
            }
            public Resource GetReference()
            {
                return new Resource(Bindings.InternalManager_Get(Name));
            }
            public IEnumerable<GameDatabase.TextureInfo> Textures
            {
                get
                {
                    return Bindings.InternalManager_TexturesOf_Get(Name);
                }
                set
                {
                    Bindings.InternalManager_TexturesOf_Set(Name, value);
                }
            }
        }
        public static class Internals
        {
            public static Internal GetInternal(string internalName)
            {
                return new Internal(internalName);
            }
            public static Resource GetReference(string internalName)
            {
                return new Resource(Bindings.InternalManager_Get(internalName));
            }
            public static IEnumerable<Internal> GetManagedInternals()
            {
                return Bindings.InternalManager_ManagedInternals_Get().Select(i => new Internal(i));
            }
        }
        /// <summary>
        /// A wrapper for Parts managed by LOD
        /// </summary>
        public class Part
        {
            public AvailablePart PartType { get; private set; }
            public Part(AvailablePart partType)
            {
                PartType = partType;
            }
            public IEnumerable<Internal> Internals
            {
                get
                {
                    return Bindings.PartManager_InternalsOf_Get(PartType).Select(i => new Internal(i));
                }
                set
                {
                    Bindings.PartManager_InternalsOf_Set(PartType, value.Select(i => i.Name));
                }
            }
            public IEnumerable<GameDatabase.TextureInfo> Textures
            {
                get
                {
                    return Bindings.PartManager_TexturesOf_Get(PartType);
                }
                set
                {
                    Bindings.PartManager_TexturesOf_Set(PartType, value);
                }
            }
            public Resource GetReference(bool withInterior = true)
            {
                return new Resource(Bindings.PartManager_Get(PartType, withInterior));
            }
        }
        public static class Parts
        {
            public static Part GetPart(AvailablePart part)
            {
                return new Part(part);
            }
            public static Resource GetReference(AvailablePart part, bool withInterior = true)
            {
                return new Resource(Bindings.PartManager_Get(part, withInterior));
            }
            public static IEnumerable<Part> GetManagedParts()
            {
                return Bindings.PartManager_ManagedParts_Get().Select(t => new Part(t));
            }
        }
        public static class Textures
        {
            public static Resource GetReference(GameDatabase.TextureInfo texture)
            {
                return new Resource(Bindings.TextureManager_Get(texture));
            }
            public static bool IsManaged(GameDatabase.TextureInfo texture)
            {
                return Bindings.TextureManager_IsManaged(texture);
            }
            public static GameDatabase.TextureInfo Setup(UrlDir.UrlFile file)
            {
                return Bindings.TextureManager_Setup(file);
            }
            public static void ForceManage(GameDatabase.TextureInfo texture)
            {
                Bindings.TextureManager_ForceManage(texture);
            }
        }

        public static bool Available { get; private set; }
        static LoadOnDemand()
        {
            var lodExternalType = System.Type.GetType("LoadOnDemand.External, LoadOnDemand");
            if (lodExternalType != null)
            {
                var bindings = (Dictionary<String, Delegate>)lodExternalType.GetMethod("RequestBindingA").Invoke(null, new object[] { 1 });
                Bindings.IResource_Release = (Action<Object>)bindings["IResource_Release"];
                Bindings.IResource_Loaded_Get = (Func<Object, bool>)bindings["IResource_Loaded_Get"];
                Bindings.PartManager_Get = (Func<AvailablePart, bool, Object>)bindings["PartManager_Get"];
                Bindings.PartManager_ManagedParts_Get = (Func<IEnumerable<AvailablePart>>)bindings["PartManager_ManagedParts_Get"];
                Bindings.PartManager_TexturesOf_Get = (Func<AvailablePart, IEnumerable<GameDatabase.TextureInfo>>)bindings["PartManager_TexturesOf_Get"];
                Bindings.PartManager_TexturesOf_Set = (Action<AvailablePart, IEnumerable<GameDatabase.TextureInfo>>)bindings["PartManager_TexturesOf_Set"];
                Bindings.PartManager_InternalsOf_Get = (Func<AvailablePart, IEnumerable<string>>)bindings["PartManager_InternalsOf_Get"];
                Bindings.PartManager_InternalsOf_Set = (Action<AvailablePart, IEnumerable<string>>)bindings["PartManager_InternalsOf_Set"];
                Bindings.InternalManager_Get = (Func<string, Object>)bindings["InternalManager_Get"];
                Bindings.InternalManager_ManagedInternals_Get = (Func<IEnumerable<string>>)bindings["InternalManager_ManagedInternals_Get"];
                Bindings.InternalManager_TexturesOf_Get = (Func<string, IEnumerable<GameDatabase.TextureInfo>>)bindings["InternalManager_TexturesOf_Get"];
                Bindings.InternalManager_TexturesOf_Set = (Action<string, IEnumerable<GameDatabase.TextureInfo>>)bindings["InternalManager_TexturesOf_Set"];
                Bindings.TextureManager_Get = (Func<GameDatabase.TextureInfo, Object>)bindings["TextureManager_Get"];
                Bindings.TextureManager_IsManaged = (Func<GameDatabase.TextureInfo, bool>)bindings["TextureManager_IsManaged"];
                Bindings.TextureManager_Setup = (Func<UrlDir.UrlFile, GameDatabase.TextureInfo>)bindings["TextureManager_Setup"];
                Bindings.TextureManager_ForceManage = (Action<GameDatabase.TextureInfo>)bindings["TextureManager_ForceManage"]; 
                Available = true;
            }
            else
            {
                Available = false;
            }
        }
    }
}