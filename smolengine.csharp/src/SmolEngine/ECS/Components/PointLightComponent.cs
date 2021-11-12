using System;
using System.Runtime.InteropServices;

namespace SmolEngine
{
    [StructLayout(LayoutKind.Sequential)]
    public struct PointLightComponent
    {
        private Vector3                   _Color;
        private float                     _Intensity;
        private float                     _Raduis;
        private bool                      _IsActive;
        unsafe private readonly uint*     _Handler;

        public Vector3 Color
        {
            get { return _Color; }
            set { _Color = value; OnValueChanged(); }
        }

        public float Intensity
        {
            get { return _Intensity; }
            set { _Intensity = value; OnValueChanged(); }
        }

        public float Raduis
        {
            get { return _Raduis; }
            set { _Raduis = value; OnValueChanged(); }
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
