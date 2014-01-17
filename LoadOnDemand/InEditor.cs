using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

namespace OnDemandLoading
{
    [KSPAddon(KSPAddon.Startup.EditorAny, false)]
    public class InEditor: MonoBehaviour 
    {
        Part lastPartSelected;
        //AvailablePart mouseFocusRef = null;
        public void RefPart(Part p)
        {
            // what about destroys due to scene load?
            //var oldOnEditorDestr
            p.OnEditorDestroy += () =>
            {
                // losse ref
            };
        }
        public void Awake()
        {
            //InputLockManager.SetControlLock(ControlTypes.EDITOR_ICON_PICK, "LoadOnDemand");
        }
        public void Update()
        {
            // Events:
            // EditorLogic.SelectedPart changed
            // EditorLogic.startPod changed

        }
    }
}
