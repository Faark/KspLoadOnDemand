using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OnDemandLoading
{
    public static class CustomManagers
    {
        public static CustomInternalManager Internals = new CustomInternalManager();
        public static CustomPartManager Parts = new CustomPartManager();
    }
}
