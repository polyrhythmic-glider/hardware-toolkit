#pragma once
#include "NekoTypes.h"
namespace NekoNavigation {
constexpr Page DiagnosticPages[] = {Page::Environment,Page::Battery,Page::Connectivity,Page::Storage,Page::Audio,Page::Settings,Page::Diagnostics};
constexpr const char *Labels[] = {"Ambiente","Batteria","Connessioni","Memoria","Audio","Preferenze","Diagnostica","Esci"};
constexpr unsigned Count = 8;
inline bool isDiagnostic(Page page) {
  if (page==Page::DiagnosticsMenu) return true;
  for (auto p:DiagnosticPages) if (p==page) return true;
  return false;
}
inline Page next(Page page) {return page==Page::Clock?Page::Home:page==Page::Home||page==Page::Cat?Page::Clock:Page::Home;}
}
