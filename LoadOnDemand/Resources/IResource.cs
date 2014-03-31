using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace LoadOnDemand.Resources
{
    public interface IResource
    {
        // Todo18: Events? Un-neccessary/unused for now, but might be easy to implement...
        bool Loaded { get; }
        /// <summary>
        /// Marks this resource(s) as not used anymore. It will be unload once all all references to this resource(s) are released.
        /// </summary>
        void Release();
        /// <summary>
        /// Creates a copy reference to this resource. Both references have to be released.
        /// </summary>
        /// <param name="throwIfDead">If this bool is set, Clone() will throw an InvalidOperationException if it contains a released resource</param>
        /// <returns></returns>
        IResource Clone(bool throwIfDead);
    }
    public static class IResourceExtends
    {
        public static void Clone(this IResource self)
        {
            self.Clone(false);
        }
    }
    public sealed class ResourceDummy : IResource
    {
        public bool Loaded { get; private set; }
        public ResourceDummy()
        {
            Loaded = true;
        }
        public override string ToString()
        {

            return "Dummy Resource";
        }


        public void Release()
        {
            Loaded = false;
        }

        public IResource Clone(bool throwIfDead)
        {
            if (throwIfDead && !Loaded)
            {
                throw new InvalidOperationException("Resource Collection was already released.");
            }
            else
            {
                return new ResourceDummy { Loaded = Loaded };
            }
        }
    }
}
