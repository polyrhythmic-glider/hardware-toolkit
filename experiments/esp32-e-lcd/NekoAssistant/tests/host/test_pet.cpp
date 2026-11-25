#include <array>
#include <cassert>
#include <cstring>
#include <set>
#include "NekoPet.h"
#include "NekoPetAssets.h"
#include "NekoClockAssets.h"
int main(){
 static_assert(NekoPetAssets::Count==1);
 for(unsigned pose=0;pose<4;++pose){
  std::set<int> motion;
  for(uint32_t ms=0;ms<12000;ms+=150){auto p=NekoPet::sample(static_cast<NekoPet::Pose>(pose),ms);assert(p.blink<=2&&p.bob<=1&&p.tail>=-2&&p.tail<=2);motion.insert(p.bob*10+p.tail);}
  assert(motion.size()>=3);
 }
 std::array<uint8_t,3200> out{};
 NekoPet::raster(NekoPetAssets::Frame160_0,160,{0,0,0,0,false},out.data());
 assert(!std::memcmp(out.data(),NekoPetAssets::Frame160_0,out.size()));
 for(unsigned pose=0;pose<4;++pose)for(uint32_t ms=0;ms<9000;ms+=150){
  NekoPet::raster(NekoPetAssets::Frame160_0,160,NekoPet::sample(static_cast<NekoPet::Pose>(pose),ms),out.data());
  for(int y=0;y<160;++y)for(int x=0;x<160;++x){
   const bool effects=(x<45&&y>=87&&y<145)||(x>=63&&x<=101&&y>=53&&y<=81)||(x>=116&&x<=140&&y>=43&&y<=72)||NekoPet::ellipse(x,y,112,84,3,2)||(x>=98&&x<106&&y>=23&&y<31);
   if(!effects)assert((out[y*20+x/8]&(1<<(x%8)))==(NekoPetAssets::Frame160_0[y*20+x/8]&(1<<(x%8))));
  }
 }
 // Half-closed lids cover the top of the eye without squashing the remaining
 // pupil or highlights. Full closure has a curved profile, not a flat dash.
 NekoPet::raster(NekoPetAssets::Frame160_0,160,{1,0,0,0,false},out.data());
 for(int y=73;y<=80;++y)for(int x=73;x<=96;++x)
  assert((out[y*20+x/8]&(1<<(x%8)))==(NekoPetAssets::Frame160_0[y*20+x/8]&(1<<(x%8))));
 for(int y=54;y<=62;++y)for(int x=73;x<=93;++x)assert(!(out[y*20+x/8]&(1<<(x%8))));
 NekoPet::raster(NekoPetAssets::Frame160_0,160,{2,0,0,0,false},out.data());
 int low=160,high=0;
 for(int y=53;y<=81;++y)for(int x=67;x<=97;++x)if(out[y*20+x/8]&(1<<(x%8))){if(y<low)low=y;if(y>high)high=y;}
 assert(high-low>=4);
 static_assert(sizeof(NekoPetAssets::Frame192_0)==192*192/8);
 static_assert(sizeof(NekoPetAssets::Frame224_0)==224*224/8);
 static_assert(sizeof(NekoClockAssets::Large0)==80*172/8);
}
