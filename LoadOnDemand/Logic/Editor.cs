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

        public void RefAllFrom(Part startPart, HashSet<AvailablePart> usedParts)
        {
            usedParts.Add(startPart.partInfo);
            foreach (var sub in startPart.children)
            {
                RefAllFrom(sub, usedParts);
            }
        }
        public void Update()
        {
            if (EditorLogic.fetch != null)
            {
                HashSet<AvailablePart> usedParts = new HashSet<AvailablePart>();
                if (EditorLogic.SelectedPart != null)
                {
                    RefAllFrom(EditorLogic.SelectedPart, usedParts);
                }
                if (EditorLogic.startPod != null)
                {
                    RefAllFrom(EditorLogic.startPod, usedParts);
                }
                HashSet<AvailablePart> unusedParts = new HashSet<AvailablePart>(referencedResources.Keys);
                foreach (var part in usedParts)
                {
                    if (!unusedParts.Remove(part))
                    {
                        referencedResources[part] = Managers.PartManager.GetSafe(part, false);
                    }
                }
                foreach (var part in unusedParts)
                {
                    referencedResources.Remove(part);
                }
            }
        }
    }

    /// <summary>
    /// Todo3: Last page refs aren't GC'ed at all... find out why and fix it!
    /// Got it: EditorPartList.Instance.GreyoutFilters might not be recycled, thus still referencing this. But thats actually tricky, since i fck up good cleanup... fixed. todo: Test it!
    /// </summary>
    [KSPAddon(KSPAddon.Startup.EditorAny, false)]
    class EditorPages : MonoBehaviour
    {
        Dictionary<AvailablePart, Resources.IResource> referencedResources = new Dictionary<AvailablePart, Resources.IResource>();
        HashSet<AvailablePart> PartQueueToProcess = new HashSet<AvailablePart>();
        bool addPartsHasRunOnce = false;
        EditorPartListFilter myPseudoFilter;

        bool AddPartToList(AvailablePart part)
        {
            PartQueueToProcess.Add(part);
            addPartsHasRunOnce = true;
            return true;
        }
        void SetupNew()
        {
            myPseudoFilter = new EditorPartListFilter("LodPseudoFilter", AddPartToList);
            var list = new List<EditorPartListFilter>();
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
                    ("EditorPages: Accuqiring " + part.name).Log();
                    referencedResources[part] = Managers.PartManager.GetSafe(part, false);
                }
            }
            foreach (var part in unusedParts)
            {
                ("EditorPages: Dropping " + part.name).Log();
                referencedResources.Remove(part);
            }
            PartQueueToProcess.Clear();
            addPartsHasRunOnce = false;
        }
        void Awake()
        {
            ("EditorPages: Awake").Log();
            SetupNew();
        }
        void Update()
        {
            if (addPartsHasRunOnce)
            {
                ConsumeQueue();
            }
        }
        void OnDestroy(){
            if (EditorPartList.Instance != null && EditorPartList.Instance.GreyoutFilters != null)
            {
                EditorPartList.Instance.GreyoutFilters.RemoveFilter(myPseudoFilter);
                ("Filter removed.").Log();
            }
            myPseudoFilter = null;

            // all above didn't help... last resort:
            referencedResources.Clear();
            // Todo: Find the actual ref that keeps this alive.
            ("EditorPages: OnDestroy").Log();
        }
        ~EditorPages()
        {
            ("EditorPages: Collect").Log();
        }
    }
}
