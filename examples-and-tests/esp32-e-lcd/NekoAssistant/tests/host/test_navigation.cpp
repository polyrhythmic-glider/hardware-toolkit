#include <cassert>
#include "NekoNavigation.h"
#include "NekoButton.h"
int main(){
 Page page=Page::Home;for(int i=0;i<100;++i){page=NekoNavigation::next(page);assert(page==Page::Home||page==Page::Clock);assert(!NekoNavigation::isDiagnostic(page));}
 NekoChord c;assert(!c.update(true,false,0));assert(!c.suppress());assert(!c.update(true,true,100));assert(c.suppress());
 assert(!c.update(true,true,2099));assert(c.update(true,true,2100));assert(!c.update(true,true,2200));
 assert(!c.update(false,true,2300)&&c.suppress());assert(!c.update(false,false,2400)&&!c.suppress());
 assert(!c.update(true,true,0xffffff00U));assert(c.update(true,true,0x00000700U));
}
