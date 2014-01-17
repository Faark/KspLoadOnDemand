using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

namespace OnDemandLoading
{


    /*
     * 
     * Concept/Thoughs:
     * 
     * - i have to modify models in GameDatabase, since otherwise they would be loaded anyway
     * - the most promising way to do so seems like the URL system... if we can remove them there, they would never appear
     * - somehow injecte a dummy into gamedatabase... that might be problematic as well
     * 
     * 
     * - TODO: when skipping loads, we have to make sure to release the correct els...
     * - Editor does not really need internals...
     * 
     * Problem:
     * a. I have to remove both model and texture. I can't say for sure what texture is specific to a model without "parsing" it,
     *    so atm i have to skip all textures in a part directory and hope the mod/part does respect the convention.
     *    
     * 
     * Database is a pcies of S. Whenever a texture is loaded, it wipes and reloads all models. That makes it impossible to pre-inject a custom model without 
     * re-wiring it to a different model, what seems kinda problematic/impossible. It seems especially impossible atm to re-use the same mdl for all of them.
     */
    public class DirectoryResources
    {
        public UrlDir.UrlFile[] ModelFiles;
        public UrlDir.UrlFile[] TextureFiles;
        public UrlDir.UrlFile[] AudioFiles;

    }
    public class OriginalPartData : DirectoryResources
    {
        public string[] Internals;

        //public ConfigNode[] ModelNodes;
        //public GameObject PrefabObject;
    }
    [KSPAddon(KSPAddon.Startup.MainMenu, false)]
    public class UI : UnityEngine.MonoBehaviour
    {
        public static string getData()
        {
            return GameDatabase.Instance.databaseModel.Any() ? GameDatabase.Instance.databaseModel.Select(el => el.name).Aggregate((a, b) => a + Environment.NewLine + b) : "EMPTY!";
        }
        public static string data2;
        public static string data;
        Vector2 scroll;
        Vector2 scroll2;
        public void OnGUI()
        {
            var w = 500;
            var h = 400;
            var l = (UnityEngine.Screen.width - w) / 2;
            var t = 10;
            //var t = (UnityEngine.Screen.height - h - 192);
            GUILayout.BeginArea(new UnityEngine.Rect(l, t, w, h));
            if (GUILayout.Button("Print debug."))
            {
                data = getData();
            }
            GUILayout.BeginHorizontal();
            scroll = GUILayout.BeginScrollView(scroll);
            GUILayout.Label(data);
            GUILayout.EndScrollView();
            scroll2 = GUILayout.BeginScrollView(scroll2);
            GUILayout.Label(data2);
            GUILayout.EndScrollView();
            GUILayout.EndHorizontal();
            GUILayout.EndArea();
        }
    }
    [KSPAddon(KSPAddon.Startup.Instantly, true)]
    public class LoD: UnityEngine.MonoBehaviour
    {
        public static Dictionary<string, DirectoryResources> ReplacedInternalData = new Dictionary<string, DirectoryResources>();
        public static Dictionary<string, OriginalPartData> ReplacedPartData = new Dictionary<string,OriginalPartData>();

