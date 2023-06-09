// Copyright(c) 2019 - 2022, KaoruXun All rights reserved.

using System;
using System.Reflection;
using System.Runtime.InteropServices;

namespace MetaDotManagedCore
{
    public abstract class Layer
    {
        public virtual void OnAttach() { }
        public virtual void OnDetach() { }
        public virtual void OnUpdate() { }

    }
}
