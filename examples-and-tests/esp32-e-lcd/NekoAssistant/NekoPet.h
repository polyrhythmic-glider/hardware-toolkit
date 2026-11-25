#pragma once
#include <cstdint>
#include <cstring>
namespace NekoPet {
enum class Pose : uint8_t { Idle, Happy, Sleepy, Alert };
struct Sample { uint8_t blink, mouth, bob; int8_t tail; bool happy; };
constexpr uint32_t FrameMs = 150;
inline Sample sample(Pose pose, uint32_t now) {
  const uint32_t tick = now / FrameMs;
  const unsigned phase = tick % 48;
  const int8_t wave[] = {0,1,1,2,2,1,1,0,0,-1,-1,-2,-2,-1,-1,0};
  const uint8_t breath[] = {0,0,1,1,1,1,0,0};
  uint8_t blink = phase == 12 || phase == 14 ? 1 : phase == 13 ? 2 : 0;
  if (pose == Pose::Sleepy) blink = tick % 12 < 10 ? 2 : 1;
  if (pose == Pose::Happy) blink = tick % 10 < 6 ? 2 : 0;
  return {blink, static_cast<uint8_t>(pose != Pose::Sleepy && ((phase >= 30 && phase < 34) || (phase >= 36 && phase < 38))),
          breath[(tick / 2) % 8], wave[(tick / 2) % 16], pose == Pose::Happy};
}
inline bool ellipse(int x,int y,int cx,int cy,int rx,int ry) {
  const int dx=x-cx,dy=y-cy; return dx*dx*ry*ry+dy*dy*rx*rx<=rx*rx*ry*ry;
}
inline bool eyeRegion(int x,int y,bool right) {
  return right ? y>=43&&y<=72&&ellipse(x,y,128,58,12,15)
               : y>=53&&y<=81&&ellipse(x,y,82,67,19,15);
}
inline int eyelidY(int x,bool right,bool closed) {
  const int cx=right?128:82,rx=right?10:17;
  const int t=(x-cx+rx)*256/(2*rx),u=256-t;
  const int left=right?(closed?58:55):(closed?66:64);
  const int control=right?(closed?68:61):(closed?80:71);
  const int end=right?(closed?57:54):(closed?66:63);
  return (u*u*left+2*u*t*control+t*t*end)/65536;
}
// Rendering effects on ONE immutable character. Body, markings, collar and face
// proportions never morph into a separately generated pose.
inline void raster(const uint8_t *base, unsigned size, Sample motion, uint8_t *out) {
  const unsigned stride=size/8;
  std::memset(out,0,size*stride);
  const auto sourceInk=[&](int x,int y) {
    return x>=0&&y>=0&&x<static_cast<int>(size)&&y<static_cast<int>(size)&&
      (base[y*stride+x/8]&(1U<<(x%8)));
  };
  for (unsigned y=0;y<size;++y) for (unsigned x=0;x<size;++x) {
    const int nx=x*160/size,ny=y*160/size;
    int sx=x,sy=y;
    if (nx<45&&ny>=87&&ny<145) {
      // Taper to zero at the attachment, rather than shearing the tail root.
      const int weight=(45-nx)*(145-ny);
      sx-=motion.tail*weight*static_cast<int>(size)/(32*58*160);
      sy-=motion.tail*weight*static_cast<int>(size)/(45*58*160);
    }
    bool ink=sourceInk(sx,sy);
    for(unsigned eyeIndex=0;eyeIndex<2;++eyeIndex) {
      const bool right=eyeIndex!=0;
      if(!motion.blink||!eyeRegion(nx,ny,right)) continue;
      const int cx=right?128:82,rx=right?10:17;
      const bool within=nx>=cx-rx&&nx<=cx+rx;
      const int lid=eyelidY(nx,right,motion.blink==2);
      const bool outer=right?nx>cx+rx-4:nx<cx-rx+4;
      const bool stroke=within&&ny>=lid-(outer?2:0)&&ny<=lid+1;
      // A curved lid covers the original pupil; iris and highlights below it
      // retain their exact shape. Closed lids have their own curved lash art.
      ink=motion.blink==2?stroke:stroke||(within&&ny>lid+1&&sourceInk(x,y));
    }
    // A small lower-lip opening preserves the nose, smile and whiskers.
    if (motion.mouth&&ellipse(nx,ny,112,84,3,2)) ink=true;
    if (motion.happy&&nx>=98&&nx<106&&ny>=23&&ny<31) {
      const uint8_t heart[]={0x66,0xff,0xff,0xff,0x7e,0x3c,0x18,0};
      ink=ink||(heart[ny-23]&(1U<<(nx-98)));
    }
    if (ink) out[y*stride+x/8]|=1U<<(x%8);
  }
}
}
