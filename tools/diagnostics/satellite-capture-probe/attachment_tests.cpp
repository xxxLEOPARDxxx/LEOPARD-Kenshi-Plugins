#include "OwnedCameraAttachment.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
struct Node;
struct Camera
{
    Node* parent;
    int detached;
    Camera():parent(NULL),detached(0){}
    Node* getParentSceneNode()const{return parent;}
    bool isAttached()const{return parent!=NULL;}
    void detachFromParent();
};
struct Node
{
    int attachments,detachments;
    bool failAttach;
    Node():attachments(0),detachments(0),failAttach(false){}
    void attachObject(Camera* camera)
    {
        // Ogre SceneNode.cpp:124 contract observed in the actual game exception.
        if(camera->isAttached())throw std::runtime_error("Object already attached to a SceneNode or a Bone");
        if(failAttach)throw std::runtime_error("injected destination failure");
        ++attachments;camera->parent=this;
    }
    void detachObject(Camera* camera)
    {if(camera->parent!=this)throw std::runtime_error("wrong parent");++detachments;camera->parent=NULL;}
};
void Camera::detachFromParent(){if(parent){parent->detachObject(this);++detached;}}
unsigned checks=0;
void Check(bool ok,const char* text){++checks;if(!ok){std::cerr<<"FAIL "<<text<<"\n";std::exit(1);}}
int main()
{
    Camera mainCamera,owned;Node mainNode,engineNode,destination;
    mainNode.attachObject(&mainCamera);engineNode.attachObject(&owned);
    bool transferred=false;
    try{transferred=ProbeOwnership::AttachOwnedCamera(&owned,&mainCamera,&destination);}catch(...){ }
    Check(transferred&&owned.parent==&destination,"already attached newly created camera transfers safely");
    Check(owned.detached==1&&engineNode.detachments==1,"only owned camera detached from existing parent");
    Check(mainCamera.parent==&mainNode&&mainCamera.detached==0&&mainNode.detachments==0,"main camera and node preserved");
    Check(ProbeOwnership::AttachOwnedCamera(&owned,&mainCamera,&destination)&&owned.detached==1&&destination.attachments==1,"already at requested node is idempotent");
    Camera unattached;Node fresh;
    Check(ProbeOwnership::AttachOwnedCamera(&unattached,&mainCamera,&fresh)&&unattached.parent==&fresh&&unattached.detached==0,"unattached owned camera attaches without detach");
    Check(!ProbeOwnership::AttachOwnedCamera(&mainCamera,&mainCamera,&fresh)&&mainCamera.parent==&mainNode,"main-camera identity rejected before any mutation");
    Check(!ProbeOwnership::AttachOwnedCamera(&owned,&mainCamera,&mainNode)&&owned.parent==&destination,"main-camera node cannot be used as destination");
    Camera partial;Node oldParent,badDestination;oldParent.attachObject(&partial);badDestination.failAttach=true;
    bool threw=false;try{ProbeOwnership::AttachOwnedCamera(&partial,&mainCamera,&badDestination);}catch(const std::runtime_error&){threw=true;}
    Check(threw&&partial.parent==NULL&&badDestination.attachments==0,"attach failure leaves only owned camera detached for cleanup");
    Check(mainCamera.parent==&mainNode&&mainCamera.detached==0,"partial failure preserves main camera");
    // Existing production catch owns this camera and destination separately; a detached owned
    // camera is a valid destroyCamera input and no shared/default parent node is deleted.
    Check(partial.detached==1&&oldParent.detachments==1,"partial failure does not detach unrelated objects");
    Check(!ProbeOwnership::AttachOwnedCamera(static_cast<Camera*>(NULL),&mainCamera,&fresh),"null owned camera rejected");
    std::cout<<checks<<" production camera-attachment checks passed\n";
}
