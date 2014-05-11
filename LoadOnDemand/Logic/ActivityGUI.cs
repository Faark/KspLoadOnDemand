using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using UnityEngine;

namespace LoadOnDemand.Logic
{
    [KSPAddon(KSPAddon.Startup.EveryScene, false)]
    class ActivityGUI: MonoBehaviour
    {
        public static int ActiveThumbnailRequests;
        public static int FinishedThumbnailRequests;
        public static int ActiveHighResLoadRequests;
        public static int FinishedHighResLoadRequests;

        static DateTime ShowActivityWindowWhenStillActiveAt = DateTime.MaxValue;


        static string ErrorMessage = null;
        string CurrentError = null;
        string CurrentStatus = null;
        DateTime CurrentFadeoutTimer = DateTime.MinValue;
        bool TryUpdateActiveStatusString()
        {
            if (ActiveHighResLoadRequests != FinishedHighResLoadRequests)
            {
                if (ActiveThumbnailRequests != 0)
                {
                    CurrentStatus =
                        "Preparing Textures: " + FinishedThumbnailRequests + "/" + ActiveThumbnailRequests
                        + Environment.NewLine + "Loading Textures: " + FinishedHighResLoadRequests + "/" + ActiveHighResLoadRequests;
                }
                else
                {
                    CurrentStatus = "Loading Textures: " + FinishedHighResLoadRequests + "/" + ActiveHighResLoadRequests;
                }
            }
            else if (ActiveThumbnailRequests != FinishedThumbnailRequests)
            {
                CurrentStatus = "Preparing Textures: " + FinishedThumbnailRequests + "/" + ActiveThumbnailRequests;
            }
            else
            {
                return false;
            }
            return true;
        }
        String GetFadeoutStatus()
        {
            if (FinishedHighResLoadRequests > 0)
            {
                if (FinishedThumbnailRequests > 0)
                {
                    return FinishedHighResLoadRequests + " textures loaded, " + FinishedThumbnailRequests + " textures prepared!";
                }
                else
                {
                    return FinishedHighResLoadRequests + " textures loaded!";
                }
            }
            else
            {
                return FinishedThumbnailRequests + " textures prepared!";
            }
        }
        void ClearValuesToCloseWindow()
        {
            CurrentStatus = null;
            ShowActivityWindowWhenStillActiveAt = DateTime.MaxValue;
            CurrentFadeoutTimer = DateTime.MinValue;
            int tmp = ActiveThumbnailRequests;
            Interlocked.Add(ref ActiveThumbnailRequests, -tmp);
            Interlocked.Add(ref FinishedThumbnailRequests, -tmp);
            int tmp2 = ActiveHighResLoadRequests;
            Interlocked.Add(ref ActiveHighResLoadRequests, -tmp2);
            Interlocked.Add(ref FinishedHighResLoadRequests, -tmp2);
        }
        void Update()
        {
            if (ErrorMessage != null)
            {
                CurrentError = ErrorMessage;
                CurrentStatus = null;
                CurrentFadeoutTimer = DateTime.MinValue;
                ShowActivityWindowWhenStillActiveAt = DateTime.MaxValue;
            }
            else
            {
                CurrentError = null;
                if (CurrentFadeoutTimer > DateTime.MinValue)
                {
                    // "AGUI.update.fadeout".Log();
                    if (TryUpdateActiveStatusString())
                    {
                        //"AGUI.update.revive".Log();
                        // Revived
                        CurrentFadeoutTimer = DateTime.MinValue;
                    }
                    else if (CurrentFadeoutTimer < DateTime.Now)
                    {
                        // "AGUI.update.close".Log();
                        ClearValuesToCloseWindow();
                    }
                    else
                    {
                        //  "AGUI.update.waitfade".Log();
                        CurrentStatus = GetFadeoutStatus();
                    }
                }
                else if (CurrentStatus != null)
                {
                    //  "AGUI.update.state".Log();
                    if (!TryUpdateActiveStatusString())
                    {
                        // "AGUI.update.state.startFade".Log();
                        CurrentStatus = GetFadeoutStatus();
                        CurrentFadeoutTimer = DateTime.Now + Config.Current.UI_DelayBeforeHidingActivityUI;
                    }
                    else
                    {
                        // "AGUI.update.state.ok".Log();
                    }
                }
                else if (ShowActivityWindowWhenStillActiveAt <= DateTime.Now)
                {
                    //"AGUI.update.start".Log();
                    if (!TryUpdateActiveStatusString())
                        ClearValuesToCloseWindow();
                }
                else
                {
                    // ("AGUI.update.waitFor:" + ShowActivityWindowWhenStillActiveAt.ToString() + ", current " + DateTime.Now.ToString()).Log();
                }
            }
        }
        void OnGUI()
        {
            if (CurrentError != null)
            {
                GUI.Box(new Rect((Screen.width / 2) - 225, 100, 450, 60), CurrentError);
                if (GUI.Button(new Rect(
                    (Screen.width / 2) - 225 + (448 - 80),
                    139,
                    80,
                    18
                    ), "Dismiss"))
                {
                    ErrorMessage = null;
                }
            }
            else if (CurrentStatus != null)
            {
                GUI.Box(new Rect((Screen.width / 2) - 150, 100, 300, 40), CurrentStatus);
            }
        }


        static void MayStartTimer(int numRequestsBefore)
        {
            if (numRequestsBefore == 1)
            {
                //"AGUI.mayStart".Log();
                if (ShowActivityWindowWhenStillActiveAt >= DateTime.MaxValue)
                {
                    ShowActivityWindowWhenStillActiveAt = DateTime.Now + Config.Current.UI_DelayBeforeShowingActivityUI;
                }
                else
                {
                    //"AGui.AlreadyACtive".Log();
                }
            }
        }

        public static void ThumbStarting()
        {
            //"ThumbStart".Log();
            MayStartTimer(Interlocked.Increment(ref ActiveThumbnailRequests));
        }
        public static void ThumbFinished()
        {
            //"ThumbStop".Log();
            Interlocked.Increment(ref FinishedThumbnailRequests);
        }
        public static void HighResStarting()
        {
            //"ResStart".Log();
            MayStartTimer(Interlocked.Increment(ref ActiveHighResLoadRequests));
        }
        public static void HighResFinished()
        {
            ///"ResStop".Log();
            Interlocked.Increment(ref FinishedHighResLoadRequests);
            //FinishedHighResLoadRequests++;
        }
        public static void HighResCanceled()
        {
            Interlocked.Decrement(ref ActiveHighResLoadRequests);
        }
        public static void SetError(String text)
        {
            ErrorMessage = text;
        }
        public static void SetError(Exception err)
        {
            var msg = err.Message ?? err.ToString() ?? "NO ERROR DETAILS AVAILABLE";
            if (msg.Length > 60)
            {
                msg = msg.Substring(0, 60) + "...";
            }
            Logic.ActivityGUI.SetError("LoadOnDemand has detected a problem and was disabled." + Environment.NewLine + msg);
            Config.Disabled = true;
        }
    }
}
