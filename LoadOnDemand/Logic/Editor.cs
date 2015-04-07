using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

namespace LoadOnDemand.Logic
{
    /// <summary>
    /// Todo1: find a way to reference all parts currently used on the rocked on at least laying around somewhere...
    /// </summary>
    [KSPAddon(KSPAddon.Startup.EditorAny, false)]
    class EditorUsedParts : MonoBehaviour
    {
        Dictionary<AvailablePart, Resources.IResource> referencedResources = new Dictionary<AvailablePart, Resources.IResource>();

        void RefAllFrom(Part startPart, HashSet<AvailablePart> usedParts)
        {
            usedParts.Add(startPart.partInfo);
            foreach (var sub in startPart.children)
            {
                RefAllFrom(sub, usedParts);
            }
        }
        void Update()
        {
            if (EditorLogic.fetch != null)
            {
                HashSet<AvailablePart> usedParts = new HashSet<AvailablePart>();
                if (EditorLogic.SelectedPart != null)
                {
                    RefAllFrom(EditorLogic.SelectedPart, usedParts);
                }
                if (EditorLogic.RootPart != null)
                {
                    RefAllFrom(EditorLogic.RootPart, usedParts);
                }
                HashSet<AvailablePart> unusedParts = new HashSet<AvailablePart>(referencedResources.Keys);
                foreach (var part in usedParts)
                {
                    if (!unusedParts.Remove(part))
                    {
                        ("EditorUsedParts is referencing part: " + part.name).Log();
                        referencedResources[part] = Managers.PartManager.GetSafe(part, false);
                    }
                }
                foreach (var part in unusedParts)
                {
                    ("EditorUsedParts is releasing part: " + part.name).Log();
                    referencedResources[part].Release();
                    referencedResources.Remove(part);
                }
            }
        }
        void Awake()
        {
            if (Config.Disabled)
            {
                enabled = false;
            }
        }
        void OnDestroy()
        {
            foreach (var el in referencedResources)
            {
                ("EditorUsedParts[End] is releasing part: " + el.Key.name).Log();
                el.Value.Release();
            }
            referencedResources = null;
        }
    }

    [KSPAddon(KSPAddon.Startup.EditorAny, false)]
    class EditorPages : MonoBehaviour
    {
        Dictionary<AvailablePart, Resources.IResource> referencedResources = new Dictionary<AvailablePart, Resources.IResource>();
        HashSet<AvailablePart> PartQueueToProcess = new HashSet<AvailablePart>();
        bool addPartsHasRunOnce = false;
        EditorPartListFilter<AvailablePart> myPseudoFilter;

        bool AddPartToList(AvailablePart part)
        {
            PartQueueToProcess.Add(part);
            addPartsHasRunOnce = true;
            return true;
        }
        void SetupNew()
        {
            myPseudoFilter = new EditorPartListFilter<AvailablePart>("LodPseudoFilter", AddPartToList);
            var list = new List<EditorPartListFilter<AvailablePart>>();

            foreach (var current in EditorPartList.Instance.GreyoutFilters)
            {
                list.Add(current);
            }
            EditorPartList.Instance.GreyoutFilters.AddFilter(myPseudoFilter);
            foreach (var current in list)
            {
                EditorPartList.Instance.GreyoutFilters.RemoveFilter(current);
                EditorPartList.Instance.GreyoutFilters.AddFilter(current);
            }
        }
        void ConsumeQueue()
        {
            ("EditorPages: Consuming current state...").Log();
            var unusedParts = new HashSet<AvailablePart>(referencedResources.Keys);
            foreach (var part in PartQueueToProcess)
            {
                if (!unusedParts.Remove(part))
                {
                    ("EditorPages is referencing part: " + part.name).Log();
                    referencedResources[part] = Managers.PartManager.GetSafe(part, false);
                }
            }
            foreach (var part in unusedParts)
            {

                ("EditorPages is releasing part: " + part.name).Log();
                referencedResources[part].Release();
                referencedResources.Remove(part);
            }
            PartQueueToProcess.Clear();
            addPartsHasRunOnce = false;
        }
        void Awake()
        {
            if (Config.Disabled || Config.Current.Debug_DontLoadEditorCatalogThumbnailParts)
            {
                enabled = false;
            }
            else
            {
                ("EditorPages: Awake").Log();
                SetupNew();
            }
        }
        void Update()
        {
            if (addPartsHasRunOnce)
            {
                ConsumeQueue();
            }
        }
        void OnDestroy()
        {
            if (EditorPartList.Instance != null && EditorPartList.Instance.GreyoutFilters != null)
            {
                EditorPartList.Instance.GreyoutFilters.RemoveFilter(myPseudoFilter);
                ("Filter removed.").Log();
            }
            myPseudoFilter = null;

            foreach (var el in referencedResources)
            {
                ("EditorPages is releasing part: " + el.Key.name).Log();
                el.Value.Release();
            }
            referencedResources = null;
            // all above didn't help... last resort (".clear()") was removed for now thanks to manual "release"
            // Todo: Find the actual ref that keeps this alive.
            ("EditorPages: OnDestroy").Log();
        }
        ~EditorPages()
        {
            ("EditorPages: Collect").Log();
        }
    }
}
