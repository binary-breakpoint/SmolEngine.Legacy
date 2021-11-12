using System;
using System.Runtime.CompilerServices;

namespace SmolEngine
{
    public struct MeshComponent
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        extern static bool SetMaterial_EX(uint mesh_index, uint materialID, uint entity_id);
        [MethodImpl(MethodImplOptions.InternalCall)]
        extern static bool SetMesh_EX(ulong assetID, uint entity_id);


        private bool                    _IsVisible;
        private bool                    _IsActive;
        private ulong                   _MeshID;
        private readonly unsafe uint*   _Handler;

        public bool IsVisible
        {
            get { return _IsVisible; }
            set { _IsVisible = value; OnValueChanged(); }
        }

        public ulong MeshID
        {
            get { return _MeshID; }
            set 
            { 
                _MeshID = value;
                unsafe
                {
                    SetMesh_EX(_MeshID, *_Handler);
                }
            }
        }

        public bool SetMaterial(uint node_index, Material material)
        {

            return false;
        }

        public bool IsActibe()
        {
            return _IsActive;
        }

        private void OnValueChanged()
        {
            unsafe
            {
                if (_Handler != null)
                    Utils.OnComponentUpdated(ref this, *_Handler);
            }
        }
    }
}
