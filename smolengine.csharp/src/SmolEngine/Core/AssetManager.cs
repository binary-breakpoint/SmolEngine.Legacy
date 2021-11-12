using System.Runtime.CompilerServices;

namespace SmolEngine
{
    public class Asset
    {
        internal readonly ulong AssetID;
        internal Asset(ulong id)
        {
            AssetID = id;
        }
    }

    public static class AssetManager
    {
        enum AssetType: ushort
        {
            Prefab,
            Mesh,
            AudioClip,
            Material,
            Texture
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        extern static ulong LoadAsset_EX(string filePath, ushort type);

        public static Prefab LoadPrefab(string filePath)
        {
            ulong id = LoadAsset_EX(filePath, (ushort)AssetType.Prefab);
            if (id > 0)
            {
                return new Prefab(id);
            }

            return null;
        }

        public static Mesh LoadMesh(string filePath)
        {
            ulong id = LoadAsset_EX(filePath, (ushort)AssetType.Mesh);
            if (id > 0)
            {
                return new Mesh(id);
            }

            return null;
        }

        public static AudioClip LoadAudioClip(string filePath)
        {
            ulong id = LoadAsset_EX(filePath, (ushort)AssetType.AudioClip);
            if (id > 0)
            {
                return new AudioClip(id);
            }

            return null;
        }

        public static Material LoadMaterial(string filePath)
        {
            ulong id = LoadAsset_EX(filePath, (ushort)AssetType.Material);
            if (id > 0)
            {
                return new Material(id);
            }

            return null;
        }

        public static Texture LoadTexture(string filePath)
        {
            ulong id = LoadAsset_EX(filePath, (ushort)AssetType.Texture);
            if (id > 0)
            {
                return new Texture(id);
            }

            return null;
        }
    }
}