        void SetupFakeTexture(UrlDir.UrlFile file)
        {
            var tex = new Texture2D(1, 1, TextureFormat.RGBA32, true);
            tex.SetPixel(0, 0, Color.cyan);
            tex.Apply(true, false);
            var ti = new GameDatabase.TextureInfo(tex, false, true, false);
            ti.isNormalMap = true; //jop, its a lie... but necessary, since converted normalmaps are not editable anymore.
            ti.name = file.url;
            GameDatabase.Instance.databaseTexture.Add(ti);
        }
        void Startup_DoInternals()
        {
            foreach (var internalCfg in GameDatabase.Instance.GetConfigs("INTERNAL"))
            {
                var dir = internalCfg.parent.parent;
                var data = ReplacedInternalData[internalCfg.config.GetValue("name")] = new DirectoryResources()
                {
                    ModelFiles = dir.files.Where(f => f.fileType == UrlDir.FileType.Model).ToArray(),
                    TextureFiles = dir.files.Where(f => f.fileType == UrlDir.FileType.Texture).ToArray(),
                    AudioFiles = dir.files.Where(f => f.fileType == UrlDir.FileType.Audio).ToArray(),
                };
                foreach (var textureFile in dir.files.Where(f => f.fileType == UrlDir.FileType.Texture).ToList())
                {
                    SetupFakeTexture(textureFile);
                    dir.files.Remove(textureFile);
                }
            }
        }
        void Startup_DoParts()
        {
            foreach (var part in GameDatabase.Instance.GetConfigs("PART"))
            {
                if (part.config.HasNode("MODEL"))
                {
                    // we can't garantee that those models/textures are used only once... so lets skip them for now (yes, its actually even more wrong, but who cares for a prove of concept)
                    continue;
                }

                var dir = part.parent.parent;
                var data = ReplacedPartData[part.config.GetValue("name").Replace('_', '.')] = new OriginalPartData()
                {
                    ModelFiles = dir.files.Where(f => f.fileType == UrlDir.FileType.Model).ToArray(),
                    TextureFiles = dir.files.Where(f => f.fileType == UrlDir.FileType.Texture).ToArray(),
                    AudioFiles = dir.files.Where(f => f.fileType == UrlDir.FileType.Audio).ToArray(),

                    Internals = part.config.GetNodes("INTERNAL").Select(node => node.GetValue("name")).ToArray()
                };

                // We could distinguish between old and new format, here. but since we don't have the tools to do in-depth-stuff, lets just stick to removing all res from the DB instead.

                foreach (var textureFile in dir.files.Where(f => f.fileType == UrlDir.FileType.Texture).ToList())
                {
                    SetupFakeTexture(textureFile);
                    dir.files.Remove(textureFile);
                }

                //dir.files.RemoveAll(f => /*f.fileType == UrlDir.FileType.Audio || */f.fileType == UrlDir.FileType.Texture /*|| f.fileType == UrlDir.FileType.Model*/);


                /*
                if (el.config.HasNode("MODEL")) 
                {
                    //throw new NotImplementedException();
                }
                else
                {
                    Debug.Log("Removing model from " + el.parent.parent.url);
                    Debug.Log("Content:");
                    foreach (var f in el.parent.parent.files)
                    {
                        Debug.Log(f.url + " / " + f.fullPath + " (" + f.fileType.ToString() + ")");
                    }
                    //el.parent.parent.files.Remove()
                }*/
                /*
                if (part.config.HasNode("MODEL"))
                {
                    throw new NotImplementedException("Complicated model definitons allow tricky stuff and are not yet supported by this mod.");
                }
                else
                {
                    //this is tricky... we have to inject a link to our own model thats found via ModelIn... guess we just add a model to the db for now
                    var newObj = (GameObject)GameObject.Instantiate(GetReplacementModel());
                    newObj.name = dir.url + "/model.mu";
                    GameDatabase.Instance.databaseModel.Add(newObj);
                    Debug.Log("Fake model added to database: " + newObj.name);
                    Debug.Log("Retreiving fake model: " + (GameDatabase.Instance.GetModelIn(part.parent.parent.url) ?? new GameObject("FAILED!")).name);
                }*/
            }
        }
        public void Awake()
        {
            InFlightVesselManager.defaultLoadDistance = Vessel.loadDistance;
            InFlightVesselManager.defaultUnloadDistance = Vessel.unloadDistance;
            Vessel.unloadDistance = 0;
            Vessel.loadDistance = Single.MaxValue;

            Startup_DoInternals();
            Startup_DoParts();

            //var props = GameDatabase.Instance.GetConfigs("PROP");
            

            UI.data2 = UI.getData();

        }
    }
}
