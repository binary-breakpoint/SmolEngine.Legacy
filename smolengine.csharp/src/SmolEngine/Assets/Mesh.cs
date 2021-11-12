using System;
using System.Runtime.CompilerServices;

namespace SmolEngine
{
    public class Mesh : Asset
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        extern static uint GetChildsCount_EX(ulong id);

        internal Mesh(ulong id) : base(id)
        {

        }

        public uint GetChildsCount()
        {
            return GetChildsCount_EX(AssetID);
        }
    }
}
