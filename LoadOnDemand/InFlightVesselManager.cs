using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

namespace OnDemandLoading
{
    [KSPAddon(KSPAddon.Startup.Flight, false)]
    public class InFlightVesselManager : MonoBehaviour
    {
        public static float defaultLoadDistance;
        public static float defaultUnloadDistance;

        static Dictionary<Vessel, HashSet<AvailablePart>> ReferencedPartsPerVessel = new Dictionary<Vessel,HashSet<AvailablePart>>();
        static Vessel lastFocusedVessel = null;

#warning Issues to solve:
        /* - Vessel split... part refs not properly updated
         * - Vessel split... new vessel not properly registerd
         */
        public void Awake()
        {
            //Debug.Log("LOD - InFlightVesselManager started");
        }
        public void Update()
        {
            if (!FlightGlobals.ready)
                return;
            //Debug.Log("LOD - InFlightVesselManger update (FGR)");
            var av = FlightGlobals.ActiveVessel;
            var vesselsToUnloadParts = new HashSet<Vessel>(ReferencedPartsPerVessel.Keys);
            if (lastFocusedVessel != av)
            {
                Debug.Log("LOD - IFVM - AV changed");
                if (!ReferencedPartsPerVessel.ContainsKey(av))
                {
                    var referencedParts = ReferencedPartsPerVessel[av] = new HashSet<AvailablePart>();
                    foreach (var protoPart in av.protoVessel.protoPartSnapshots)
                    {
                        if (referencedParts.Add(protoPart.partInfo))
                        {
                            CustomManagers.Parts.IncrementUsage(protoPart.partInfo);
                        }
                    }
                }
                lastFocusedVessel = av;
            }
            foreach (var vessel in FlightGlobals.Vessels)
            {
                vesselsToUnloadParts.Remove(vessel);
                if (vessel == av)
                    continue;
                if (vessel.loaded)
                {
                    if (Vector3.Distance(av.vesselTransform.position, vessel.vesselTransform.position) > defaultLoadDistance)
                    {
                        //Debug.Log("LOD - IFVM - Loaded vessel found to unload: " + vessel.vesselName);
                        vesselsToUnloadParts.Add(vessel);
                        vessel.Unload();
                    }
                    else
                    {
                        //Debug.Log("LOD - IFVM - Loaded vessel will not be touched: " + vessel.vesselName);
                    }
                }
                else
                {
                    if (Vector3.Distance(av.vesselTransform.position, vessel.vesselTransform.position) < defaultUnloadDistance)
                    {
                        //Debug.Log("LOD - IFVM - Unloaded vessel has to be loaded: " + vessel.vesselName);
                        HashSet<AvailablePart> vesselParts;
                        if (!ReferencedPartsPerVessel.TryGetValue(vessel, out vesselParts))
                        {
                            vesselParts = new HashSet<AvailablePart>();
                            foreach (var protoPart in vessel.protoVessel.protoPartSnapshots)
                            {
                                if (vesselParts.Add(protoPart.partInfo))
                                {
                                    CustomManagers.Parts.IncrementUsage(protoPart.partInfo);
                                }
                            }
                            ReferencedPartsPerVessel[vessel] = vesselParts;
                        }
                        var missing = 0;
                        foreach (var partTemplate in vesselParts)
                        {
                            if (!CustomManagers.Parts.IsLoaded(partTemplate))
                                missing++;
                        }
                        if (missing <= 0)
                        {
                            vessel.Load();
                        }
                    }
                    else
                    {
                        //Debug.Log("LOD - IFVM - Unloaded vessel will not be touched: " + vessel.vesselName);
                    }
                }
            }
            //Debug.Log("LOD - IFVM - Elements in unload list: " + vesselsToUnloadParts.Count);
            foreach (var vessel in vesselsToUnloadParts)
            {
                HashSet<AvailablePart> parts;
                if (ReferencedPartsPerVessel.TryGetValue(vessel, out parts))
                {
                    foreach (var part in ReferencedPartsPerVessel[vessel])
                    {
                        CustomManagers.Parts.DecrementUsage(part);
                    }
                    ReferencedPartsPerVessel.Remove(vessel);
                }
                else
                {
                    Debug.Log("Could not unload vessel " + vessel.vesselName);
                }
            }
        }
    }
}
