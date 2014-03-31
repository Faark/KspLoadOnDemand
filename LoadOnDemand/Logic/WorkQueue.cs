using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

namespace LoadOnDemand.Logic
{
    /// <summary>
    /// This class allows other code to delay work and, more importantly, pass jobs to the Unity thread.
    /// </summary>
    [KSPAddon(KSPAddon.Startup.EveryScene, false)]
    class WorkQueue: MonoBehaviour
    {
        static System.Object LockObj = new System.Object();
        static Queue<Action> Jobs = new Queue<Action>();
        static Queue<Action> OtherList = new Queue<Action>();

        public static void AddJob(Action job)
        {
            lock (LockObj)
            {
                Jobs.Enqueue(job);
            }
        }
        void Update()
        {
            var currentJobs = Jobs;
            lock (LockObj)
            {
                Jobs = OtherList;
            }
            OtherList = currentJobs;
            List<Exception> exceptions = null;
            while (currentJobs.Count > 0)
            {
                try
                {
                    currentJobs.Dequeue()();
                }
                catch (Exception err)
                {
                    err.Log();
                    if (exceptions == null)
                    {
                        exceptions = new List<Exception>();
                    }
                    exceptions.Add(err);
                }
            }
            if (exceptions != null)
            {
                throw new AggregateException("Error when running Jobs.", exceptions);
            }
            /*
            while (true)
            {
                Action current;
                lock (Jobs)
                {
                    if (Jobs.Count <= 0)
                    {
                        break;
                    }
                    current = Jobs.Dequeue();
                }
                current();
            }
             */
        }
        void Awake()
        {
            if (Config.Disabled)
            {
                this.enabled = false;
            }
        }
    }
}
