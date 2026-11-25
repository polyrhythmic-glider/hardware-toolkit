#include <cassert>
#include <cstdio>
#include <set>
#include <string>
#include "u8g2.h"
#include "NekoSpeech.h"
#include "NekoPhrases.h"
int main(){u8g2_t u{};std::set<int> heights,widths;
 for(auto q:NekoPhrases::Entries){
  auto quoteMeasure=[&](const char*s){u8g2_SetFont(&u,u8g2_font_helvB18_tf);return u8g2_GetUTF8Width(&u,s);};
  auto authorMeasure=[&](const char*s){u8g2_SetFont(&u,u8g2_font_helvR10_tf);return u8g2_GetUTF8Width(&u,s);};
  const auto box=NekoSpeech::layout(q.text,q.author,quoteMeasure,authorMeasure);
  if(!box.fits)std::fprintf(stderr,"Too large: %s [%u lines, %d px]\n",q.text,box.textCount,box.height);
  assert(box.fits);heights.insert(box.height);widths.insert(box.width);
  assert(box.x>=188&&box.x+box.width==392&&box.y>=8&&box.y+box.height<=292);
  // The frame hugs the content: 16 px top/bottom, plus an 18 px author gap.
  assert(box.height==24+static_cast<int>(box.textCount-1)*28+18+14+static_cast<int>(box.authorCount-1)*17+32);
  assert(box.width>=112&&box.width<=204);
  std::string reconstructed;
  for(unsigned i=0;i<box.textCount;++i){assert(quoteMeasure(box.text[i].text)<=box.width-28);if(i)reconstructed+=' ';reconstructed+=box.text[i].text;}
  if(reconstructed!="«"+std::string(q.text)+"»")std::fprintf(stderr,"WRAP: %s => %s\n",q.text,reconstructed.c_str());
  assert(reconstructed=="«"+std::string(q.text)+"»");
  reconstructed.clear();
  for(unsigned i=0;i<box.authorCount;++i){assert(authorMeasure(box.author[i].text)<=box.width-28);if(i)reconstructed+=' ';reconstructed+=box.author[i].text;}
  assert(reconstructed==q.author);
  assert(box.authorBaseline+static_cast<int>(box.authorCount-1)*17+3<=box.y+box.height-16);
  // Every non-ASCII glyph must actually exist in the font, not disappear.
  for(const char *str:{q.text,q.author}){u8x8_t utf{};u8x8_utf8_init(&utf);for(const unsigned char*p=reinterpret_cast<const unsigned char*>(str);*p;++p){const auto code=u8x8_utf8_next(&utf,*p);if(code<0xfffe){u8g2_SetFont(&u,str==q.text?u8g2_font_helvB18_tf:u8g2_font_helvR10_tf);assert(u8g2_IsGlyph(&u,code));}}}
 }
 assert(heights.size()>=4&&widths.size()>=4);
 std::printf("All %zu quotes fit at full size, with complete authors, supported glyphs and adaptive speech bubbles.\n",NekoPhrases::Count);
}
