// Local ABI measurement only. This executable never opens/calls the game.
#define private public
#define protected public
#include <map>
#include <string>
#include <kenshi/util/hand.h>
#include "XpInfoCore.h"
#undef protected
#undef private
#include <cstdio>
#include <cstddef>
const size_t kCount=37;
struct Snapshot {
 XpInfoCore::Skill skills[kCount];float* refs[kCount];bool live,pending;
};
typedef std::map<hand,Snapshot> Registry;
typedef std::map<int,std::map<std::string,void*> > PanelMap;
typedef std::map<std::string,void*> LineMap;
int main(){
 printf("hand=%u skill=%u snapshot=%u refs=%u live=%u pending=%u\n",
  sizeof(hand),sizeof(XpInfoCore::Skill),sizeof(Snapshot),offsetof(Snapshot,refs),offsetof(Snapshot,live),offsetof(Snapshot,pending));
 printf("registry size=%u head=%u count=%u nodeValue=%u snapshotWithinValue=%u\n",
  sizeof(Registry),offsetof(Registry,_Myhead),offsetof(Registry,_Mysize),offsetof(Registry::_Node,_Myval),offsetof(Registry::value_type,second));
 printf("panelMap nodeValue=%u innerWithinValue=%u lineMap nodeValue=%u lineWithinValue=%u\n",
  offsetof(PanelMap::_Node,_Myval),offsetof(PanelMap::value_type,second),offsetof(LineMap::_Node,_Myval),offsetof(LineMap::value_type,second));
 printf("string size=%u buffer=%u length=%u capacity=%u\n",sizeof(std::string),offsetof(std::string,_Bx),offsetof(std::string,_Mysize),offsetof(std::string,_Myres));
}
