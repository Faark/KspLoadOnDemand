using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace LoadOnDemand
{
    static class External
    {
        #region Wrappers by called by external DLLs, have to "neutralize" local types
        static void IResource_Release(Object obj)
        {
            if (obj != null)
                ((Resources.IResource)obj).Release();
        }
        static bool IResource_Loaded_Get(Object obj)
        {
            return obj == null ? true : ((Resources.IResource)obj).Loaded;
        }
        static Object PartManager_Get(AvailablePart part, bool withInterior)
        {
            return Managers.PartManager.Get(part, withInterior);
        }
        static IEnumerable<AvailablePart> PartManager_ManagedParts_Get()
        {
            return Managers.PartManager.ManagedParts;
        }
        static IEnumerable<GameDatabase.TextureInfo> PartManager_TexturesOf_Get(AvailablePart part)
        {
            return Managers.PartManager.TexturesOf[part];
        }
        static void PartManager_TexturesOf_Set(AvailablePart part, IEnumerable<GameDatabase.TextureInfo> textures)
        {
            Managers.PartManager.TexturesOf[part] = textures;
        }
        static IEnumerable<string> PartManager_InternalsOf_Get(AvailablePart part)
        {
            return Managers.PartManager.InternalsOf[part];
        }
        static void PartManager_InternalsOf_Set(AvailablePart part, IEnumerable<string> internals)
        {
            Managers.PartManager.InternalsOf[part] = internals;
        }
        static Object InternalManager_Get(string internalName)
        {
            return Managers.InternalManager.Get(internalName);
        }
        static IEnumerable<string> InternalManager_ManagedInternals_Get()
        {
            return Managers.InternalManager.ManagedInternals;
        }
        static IEnumerable<GameDatabase.TextureInfo> InternalManager_TexturesOf_Get(string internalName)
        {
            return Managers.InternalManager.TexturesOf[internalName];
        }
        static void InternalManager_TexturesOf_Set(string internalName, IEnumerable<GameDatabase.TextureInfo> textures)
        {
            Managers.InternalManager.TexturesOf[internalName] = textures;
        }
        static Object TextureManager_Get(GameDatabase.TextureInfo texture)
        {
            return Managers.TextureManager.Get(texture);
        }
        static bool TextureManager_IsManaged(GameDatabase.TextureInfo texture)
        {
            return Managers.TextureManager.IsManaged(texture);
        }
        static GameDatabase.TextureInfo TextureManager_Setup(UrlDir.UrlFile file)
        {
            return Managers.TextureManager.Setup(file);
        }
        static void TextureManager_ForceManage(GameDatabase.TextureInfo texture)
        {
            Managers.TextureManager.ForceManage(texture);
        }
        #endregion

        public static Dictionary<String, Delegate> RequestBindingA(int requested_version)
        {
            if ((requested_version < 1) || (requested_version > 1))
            {
                throw new NotSupportedException("External Bridge Version " + requested_version + " is not supported by this Version of LoadOnDemand.");
            }

            var dict = new Dictionary<string, Delegate>();

            dict["IResource_Release"] = new Action<Object>(IResource_Release);
            dict["IResource_Loaded_Get"] = new Func<Object, bool>(IResource_Loaded_Get);
            dict["PartManager_Get"] = new Func<AvailablePart, bool, Object>(PartManager_Get);
            dict["PartManager_ManagedParts_Get"] = new Func<IEnumerable<AvailablePart>>(PartManager_ManagedParts_Get);
            dict["PartManager_TexturesOf_Get"] = new Func<AvailablePart, IEnumerable<GameDatabase.TextureInfo>>(PartManager_TexturesOf_Get);
            dict["PartManager_TexturesOf_Set"] = new Action<AvailablePart, IEnumerable<GameDatabase.TextureInfo>>(PartManager_TexturesOf_Set);
            dict["PartManager_InternalsOf_Get"] = new Func<AvailablePart, IEnumerable<string>>(PartManager_InternalsOf_Get);
            dict["PartManager_InternalsOf_Set"] = new Action<AvailablePart, IEnumerable<string>>(PartManager_InternalsOf_Set);
            dict["InternalManager_Get"] = new Func<string, Object>(InternalManager_Get);
            dict["InternalManager_ManagedInternals_Get"] = new Func<IEnumerable<string>>(InternalManager_ManagedInternals_Get);
            dict["InternalManager_TexturesOf_Get"] = new Func<string, IEnumerable<GameDatabase.TextureInfo>>(InternalManager_TexturesOf_Get);
            dict["InternalManager_TexturesOf_Set"] = new Action<string, IEnumerable<GameDatabase.TextureInfo>>(InternalManager_TexturesOf_Set);
            dict["TextureManager_Get"] = new Func<GameDatabase.TextureInfo, Object>(TextureManager_Get);
            dict["TextureManager_IsManaged"] = new Func<GameDatabase.TextureInfo, bool>(TextureManager_IsManaged);
            dict["TextureManager_Setup"] = new Func<UrlDir.UrlFile, GameDatabase.TextureInfo>(TextureManager_Setup);
            dict["TextureManager_ForceManage"] = new Action<GameDatabase.TextureInfo>(TextureManager_ForceManage);

            return dict;
        }

    }
}
