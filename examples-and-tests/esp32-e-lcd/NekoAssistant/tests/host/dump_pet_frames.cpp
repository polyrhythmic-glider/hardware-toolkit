#include <array>
#include <cstdio>
#include "NekoPet.h"
#include "NekoPetAssets.h"
int main(){
 std::array<uint8_t,3200> out{};
 for(unsigned pose=0;pose<4;++pose)for(unsigned tick=0;tick<100;++tick){
  const auto p=NekoPet::sample(static_cast<NekoPet::Pose>(pose),tick*150);
  NekoPet::raster(NekoPetAssets::Frame160_0,160,p,out.data());uint32_t hash=2166136261U;for(auto b:out)hash=(hash^b)*16777619U;
  std::printf("%u,%u,%u,%d,%u,%u\n",p.blink,p.mouth,p.bob,p.tail,p.happy,hash);
 }
}
