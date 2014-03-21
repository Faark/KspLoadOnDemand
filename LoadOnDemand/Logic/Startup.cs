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
                throw new Exception("LoadOnDemand failed loaded its native library:" + path);
            else
                ("LoadOnDemand has loaded its native library: "+ path).Log();
        }
        static Startup()
        {
            LoadNativeDll();
        }



        HashSet<UrlDir.UrlFile> createdTextures = new HashSet<UrlDir.UrlFile>();
        HashSet<string> createdInternals = new HashSet<string>();

        IEnumerable<GameDatabase.TextureInfo> FakeTextures(IEnumerable<UrlDir.UrlFile> files)
        {
            foreach (var file in files)
            {
                if (Managers.TextureManager.IsSupported(file))
                {
                    ("Registering texture: " + file.url).Log();
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
                if (createdInternals.Add(internalCfg.name))
                {
                    var dir = internalCfg.parent.parent;
                    ("Registering internal [" + internalCfg.name + "] from [" + internalCfg.url + "].").Log();
                    yield return new Managers.InternalManager.InternalData()
                    {
                        Name = internalCfg.config.GetValue("name"),
                        Textures = FakeTextures(dir.files.Where(f => f.fileType == UrlDir.FileType.Texture)).ToArray()
                    };
                }
            }
        }

        static UrlDir GetGameData(UrlDir.UrlFile file)
        {
            var current = file.parent;
            while ((current != null) && (current.parent != current.root))
            {
                current = current.parent;
                if (current == null)
                {
                    throw new Exception("Looks like we missed it?!");
                }
            }
            ("Debug: Should be GameData: " + current.name + ", " + current.path + ", " + current.url).Log();
            return current;
        }
        static UrlDir.UrlFile GetFileByRelativeSegments(UrlDir gameData, string[] pathSegments, UrlDir.FileType? fileType)
        {
            var current = gameData;
            int i = 0;
            for (; i < (pathSegments.Length - 1); i++)
            {
                var segment = pathSegments[i];
                try
                {
                    current = current.children.Single(el => el.name == segment);
                }
                catch (Exception err)
                {
                    throw new Exception("Could not find a directory [" + segment + "] in [" + current.url + " aka " + current.path + "]."
                    + Environment.NewLine + "Base: " + gameData.path + ", Path: " + String.Join("/", pathSegments) + ", Dirs: " + String.Join(";", current.children.Select(dir => dir.name).ToArray()), err);
                }
            }
            ("FINAL DIR: " + current.name + ", " + current.path).Log();
            var fname = pathSegments[i];
            try
            {
                return current.files.First(file => file.name == fname && (!fileType.HasValue || file.fileType == fileType));
            }
            catch (Exception err)
            {
                throw new Exception("Could not find a file [" + fname + "] in [" + current.url + " aka " + current.path + "]."
                + Environment.NewLine + "Base: " + gameData.url + ", Path: " + String.Join("/", pathSegments) + ", Files: " + String.Join(";", current.files.Select(file => file.name).ToArray()), err);
            }
            // todo: better error handling or get UrlDir.GetDirectory working?!
        }
        static UrlDir.UrlFile GetFileByRelativePath(UrlDir gameData, string pathSegments, UrlDir.FileType? fileType = null)
        {
            return GetFileByRelativeSegments(gameData, pathSegments.Trim().Split('/'), fileType);
        }
        IEnumerable<Managers.PartManager.PartData> processAndGetParts()
        {
            foreach (var part in GameDatabase.Instance.GetConfigs("PART"))
            {
                var dir = part.parent.parent;

                ("Registering part [" + part.name + "] from [" + part.url + "].").Log();

                var partData = new Managers.PartManager.PartData()
                {
                    Name = part.config.GetValue("name").Replace('_', '.'),
                    Internals = part.config.GetNodes("INTERNAL").Select(node => node.GetValue("name")).ToArray()
                };

                if (part.config.HasNode("MODEL"))
                {

                    var gameDataDir = GetGameData(part.parent);

                    var used_textures = new HashSet<UrlDir.UrlFile>();//dir.files.Where(f => f.fileType == UrlDir.FileType.Texture));

                    foreach (var modelNode in part.config.GetNodes("MODEL"))
                    {
                        try
                        {
                            // Part1: Lets find the UrlDir for the model referenced in this MODEL block
                            var newModelFile = GetFileByRelativePath(gameDataDir, modelNode.GetValue("model"));
                            var newModelDir = newModelFile.parent;

                            var modelTextures = new List<UrlDir.UrlFile>(newModelDir.files.Where(f => f.fileType == UrlDir.FileType.Texture));

                            foreach (var textureReplacement in modelNode.GetValues("texture"))
                            {
                                try
                                {

                                    var replacementData = textureReplacement.Split(',');
                                    if (replacementData.Length == 2)
                                    {
                                        var filenameToReplace = replacementData[0].Trim(); // todo: Cut .extension?
                                        for (var i = 0; i < modelTextures.Count; i++)
                                        {
                                            ("trycomp: " + modelTextures[i].name + " vs " + filenameToReplace).Log();
                                            if (modelTextures[i].name == filenameToReplace)
                                            {
                                                if (replacementData[1].Contains("/"))
                                                {
                                                    modelTextures[i] = GetFileByRelativePath(gameDataDir, replacementData[1], UrlDir.FileType.Texture);
                                                }
                                                else
                                                {
                                                    ("NotImplementedException / Todo: Replacement in same dir is not yet supported. Kethane/Parts/kethane_generator/part/kethane_generator does it, but kinda non-sensical.").Log();
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        ("WARNING: Texture nodes seems wrong [" + replacementData.Length + ", " + textureReplacement + "] for part: " + part.url).Log();
                                    }
                                }
                                catch (Exception err)
                                {
                                    err.Log("WARNING: Invalid TextureReplacement!");
                                }
                            }

                            foreach (var current in modelTextures)
                            {
                                used_textures.Add(current);
                            }
                        }
                        catch (Exception err)
                        {
                            err.Log("WARNING: Invalid Model-Node!");
                        }
                    }


                    partData.Textures = FakeTextures(used_textures).ToArray();
                }
                else
                {
                    partData.Textures = FakeTextures(dir.files.Where(f => f.fileType == UrlDir.FileType.Texture)).ToArray();
                }
                yield return partData;
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
