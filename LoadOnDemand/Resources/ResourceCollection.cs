using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace LoadOnDemand.Resources
{
    /// <summary>
    /// Holds references to resources
    /// </summary>
    /// <remarks>The primary purpos of a ResourceCollection is to keep Resources alive / prevent them from being garbage collected as long as an object exist.</remarks>
    public class ResourceCollection : IResource
    {
        bool mLoaded = false;
        public bool Loaded
        {
            get
            {
                if (mLoaded)
                    return true;

                if (Resources == null)
                    return false;

                if (Resources.Any(res => !res.Loaded))
                    return false;

                mLoaded = true;
                return true;
            }
        }
        private IEnumerable<IResource> Resources { get; /*private*/ set; }

        private ResourceCollection(bool loaded, IEnumerable<IResource> resources)
        {
            mLoaded = loaded;
            Resources = resources.ToArray();
        }
        public ResourceCollection(IEnumerable<IResource> resources)
        {
            if (resources == null)
            {
                throw new ArgumentException("resources cannot be null", "resources");
            }
            Resources = resources.ToArray();
        }

        public override string ToString()
        {
            if (Resources == null)
            {
                return "Released Resource Collection.";
            }
            else
            {
                return "Resource Collection: "
                    + Environment.NewLine + "  "
                    + String.Join(Environment.NewLine + "  ", String.Join(Environment.NewLine, Resources.Select(res => res.ToString()).ToArray()).Split(new[] { Environment.NewLine }, StringSplitOptions.None));
            }
        }


        public void Release()
        {
            if (Resources == null)
                return;
            foreach (var el in Resources)
            {
                el.Release();
            }
            Resources = null;
        }

        public IResource Clone(bool throwIfDead)
        {
            if (Resources == null)
            {
                if (throwIfDead)
                {
                    throw new InvalidOperationException("Resource Collection was already released.");
                }
                else
                {
                    return new ResourceCollection(mLoaded, Resources);
                }
            }
            else
            {
                return new ResourceCollection(mLoaded, Resources.Select(el => el.Clone(throwIfDead)));
            }
        }
    }
}
