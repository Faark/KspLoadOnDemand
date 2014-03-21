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
    }
    public sealed class ResourceDummy: IResource
    {
        public bool Loaded { get { return true; } }
        public override string ToString()
        {
            return "Dummy Resource";
        }
    }
}
