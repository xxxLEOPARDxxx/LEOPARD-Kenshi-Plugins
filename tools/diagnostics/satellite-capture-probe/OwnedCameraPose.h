#pragma once
namespace ProbeOwnership
{
template <class Camera, class Node>
bool CopyOwnedCameraPose(Camera *owned, Camera *mainCamera, Node *node, Node *root)
{
    if (!owned || !mainCamera || owned == mainCamera || !node || !root || node == root ||
        owned->getParentSceneNode() != node || node->getParent() != root || root->getParent() ||
        node == mainCamera->getParentSceneNode())
        return false;
    // A newly allocated child initially has identity derived caches even when
    // the scene root already carries Kenshi's floating-origin translation.
    // This updates only the known node/root ancestry, never the whole scene.
    node->_getDerivedPositionUpdated();
    owned->setPosition(node->convertWorldToLocalPosition(mainCamera->getDerivedPosition()));
    owned->setOrientation(node->convertWorldToLocalOrientation(mainCamera->getDerivedOrientation()));
    return true;
}
}
