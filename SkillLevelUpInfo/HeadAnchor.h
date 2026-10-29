#pragma once

#include <Windows.h>
#include <float.h>
#include <string>
#include <ogre/OgreVector3.h>

namespace SkillHeadAnchor
{
    // World units, as in RecruitMarkers. ScreenLabel adds this vector to the
    // tracked object's current world position before projection and rising.
    const float kGap = 0.4f;
    const std::string kBones[3] = { "Bip01 Head", "Head", "head" };

    inline Ogre::Vector3 Fallback()
    {
        return Ogre::Vector3(0, 2.2f + kGap, 0);
    }

    inline bool Finite(const Ogre::Vector3& p)
    {
        return _finite(p.x) && _finite(p.y) && _finite(p.z);
    }

    inline bool FromWorldHead(const Ogre::Vector3& base, const Ogre::Vector3& head,
                              Ogre::Vector3& offset)
    {
        if (!Finite(base) || !Finite(head)) return false;
        const Ogre::Vector3 d = head - base;
        if (d.x < -12 || d.x > 12 || d.z < -12 || d.z > 12 ||
            d.y < -4 || d.y > 20 || d.squaredLength() <= 0.04f)
            return false;
        offset = d;
        offset.y += kGap;
        return true;
    }

    template<class CharacterT>
    bool Position(CharacterT* c, Ogre::Vector3& base)
    {
        __try { base = c->getPosition(); return Finite(base); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

    template<class CharacterT>
    bool Bone(CharacterT* c, int index, const Ogre::Vector3& base,
              Ogre::Vector3& offset)
    {
        __try
        {
            const Ogre::Vector3 head = c->getBoneWorldPosition(kBones[index]);
            return FromWorldHead(base, head, offset);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

    // Called once when creating a notification. The game owns all subsequent
    // tracking, rise, fade and destruction; no label/bone cache is retained.
    template<class CharacterT>
    Ogre::Vector3 TrackingOffset(CharacterT* c)
    {
        Ogre::Vector3 base, offset;
        if (c != NULL && Position(c, base))
            for (int i = 0; i < 3; ++i)
                if (Bone(c, i, base, offset)) return offset;
        return Fallback();
    }
}
