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
        HashSet<UrlDir.UrlFile> createdTextures = new HashSet<UrlDir.UrlFile>();
        HashSet<string> createdInternals = new HashSet<string>();

        /// <summary>
        /// They broke UrlFile.fileType, so this is a custom replacement... we can only handle a few known file types anyway
        /// </summary>
        /// <returns></returns>
        static bool isFileATexture(UrlDir.UrlFile file)
        {
            // old: return file.fileType == UrlDir.FileType.Texture
            return Managers.TextureManager.IsSupported(file, false);
        }

        IEnumerable<GameDatabase.TextureInfo> FakeTextures(IEnumerable<UrlDir.UrlFile> files)
        {
            foreach (var file in files)
            {
                if (Managers.TextureManager.IsSupported(file))
                {
                    if (Config.Current.GetImageConfig(file).SkipImage)
                    {
                        ("Skipping image due to LOD configuration: " + file.url).Log();
                    }
                    else
                    {
                        /*
                        var cached = System.IO.Path.Combine(Config.Current.GetCacheDirectory(), Config.Current.GetImageConfig(file).CacheKey + ".CACHE");
                        if (System.IO.File.Exists(cached))
                        {
                            using (var ms = System.IO.File.OpenRead(cached))
                            using (var bs = new System.IO.BinaryReader(ms))
                            {
                                ("Faking " + file.fullPath).Log();
                                if (bs.ReadString() != "LODC")
                                {
                                    throw new FormatException("Given file is not a valid CacheTexture (Signature doesn't match)");
                                }
                                var width = bs.ReadInt32();
                                var height = bs.ReadInt32();
                                TextureFormat tf;
                                var fmtInt = bs.ReadInt32();
                                switch (fmtInt)
                                {
                                    case 21:
                                        continue;
                                        tf = TextureFormat.ARGB32;
                                        break;
                                    case 827611204:
                                        tf = TextureFormat.DXT1;
                                        break;
                                    case 894720068:
                                        tf = TextureFormat.DXT5;
                                        break;
                                    default:
                                        throw new FormatException("New or unsupported format: " + fmtInt);
                                }
                                var isNormal = bs.ReadByte() != 0;
                                var dataOffset = (int)ms.Position;


                                #region From CreateEmptyThumbnailTexture
                                var w = Config.Current.DefaultImageSettings.ThumbnailWidth.Value;
                                var h = Config.Current.DefaultImageSettings.ThumbnailHeight.Value;
                                var tex = new Texture2D(w, h, TextureFormat.RGBA32, false);
                                var startsBlack = true;
                                var b = Color.black;
                                var m = Color.magenta;
                                b.a = m.a = 0.5f;
                                for (var x = 0; x < w; x++)
                                {
                                    var isBlack = (startsBlack = !startsBlack);
                                    for (var y = 0; y < h; y++)
                                    {
                                        tex.SetPixel(x, y, isBlack ? Color.black : Color.magenta);
                                        isBlack = !isBlack;
                                    }
                                }
                                tex.Apply(true, false);
                                #endregion

                                var info = new GameDatabase.TextureInfo(tex, isNormal, false, true);
                                info.texture.name = info.name = file.url;
                                GameDatabase.Instance.databaseTexture.Add(info);



                                var ctex = new Texture2D(width, height, tf, false);
                                var data = new byte[ms.Length - dataOffset];
                                ms.Read(data, 0, data.Length);
                                ctex.LoadRawTextureData(data);
                                ctex.Apply(false, true);

                                var ctexPtr = ctex.GetNativeTexturePtr();
                                var texPtr = tex.GetNativeTexturePtr();

                                tex.UpdateExternalTexture(ctexPtr);
                                ctex.UpdateExternalTexture(texPtr);

                                UnityEngine.Object.Destroy(ctex);




                            }
                        }
                        continue;
                         */

                        ("Registering texture: " + file.url + ", Dated: " + file.fileTime).Log();
                        var texInfo = Managers.TextureManager.Setup(file);
                        createdTextures.Add(file);
                        yield return texInfo;
                    }
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
                        Textures = FakeTextures(dir.files.Where(isFileATexture)).ToArray()
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
            //("Debug: Should be GameData: " + current.name + ", " + current.path + ", " + current.url).Log();
            return current;
        }
        static UrlDir.UrlFile GetFileByRelativeSegments(UrlDir gameData, string[] pathSegments, Func<UrlDir.UrlFile, bool> fileVery = null)
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
            var fname = pathSegments[i];
            try
            {
                return (fileVery != null ? current.files.Where(fileVery) : current.files).First(file => file.name == fname);
            }
            catch (Exception err)
            {
                throw new Exception("Could not find a file [" + fname + "] in [" + current.url + " aka " + current.path + "]."
                + Environment.NewLine + "Base: " + gameData.url + ", Path: " + String.Join("/", pathSegments) + ", Files: " + String.Join(";", current.files.Select(file => file.name).ToArray()), err);
            }
            // todo: better error handling or get UrlDir.GetDirectory working?!
        }
        static UrlDir.UrlFile GetFileByRelativePath(UrlDir gameData, string pathSegments, Func<UrlDir.UrlFile, bool> fileVery = null)
        {
            return GetFileByRelativeSegments(gameData, pathSegments.Trim().Split('/'), fileVery);
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

                            var modelTextures = new List<UrlDir.UrlFile>(newModelDir.files.Where(isFileATexture));

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
                                                    modelTextures[i] = GetFileByRelativePath(gameDataDir, replacementData[1], isFileATexture);
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
                    partData.Textures = FakeTextures(dir.files.Where(isFileATexture)).ToArray();
                }
                yield return partData;
            }
        }

        void printPathRecursive(UrlDir dir)
        {
            (dir.url + " / " + dir.path + " / " + dir.type + " / " + dir.root.path).Log();
            foreach (var sub in dir.children)
            {
                printPathRecursive(sub);
            }
        }

        static int didRun = 0;
        public void Awake()
        {
            try
            {
                // Makes LOD ignore GameDatabase's "force reload all", especially since tihs kinda breaks the KspAddon(..., true)
                if (System.Threading.Interlocked.Exchange(ref didRun, 1) != 0)
                {
                    return;
                }
                //printPathRecursive(GameDatabase.Instance.root);

                NativeBridge.Setup(Config.Current.GetCacheDirectory());
#if DISABLED_DEBUG
            foreach(var file in System.IO.Directory.GetFiles(Config.Current.GetCacheDirectory()))
            {
                try
                {
                    System.IO.File.Delete(file);
                }
                catch (Exception) { }
            }
#endif

                Managers.InternalManager.Setup(processAndGetInternals());
                StartupDelayed.PartsToManage = processAndGetParts().ToList();

                // Delay removal from file"system" till all are resolved, so we don't have problems with textures that are used multiple times...
                // we have to remove it, or GameDatabase would reload it...
                foreach (var ft in createdTextures)
                {
                    ft.parent.files.Remove(ft);
                }
                ("LoadOnDemand.Startup done.").Log();


                /*
                 * - The following updates GameDatabase's "last loaded at" to "Now", to prevent files changed before that to be force-reloaded.
                 * - KSP should do that anyway, since there isn't any drawback & any file has to be loaded initially, but this allows us to do it for them.
                 * - ATM hasn't found a way without using reflection as well, and this one is way less invasiv.
                 * - Todo: ForceReload (eg changing file and using Debug-UI's "reload all") will break LOD, since it can't handle refs becoming invalid. Fixable, though and rearely matters anyway.
                 */
                typeof(GameDatabase)
                    .GetFields(System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.NonPublic)
                    .Single(field => field.FieldType == typeof(DateTime))
                    .SetValue(GameDatabase.Instance, DateTime.Now);

                foreach (var el in GameDatabase.Instance.databaseTexture)
                {
                    if (el.texture == null)
                    {
                        ("NULLTEX FOUND: " + el.name).Log();
                    }
                }
                foreach (var imgs in GameDatabase.Instance.root.AllFiles.Where(isFileATexture))
                {
                    ("REM_IMG: " + imgs.fullPath).Log();
                    // Todo: Details about them incl their meta-data & size would be nice...
                }
            }
            catch (Exception err)
            {
                ActivityGUI.SetWarning(err);
                throw;
            }
        }
    }
}
