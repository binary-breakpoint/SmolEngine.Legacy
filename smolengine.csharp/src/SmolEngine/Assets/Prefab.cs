using System;
using System.Runtime.CompilerServices;

namespace SmolEngine
{
    public class Prefab: Asset
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        extern static uint Instantiate_EX(ulong id, ref Vector3 pos, ref Vector3 scale, ref Vector3 rotation);

        internal Prefab(ulong id) : base(id) { }

        public Actor Instantiate(Vector3 pos)
        {
            Vector3 scale = new Vector3(1);
            Vector3 rotation = new Vector3(0);
            uint id = Instantiate_EX(AssetID, ref pos, ref scale, ref rotation);
            if( id > 0)
            {
                return new Actor(id);
            }

            return null;
        }

        public Actor Instantiate(Vector3 pos, Vector3 scale)
        {
            Vector3 rotation = new Vector3(0);
            uint id = Instantiate_EX(AssetID, ref pos, ref scale, ref rotation);
            if (id > 0)
            {
                return new Actor(id);
            }

            return null;
        }

        public Actor Instantiate(Vector3 pos, Vector3 scale, Vector3 rotation)
        {
            uint id = Instantiate_EX(AssetID, ref pos, ref scale, ref rotation);
            if (id > 0)
            {
                return new Actor(id);
            }

            return null;
        }
    }
}
