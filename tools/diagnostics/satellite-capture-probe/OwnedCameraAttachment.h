#pragma once
// Own-camera transfer only. This helper never owns or deletes either scene node.
// Kept independent of Ogre types so the exact production operation can be tested offline.
namespace ProbeOwnership
{
template<class Camera,class Node>
bool AttachOwnedCamera(Camera* owned,Camera* mainCamera,Node* destination)
{
    if(!owned||!mainCamera||owned==mainCamera||!destination)
        return false;
    // The shipped Ogre2 SDK creates cameras already attached to the root scene node.
    // Detach this new owned object only; never detach/delete the main camera or a shared node.
    if(destination==mainCamera->getParentSceneNode())
        return false;
    if(owned->getParentSceneNode()==destination)
        return true;
    if(owned->isAttached())
        owned->detachFromParent();
    destination->attachObject(owned);
    return true;
}
}
