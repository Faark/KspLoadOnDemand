using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

namespace LoadOnDemand.Logic
{
    /// <summary>
    /// Pre-Load event... replaces textures with dummies, so they won't actually be loaded (does not just prevent out of memory but also improves load times)
    /// </summary>
    [KSPAddon(KSPAddon.Startup.Instantly, true)]
    public class Startup: MonoBehaviour
    {

        [System.Runtime.InteropServices.DllImport("kernel32", SetLastError = true, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
        static extern IntPtr LoadLibrary(string lpFileName);




        static void LoadNativeDll()
        {
            var tmpDir = Config.Current.GetCacheDirectory();
            if (!System.IO.Directory.CreateDirectory(tmpDir).Exists)
            {
                throw new Exception("Failed to create tmp dir!");
            }

            var path = System.IO.Path.Combine(tmpDir, "NetWrapper.native");
            /*
             * Todo9: Date of the created file seems wrong (hasn't changed for more than 1h, even after i deleted the file it was re-created with the wrong date)... Investiage?!
             * 
             * 
            System.IO.File.Exists(path).ToString().Log();
            System.IO.File.Delete(path);
            System.IO.File.Exists(path).ToString().Log();
            path.Log();
             * 
             */
            var data = System.Reflection.Assembly.GetExecutingAssembly().GetManifestResourceStream("LoadOnDemand.EmbededResources.NetWrapper.dll");
            System.IO.File.WriteAllBytes(path, new System.IO.BinaryReader(data).ReadBytes((int)data.Length));
            IntPtr lib = LoadLibrary(path);
            if (lib == IntPtr.Zero)
                throw new Exception("Faild to load library!");
            else
                ("LoadOnDemand has loaded its native library: " + path).Log();
        }
        static Startup()
        {
            LoadNativeDll();
        }



        HashSet<UrlDir.UrlFile> createdTextures = new HashSet<UrlDir.UrlFile>();

        IEnumerable<GameDatabase.TextureInfo> FakeTextures(IEnumerable<UrlDir.UrlFile> files)
        {
            foreach (var file in files)
            {
                if (Managers.TextureManager.IsSupported(file))
                {
                    ("Registering texture for LoadOnDemand: " + file.url).Log();
                    var tex = Managers.TextureManager.Setup(file);
                    createdTextures.Add(file);
                    yield return tex;
                }
                else
                {
                    ("Failed to register texture for LoadOnDemand (unsuppored format): " + file.url).Log();
                    // Todo21: Evil edge-case on duplicate file name + different extension. Cant currently handle it...
                }
            }
        }
        IEnumerable<Managers.InternalManager.InternalData> processAndGetInternals()
        {
            foreach (var internalCfg in GameDatabase.Instance.GetConfigs("INTERNAL"))
            {
                var dir = internalCfg.parent.parent;
                ("Registering internal for LoadOnDemand: " + internalCfg.name).Log();
                yield return new Managers.InternalManager.InternalData()
                {
                    Name = internalCfg.config.GetValue("name"),
                    Textures = FakeTextures(dir.files.Where(f => f.fileType == UrlDir.FileType.Texture)).ToArray()
                };
            }
        }
        IEnumerable<Managers.PartManager.PartData> processAndGetParts()
        {
            foreach (var part in GameDatabase.Instance.GetConfigs("PART"))
            {
                var dir = part.parent.parent;
                var name = part.config.GetValue("name").Replace('_', '.');
                if (part.config.HasNode("MODEL"))
                {
                    // Todo17: implementing this might be kinda tricky... as far as i can tell, they don't just ref other models but might even copy TextureInfo's.
                    // While hopefully using the same texture2D, so loading is fine, a fake-TextureInfo would fck up the current resource handling...
                    ("Not implemented yet: loading parts with MODEL nodes! Part: " + part.name).Log();
                    yield return new Managers.PartManager.PartData()
                    {
                        Name = name,
                        Internals = new string[0],
                        Textures = new GameDatabase.TextureInfo[0]
                    };
                }
                else
                {
                    ("Registering part for LoadOnDemand: " + part.name).Log();
                    yield return new Managers.PartManager.PartData()
                    {
                        Name = name,
                        Internals = part.config.GetNodes("INTERNAL").Select(node => node.GetValue("name")).ToArray(),
                        Textures = FakeTextures(dir.files.Where(f => f.fileType == UrlDir.FileType.Texture)).ToArray()
                    };
                }
            }
        }
        public void Awake()
        {
            Managers.InternalManager.Setup(processAndGetInternals());
            StartupDelayed.PartsToManage = processAndGetParts().ToList();

            // Delay removal from file"system" till all are resolved, so we don't have problems with textures that are used multiple times...
            // we have to remove it, or GameDatabase would reload it...
            foreach (var ft in createdTextures)
            {
                ft.parent.files.Remove(ft);
            }
            ("LoadOnDemand.Startup done.").Log();

            foreach (var el in GameDatabase.Instance.databaseTexture)
            {
                if (el.texture == null)
                {
                    ("NULLTEX FOUND: " + el.name).Log();
                }
            }
            foreach (var imgs in GameDatabase.Instance.root.AllFiles.Where(f => f.fileType == UrlDir.FileType.Texture))
            {
                ("REM_IMG: " + imgs.fullPath).Log();
            }
        }
    }
}
