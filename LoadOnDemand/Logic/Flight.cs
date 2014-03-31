using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

namespace LoadOnDemand.Logic
{
    /// <summary>
    /// Logic.Flight tracks vessels and stores references for their resources as long as they exist.
    /// </summary>
    [KSPAddon(KSPAddon.Startup.Flight, false)]
    public class Flight : MonoBehaviour
    {
        Dictionary<Vessel, Dictionary<AvailablePart, Resources.IResource>> ReferencedVessels = new Dictionary<Vessel, Dictionary<AvailablePart, Resources.IResource>>();
        void Update()
        {
            // Todo: Optimize performance...
            if (FlightGlobals.ready)
            {
                var vesselsToUnload = new HashSet<Vessel>(ReferencedVessels.Keys);
                foreach (var vessel in FlightGlobals.Vessels)
                {
                    if (vessel.loaded)
                    {
                        Dictionary<AvailablePart, Resources.IResource> partRes;
                        HashSet<AvailablePart> partsToRemove;
                        if (ReferencedVessels.TryGetValue(vessel, out partRes))
                        {
                            vesselsToUnload.Remove(vessel);
                            partsToRemove = new HashSet<AvailablePart>(partRes.Keys);
                        }
                        else
                        {
                            //("Adding vesssel: " + vessel.GetHashCode()).Log();
                            partsToRemove = new HashSet<AvailablePart>();
                            partRes = ReferencedVessels[vessel] = new Dictionary<AvailablePart, Resources.IResource>();
                        }
                        foreach (var part in vessel.Parts)
                        {
                            if (!partRes.ContainsKey(part.partInfo))
                            {
                                partRes[part.partInfo] = Managers.PartManager.GetSafe(part.partInfo);
                            }
                            else
                            {
                                partsToRemove.Remove(part.partInfo);
                            }
                        }
                        foreach (var part in partsToRemove)
                        {
                            partRes[part].Release();
                            partRes.Remove(part);
                        }
                    }
                }
                foreach (var vessel in vesselsToUnload)
                {
                    foreach (var parts in ReferencedVessels[vessel])
                    {
                        parts.Value.Release();
                    }
                    ReferencedVessels.Remove(vessel);
                }
            }
            else
            {
                // Todo: Think about not releasing everything at all? Nope, Flight will be garbage collected anyway...
            }
        }
        void Awake()
        {
            if (Config.Disabled)
            {
                enabled = false;
            }
            else
            {
                GameEvents.onVesselDestroy.Add(OnVesselDestroyed);
            }
        }
        void OnVesselDestroyed(Vessel vessel)
        {
            //("Vessel.OnDestroy: " + vessel.GetHashCode()).Log();
            Dictionary<AvailablePart, Resources.IResource> parts;
            if (ReferencedVessels.TryGetValue(vessel, out parts))
            {
                //"Dereferencing vessel".Log();
                foreach (var part in parts)
                {
                    part.Value.Release();
                }
                ReferencedVessels.Remove(vessel);
            }
            else
            {
                //"Unreferenced vessel".Log();
            }
        }
        void OnDestroy()
        {
            GameEvents.onVesselDestroy.Remove(OnVesselDestroyed);
            foreach (var vessel in ReferencedVessels)
            {
                foreach (var part in vessel.Value)
                {
                    part.Value.Release();
                }
            }
            ReferencedVessels = null;
        }
    }
}
