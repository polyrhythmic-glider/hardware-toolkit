#include "NekoWeb.h"
#include "NekoWebPage.h"
#include "NekoPhrases.h"
#include "NekoWebAssets.h"
#include <ESPmDNS.h>
#include <esp_system.h>

String NekoWeb::json(const char *text) {
  String out = "\"";
  for (const unsigned char *p = reinterpret_cast<const unsigned char *>(text); *p; ++p) {
    if (*p == '"' || *p == '\\') { out += '\\'; out += static_cast<char>(*p); }
    else if (*p >= 32) out += static_cast<char>(*p);
    else { char code[7]; snprintf(code, sizeof(code), "\\u%04x", *p); out += code; }
  }
  return out + "\"";
}
bool NekoWeb::authenticate() {
  server_.sendHeader("Cache-Control", "no-store");
  if (server_.authenticate("neko", password_)) return true;
  server_.requestAuthentication(BASIC_AUTH, "Neko - password sul display", "Accedi con utente neko e password dalla pagina Connessioni di Neko."); return false;
}
void NekoWeb::begin() {
  Preferences prefs;
  if (!prefs.begin("neko_web", false)) return;
  String key = prefs.getString("key", "");
  if (key.length() != 12) {
    const char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789";
    key = ""; for (int i = 0; i < 12; ++i) key += alphabet[esp_random() % (sizeof(alphabet) - 1)];
    prefs.putString("key", key);
  }
  strlcpy(password_, key.c_str(), sizeof(password_)); prefs.end();
  snprintf(apName_, sizeof(apName_), "Neko-%04X", static_cast<unsigned>(ESP.getEfuseMac() & 0xFFFF));
  server_.on("/", HTTP_GET, [this]() {
    if (!authenticate()) return;
    server_.sendHeader("X-Content-Type-Options", "nosniff");
    server_.sendHeader("X-Frame-Options", "DENY");
    server_.send_P(200, "text/html; charset=utf-8", NEKO_WEB_PAGE);
  });
  server_.on("/api/status", HTTP_GET, [this]() { if (authenticate()) status(); });
  server_.on("/pet.png", HTTP_GET, [this]() {
    if (authenticate()) server_.send_P(200, "image/png", reinterpret_cast<const char *>(NekoWebAssets::PetPng), sizeof(NekoWebAssets::PetPng));
  });
  server_.on("/orbitron.ttf", HTTP_GET, [this]() {
    if (authenticate()) server_.send_P(200, "font/ttf", reinterpret_cast<const char *>(NekoWebAssets::OrbitronTtf), sizeof(NekoWebAssets::OrbitronTtf));
  });
  server_.on("/api/action", HTTP_POST, [this]() { if (authenticate()) action(); });
  server_.onNotFound([this]() { server_.send(404, "text/plain", "Neko: pagina non trovata"); });
  const char *headers[] = {"Origin"}; server_.collectHeaders(headers, 1);
  // WebServer needs the lwIP task even before a station has credentials.
  Serial.println("[NEKO] starting web server");
  Network.begin();
  server_.begin(); started_ = true;
  Serial.println("[NEKO] web server ready");
  if (!settings_.get().ssid[0]) openSetupWindow(600);
}
void NekoWeb::openSetupWindow(uint32_t seconds) {
  if (!started_) return;
  WiFi.mode(WIFI_AP_STA);
  if (!WiFi.softAP(apName_, password_, 1, false, 2)) return;
  Serial.println("[NEKO] Wi-Fi setup ready");
  setupUntil_ = millis() + constrain(seconds, 30UL, 600UL) * 1000UL;
  changed_ = true;
}
void NekoWeb::tick() {
  if (!started_) return;
  server_.handleClient();
  const bool connected = WiFi.status() == WL_CONNECTED;
  if (connected != wasConnected_) {
    wasConnected_ = connected; changed_ = true;
    if (connected) Serial.printf("[WIFI] connected ip=%s rssi=%d\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
    else { MDNS.end(); mdns_ = false; Serial.println("[WIFI] disconnected; reconnecting"); }
  }
  if (connected && !mdns_) { mdns_ = MDNS.begin("neko"); if (mdns_) MDNS.addService("http", "tcp", 80); }
  if (setupUntil_ && !setupOpen()) { WiFi.softAPdisconnect(true); setupUntil_ = 0; changed_ = true; }
}
void NekoWeb::status() {
  const auto &env = hardware_.environment();
  String out; out.reserve(1600);
  out = "{\"version\":\"0.7.0\",\"name\":" + json(settings_.get().catName);
  out += ",\"epoch\":" + String(static_cast<unsigned long>(time(nullptr)));
  out += ",\"battery\":" + String(hardware_.battery().percent);
  out += ",\"voltage\":" + String(hardware_.battery().voltage, 3);
  out += ",\"temperature\":" + (env.valid ? String(env.temperatureC, 1) : String("null"));
  out += ",\"humidity\":" + (env.valid ? String(env.humidityRh, 0) : String("null"));
  out += ",\"ssid\":" + json(settings_.get().ssid) + ",\"ip\":" + json(WiFi.localIP().toString().c_str());
  out += ",\"rssi\":" + String(WiFi.RSSI()) + ",\"profile\":" + String(static_cast<unsigned>(settings_.get().powerProfile));
  out += ",\"setup\":" + String(setupOpen() ? "true" : "false");
  out += ",\"uptime\":" + String(millis() / 1000) + ",\"heap\":" + String(ESP.getFreeHeap());
  out += ",\"pet_ms\":" + String(millis());
  const auto &phrase = NekoPhrases::current(millis());
  out += ",\"message\":" + json(scheduler_.notification().active ? scheduler_.notification().text : phrase.text);
  out += ",\"quote\":{\"text\":" + json(phrase.text) + ",\"author\":" + json(phrase.author) + ",\"source\":" + json(phrase.source) + "}";
  out += ",\"timers\":[";
  bool first = true;
  for (const auto &t : scheduler_.timers()) if (t.active) {
    if (!first) out += ','; first = false;
    out += "{\"id\":" + String(t.id) + ",\"label\":" + json(t.label) + ",\"seconds\":" + String(t.remainingSeconds) + ",\"paused\":" + (t.paused ? "true" : "false") + "}";
  }
  out += "],\"reminders\":["; first = true;
  for (size_t i = 0; i < scheduler_.reminders().size(); ++i) {
    const auto &r = scheduler_.reminders()[i]; if (!r.active) continue;
    if (!first) out += ','; first = false;
    out += "{\"index\":" + String(i) + ",\"text\":" + json(r.text) + ",\"epoch\":" + String(static_cast<unsigned long>(r.dueEpoch)) + "}";
  }
  out += "],\"alarms\":["; first = true;
  for (size_t i = 0; i < scheduler_.alarms().size(); ++i) {
    const auto &a = scheduler_.alarms()[i]; if (!a.enabled) continue;
    if (!first) out += ','; first = false;
    out += "{\"index\":" + String(i) + ",\"label\":" + json(a.label) + ",\"hour\":" + String(a.hour) + ",\"minute\":" + String(a.minute) + ",\"days\":" + String(a.weekdays) + "}";
  }
  out += "]}"; server_.send(200, "application/json", out);
}
void NekoWeb::action() {
  // URL-encoded POSTs from the local dashboard only; disallow cross-origin form submissions.
  if (server_.hasHeader("Origin")) {
    const String expected = "http://" + server_.hostHeader();
    if (server_.header("Origin") != expected) { server_.send(403, "application/json", "{\"error\":\"Origine non valida\"}"); return; }
  }
  const String action = server_.arg("action"); bool ok = false;
  const uint32_t id = strtoul(server_.arg("id").c_str(), nullptr, 10);
  if (action == "timer") {
    const long seconds = server_.arg("seconds").toInt();
    ok = seconds > 0 && seconds <= 604800 && scheduler_.startTimer(seconds, server_.arg("label").c_str());
    if (ok) pageRequest_ = static_cast<int>(Page::Timers);
  } else if (action == "pause") ok = scheduler_.pauseTimer(id);
  else if (action == "resume") ok = scheduler_.resumeTimer(id);
  else if (action == "cancel") ok = scheduler_.cancelTimer(id);
  else if (action == "pet") { pet_ = true; pageRequest_ = static_cast<int>(Page::Cat); ok = true; }
  else if (action == "home") { pageRequest_ = static_cast<int>(Page::Home); ok = true; }
  else if (action == "clock") { pageRequest_ = static_cast<int>(Page::Clock); ok = true; }
  else if (action == "phraseNext") { NekoPhrases::next(); ok = true; }
  else if (action == "name") {
    const String name = server_.arg("name");
    if (name.length() && name.length() < sizeof(Settings::catName)) {
      Settings value = settings_.get(); strlcpy(value.catName, name.c_str(), sizeof(value.catName)); ok = settings_.save(value);
    }
  } else if (action == "profile") {
    const int profile = server_.arg("profile").toInt();
    if (profile >= 0 && profile < 4) { Settings value = settings_.get(); value.powerProfile = static_cast<PowerProfile>(profile); ok = settings_.save(value); }
  } else if (action == "notification") ok = scheduler_.pushNotification(server_.arg("text").c_str(), 1, 300);
  else if (action == "dismiss") { scheduler_.dismissNotification(); ok = true; }
  else if (action == "reminder") {
    const long minutes = server_.arg("minutes").toInt();
    ok = minutes > 0 && minutes <= 10080 && scheduler_.addReminder(time(nullptr) + minutes * 60, server_.arg("text").c_str());
  } else if (action == "removeReminder") ok = scheduler_.removeReminder(id);
  else if (action == "alarm") {
    AlarmSlot a{}; const int hour = server_.arg("hour").toInt(), minute = server_.arg("minute").toInt(), days = server_.arg("days").toInt();
    if (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && days > 0 && days <= 127 && id < 8) {
      a.enabled = server_.arg("enabled") != "0"; a.hour = hour; a.minute = minute; a.weekdays = days;
      strlcpy(a.label, server_.arg("label").c_str(), sizeof(a.label)); ok = scheduler_.setAlarm(id, a);
    }
  } else if (action == "time") {
    const time_t epoch = strtoul(server_.arg("epoch").c_str(), nullptr, 10);
    if (epoch >= 1700000000 && epoch <= 4102444800LL) { timeval tv{epoch, 0}; settimeofday(&tv, nullptr); hardware_.writeRtc(epoch); ok = true; }
  } else if (action == "wifi" && setupOpen()) ok = connectivity_.configureWifi(server_.arg("ssid").c_str(), server_.arg("password").c_str());
  changed_ = true;
  server_.send(ok ? 200 : 400, "application/json", ok ? "{\"ok\":true}" : "{\"error\":\"Controlla i valori, gli slot liberi e l'ora di Neko. Per il Wi-Fi apri prima la configurazione con KEY.\"}");
}
