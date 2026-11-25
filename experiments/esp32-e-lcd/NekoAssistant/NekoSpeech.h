#pragma once
#include <algorithm>
#include <cstdio>
#include "NekoText.h"

namespace NekoSpeech {
struct Layout {
  NekoText::Line text[8], author[2];
  unsigned textCount=0,authorCount=0;
  int x=0,y=0,width=0,height=0,textBaseline=0,authorBaseline=0,spacing=28;
  bool fits=false;
};
template<class QuoteMeasure,class AuthorMeasure>
Layout layout(const char *message,const char *author,QuoteMeasure quoteMeasure,
              AuthorMeasure authorMeasure,bool large=true) {
  Layout result;
  char quoted[192];std::snprintf(quoted,sizeof(quoted),author?"«%s»":"%s",message?message:"");
  const auto text=NekoText::wrap(quoted,176,result.text,8,quoteMeasure);
  const auto attribution=NekoText::wrap(author,176,result.author,2,authorMeasure);
  result.textCount=text.count;result.authorCount=attribution.count;
  result.spacing=large?28:22;
  const int ascent=large?19:14,descent=large?5:4;
  const int textHeight=text.count?ascent+descent+(text.count-1)*result.spacing:0;
  const int authorHeight=attribution.count?14+(attribution.count-1)*17:0;
  int contentWidth=0;
  for(unsigned i=0;i<text.count;++i) contentWidth=std::max(contentWidth,static_cast<int>(quoteMeasure(result.text[i].text)));
  for(unsigned i=0;i<attribution.count;++i) contentWidth=std::max(contentWidth,static_cast<int>(authorMeasure(result.author[i].text)));
  result.width=std::max(112,contentWidth+28);
  result.height=textHeight+(authorHeight?18+authorHeight:0)+32;
  result.x=392-result.width;
  result.y=std::max(8,std::min(175-result.height/2,292-result.height));
  result.textBaseline=result.y+16+ascent;
  result.authorBaseline=result.y+16+textHeight+18+11;
  result.fits=!text.overflow&&!attribution.overflow&&result.width<=204&&result.height<=284;
  return result;
}
}
