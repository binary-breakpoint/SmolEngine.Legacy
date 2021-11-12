using System;
using System.Runtime.InteropServices;

namespace SmolEngine
{
    public enum ShadowType: ushort
    {
        None,
        Hard,
        Soft
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct DirectionalLightComponent
    {
        private Vector3                 _Color;
        private Vector3                 _Direction;
        private ShadowType              _ShadowType;
        private float                   _Intensity;
        private bool                    _IsActive;
        unsafe private readonly uint*   _Handler;

        public Vector3 Color
        {
            get { return _Color; }
            set { _Color = value; OnValueChanged(); }
        }

        public Vector3 Direction
        {
            get { return _Direction; }
            set { _Direction = value; OnValueChanged(); }
        }

        public float Intensity
        {
            get { return _Intensity; }
            set { _Intensity = value; OnValueChanged(); }
        }

        public ShadowType ShadowType
        {
            get { return _ShadowType; }
            set { _ShadowType = value; OnValueChanged(); }
        }

        public bool IsActive
        {
            get { return _IsActive; }
            set { _IsActive = value; OnValueChanged(); }
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
