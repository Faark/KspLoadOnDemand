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
        Dictionary<Vessel, Dictionary<AvailablePart, Resources.IResource>> referencedVessels = new Dictionary<Vessel, Dictionary<AvailablePart, Resources.IResource>>();
        public void Update()
        {
            // Todo: Optimize performance...
            if (FlightGlobals.ready)
            {
                var vesselsToUnload = new HashSet<Vessel>(referencedVessels.Keys);
                foreach (var vessel in FlightGlobals.Vessels)
                {
                    if (vessel.loaded)
                    {
                        Dictionary<AvailablePart, Resources.IResource> partRes;
                        HashSet<AvailablePart> partsToRemove;
                        if (referencedVessels.TryGetValue(vessel, out partRes))
                        {
                            vesselsToUnload.Remove(vessel);
                            partsToRemove = new HashSet<AvailablePart>(partRes.Keys);
                        }
                        else
                        {
                            partsToRemove = new HashSet<AvailablePart>();
                            partRes = referencedVessels[vessel] = new Dictionary<AvailablePart, Resources.IResource>();
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
                            partRes.Remove(part);
                        }
                    }
                }
                foreach (var vessel in vesselsToUnload)
                {
                    referencedVessels.Remove(vessel);
                }
            }
            else
            {
                // Todo: Think about not releasing everything at all? Nope, Flight will be garbage collected anyway...
            }
        }
    }
}
