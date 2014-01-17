using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OnDemandLoading
{
    /// <summary>
    /// 
    /// </summary>
    /// <remarks>
    /// Objs not listed are treated as "always loaded & referenced"
    /// </remarks>
    /// <typeparam name="TRefKey"></typeparam>
    /// <typeparam name="TRefData"></typeparam>
    public abstract class CustomRefCountManagerTemplate<TRefKey, TRefData>
    {
        protected class RefElement
        {
            public int RefCount = 0;
            public State State = State.Unloaded;
            public List<Action> StateChangeListeners;
            public TRefData RefData;
        }
        protected Dictionary<TRefKey, RefElement> States = new Dictionary<TRefKey, RefElement>();

        public CustomRefCountManagerTemplate(Dictionary<TRefKey, TRefData> elementsToManage)
        {
            foreach (var el in elementsToManage)
            {
                States.Add(el.Key, new RefElement() { RefData = el.Value });
            }
        }
        public enum State { Unloaded, Loading, Loaded };


        /// <summary>
        /// 
        /// </summary>
        /// <param name="obj"></param>
        /// <param name="on_load_callback">A callback that will be fired once the part is available or instantly, if it already is. It will not fire when RefCnt drops to 0 before loading finshes</param>
        public void IncrementUsage(TRefKey obj, Action on_load_callback = null)
        {
            RefElement state;
            if (States.TryGetValue(obj, out state))
            {
                state.RefCount++;
                if (state.State == State.Unloaded)
                {
                    state.State = State.Loading;
                    StartLoadObject(obj, state.RefData);
                }
                if (on_load_callback != null)
                {
                    if (state.State == State.Loaded)
                    {
                        on_load_callback();
                    }
                    else
                    {
                        (state.StateChangeListeners ?? (state.StateChangeListeners = new List<Action>())).Add(on_load_callback);
                    }
                }
            }
            else
            {
                if (on_load_callback != null)
                    on_load_callback();
            }
        }
        public bool IsLoaded(TRefKey obj)
        {
            RefElement state;
            if (States.TryGetValue(obj, out state))
            {
                return state.State == State.Loaded;
            }
            return true;
        }
        public void DecrementUsage(TRefKey obj)
        {
            RefElement state;
            if (States.TryGetValue(obj, out state))
            {
                var refCnt = --state.RefCount;
                if (refCnt <= 0)
                {
                    //throw new NotImplementedException("Todo: may start unloading? Mark as unloadable? 'dead' timer? resource counters? sth like that...");
                    if (refCnt < 0)
                    {
                        throw new Exception("sth has gone horribly wrong... #584732");
                    }
                    else
                    {
                        UnloadObject(obj, state.RefData);
                        state.State = State.Unloaded;
                    }
                }
            }
        }

        protected void AnnounceLoadFinished(TRefKey key)
        {
            var state = States[key];
            if (state.RefCount > 0)
            {
                state.State = State.Loaded;
                if (state.StateChangeListeners != null)
                {
                    foreach (var callback in state.StateChangeListeners)
                    {
                        callback();
                    }
                    state.StateChangeListeners = null;
                }
            }
            else
            {
                throw new NotImplementedException();
                state.State = State.Unloaded;
                state.StateChangeListeners = null;
            }
        }
        protected abstract void StartLoadObject(TRefKey key, TRefData data);
        protected abstract void UnloadObject(TRefKey key, TRefData data);
    }
}
