#include "OwnedCameraPose.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
struct Vec
{
    double x,y,z;
    Vec(double X=0,double Y=0,double Z=0):x(X),y(Y),z(Z){}
};
struct Node
{
    Node *parent;
    Vec current, cached;
    unsigned updates, conversions;
    bool fail;
    Node(Node *p=NULL):parent(p),updates(0),conversions(0),fail(false){}
    Node *getParent(){return parent;}
    Vec _getDerivedPositionUpdated()
    {
        ++updates;
        if(fail)throw std::runtime_error("update failed");
        cached=current;
        if(parent)
        {
            Vec p=parent->_getDerivedPositionUpdated();
            cached=Vec(current.x+p.x,current.y+p.y,current.z+p.z);
        }
        return cached;
    }
    Vec convertWorldToLocalPosition(Vec p)
    {
        ++conversions;
        return Vec(p.x-cached.x,p.y-cached.y,p.z-cached.z);
    }
    double convertWorldToLocalOrientation(double q){++conversions;return q;}
};
struct Camera
{
    Node *parent;
    Vec position;
    double q;
    unsigned writes;
    Camera(Node *p):parent(p),q(1),writes(0){}
    Node *getParentSceneNode(){return parent;}
    Vec getDerivedPosition(){return position;}
    double getDerivedOrientation(){return q;}
    void setPosition(Vec p){position=p;++writes;}
    void setOrientation(double v){q=v;++writes;}
};
unsigned checks=0;
void Check(bool ok,const char *message)
{
    ++checks;
    if(!ok){std::cerr<<"FAIL "<<message<<'\n';std::exit(1);}
}
bool Near(double a,double b){return std::fabs(a-b)<0.00001;}
int main()
{
    using ProbeOwnership::CopyOwnedCameraPose;
    Node root, mainNode(&root), ownedNode(&root);
    Camera mainCamera(&mainNode), own(&ownedNode);
    mainCamera.position=Vec(230.612732,3120.27393,2399.58643);
    root.current=Vec(51212.375,0,-2622.64819);
    Check(CopyOwnedCameraPose(&own,&mainCamera,&ownedNode,&root),"owned pose accepted");
    Check(Near(own.position.x+root.current.x,mainCamera.position.x) &&
          Near(own.position.z+root.current.z,mainCamera.position.z),
          "live floating-origin shift must not displace cloned view by51km");
    Check(ownedNode.updates==1 && ownedNode.conversions==2,"one bounded freshness step before conversion");
    Check(mainCamera.writes==0 && mainNode.updates==0 && root.updates==1,"root ancestry refreshed; borrowed camera untouched");
    root.current=Vec(-80000,17,43000);
    Check(CopyOwnedCameraPose(&own,&mainCamera,&ownedNode,&root) &&
          Near(own.position.x+root.current.x,mainCamera.position.x) &&
          Near(own.position.y+root.current.y,mainCamera.position.y),"second request handles changed origin");
    unsigned writes=own.writes,updates=ownedNode.updates;
    Check(!CopyOwnedCameraPose(&mainCamera,&mainCamera,&mainNode,&root),"main camera refused");
    Check(!CopyOwnedCameraPose(&own,&mainCamera,&root,&root),"root node refused");
    Node unrelated;
    Check(!CopyOwnedCameraPose(&own,&mainCamera,&ownedNode,&unrelated),"unknown ancestry refused");
    root.parent=&unrelated;
    Check(!CopyOwnedCameraPose(&own,&mainCamera,&ownedNode,&root),"root with further ancestors refused");
    root.parent=NULL;
    Camera other(&mainNode);
    Check(!CopyOwnedCameraPose(&other,&mainCamera,&mainNode,&root),"shared main camera node refused");
    Check(own.writes==writes && ownedNode.updates==updates && mainCamera.writes==0,
          "rejected requests have no mutation");
    ownedNode.fail=true;
    bool threw=false;
    try{CopyOwnedCameraPose(&own,&mainCamera,&ownedNode,&root);}catch(const std::runtime_error&){threw=true;}
    Check(threw && own.writes==writes,"freshness failure propagates before camera writes");
    Check(!CopyOwnedCameraPose(static_cast<Camera*>(NULL),&mainCamera,&ownedNode,&root),"null owned camera refused");
    std::cout<<checks<<" production camera-pose checks passed\n";
}
