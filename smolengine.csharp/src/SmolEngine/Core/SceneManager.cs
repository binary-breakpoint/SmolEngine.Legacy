using System;
using System.Runtime.CompilerServices;

namespace SmolEngine
{
    public static class SceneManager
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        extern static uint FindActorByName_EX(string name);

        [MethodImpl(MethodImplOptions.InternalCall)]
        extern static uint FindActorByTag_EX(string name);

        [MethodImpl(MethodImplOptions.InternalCall)]
        extern static bool IsActorExistsID_EX(uint id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        extern static uint CreateActor_EX(string name, string tag);

        [MethodImpl(MethodImplOptions.InternalCall)]
        extern static bool DestroyActor_EX(uint id);

        public static Actor CreateActor(string name = "", string tag = "")
        {
            uint id = CreateActor_EX(name, tag);
            if(id > 0)
            {
                return new Actor(id);
            }

            return null;
        }

        public static bool DestroyActor(Actor obj)
        {
            uint id = obj.GetID();
            return DestroyActor_EX(id);
        }

        public static Actor FindActorByName(string name)
        {
            uint id = FindActorByName_EX(name); 
            if(id > 0)
            {
                return new Actor(id);
            }

            return null;
        }

        public static Actor FindActorByTag(string tag)
        {
            uint id = FindActorByTag_EX(tag);
            if (id > 0)
            {
                return new Actor(id);
            }

            return null;
        }

        public static Actor FindActorByID(uint id)
        {
            bool exists = IsActorExistsID_EX(id);
            if(exists)
            {
                return new Actor(id);
            }

            return null;
        }
    }
}
