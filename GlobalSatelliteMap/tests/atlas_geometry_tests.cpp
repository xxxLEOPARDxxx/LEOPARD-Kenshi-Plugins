#include "../SatellitePolicy.h"
#include <cstdio>
namespace Ogre{struct Vector3{float x,y,z;Vector3(float X,float Y,float Z):x(X),y(Y),z(Z){}};}
namespace MyGUI{struct IntPoint{int left,top;IntPoint(int x=0,int y=0):left(x),top(y){}};struct ImageBox{int w,h;int getWidth(){return w;}int getHeight(){return h;}};}
struct MapScreen{MyGUI::ImageBox*mapImage;void*mapScrollView;bool flipped;MyGUI::IntPoint worldToMapCoords(const Ogre::Vector3&v){return MyGUI::IntPoint(int(std::floor((v.x+147456)*mapImage->w/294912+.5)),int(std::floor(((flipped?-v.z:v.z)+147456)*mapImage->h/294912+.5)));}};
struct Bounds{double minX,minZ,maxX,maxZ;Bounds():minX(-147456),minZ(-147456),maxX(147456),maxZ(147456){}}manifest;
#ifndef GEOMETRY_HEADER
#define GEOMETRY_HEADER "../build/ExtractedGeometry.h"
#endif
#include GEOMETRY_HEADER
int main(){MyGUI::ImageBox image={4096,3072};MapScreen map={&image,&image,false};GSM::Rect tiles[16];if(!Geometry(&map,tiles))return 1;for(int z=0;z<4;++z)for(int x=0;x<4;++x)if(tiles[z*4+x].x!=x*1024||tiles[z*4+x].y!=z*768||tiles[z*4+x].w!=1024||tiles[z*4+x].h!=768){std::fprintf(stderr,"FAIL native outer boundary exactness\n");return 1;}map.flipped=true;if(Geometry(&map,tiles))return 1;std::puts("GST2 extracted geometry PASS17checks; exact native global boundaries and no-flip guard");return 0;}
