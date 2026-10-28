/* WardriveHub C5 Mini v1.7.0 Optimized - Standalone-only architecture
   Target: XIAO ESP32-C5 on the user's existing Piglet carrier.
   Original v1.3.4 GPIOs retained; VERIFY the carrier's actual GPS wiring.
   Libraries: ESP32 Arduino 3.3.12, TinyGPSPlus, Adafruit GFX,
              Adafruit SSD1306, NimBLE-Arduino.
   No Core, ESP-NOW, Android GATT, or node roles.
   Source release; see VALIDATION.md for build checks and remaining hardware tests.
*/
#include <Arduino.h>
#include "ScanHelpers.h"
struct MiniHistoryState;

// Explicit Arduino entry points; prevent generated-prototype ambiguity.
void setup();
void loop();
void stopMiniSweep();
void serviceWifi();
#include <WiFi.h>
#include <esp_wifi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Preferences.h>
#include <TinyGPSPlus.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NimBLEDevice.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <esp_system.h>
#include <esp_heap_caps.h>

static constexpr int OLED_SDA=23,OLED_SCL=24,GPS_RX=12,GPS_TX=11;
static constexpr int BUTTON_PIN=0,SD_CS=7,SD_MOSI=10,SD_MISO=9,SD_SCK=8;
static constexpr uint32_t GPS_MAX_AGE=12000,UI_PERIOD=750,SD_FLUSH_MS=3000;
static constexpr uint32_t SCAN_PERIOD=150; // time between complete channel sweeps
uint16_t wifiDwellMs=75; // fixed 75/100/125 or adaptive 75..125
bool smartAggressive=false; // persisted scan profile, fixed 75 ms by default
// Boot is GPS-gated: do not arm capture or start either radio scanner until
// fresh position AND UTC are available. A manual STOP cancels pending autostart.
bool gpsStartPending=true;
bool portalActive=false;
String portalPassword;
WebServer portal(80);
DNSServer portalDns;
File uploadCsv;
String uploadName;
String cfgSsid,cfgPassword,cfgWigle,cfgWdg,cfgSignalFrontier,cfgSignalFrontierHost="signalfrontier.org",uploadReport="No uploads yet";
bool uploadBusy=false;
uint32_t portalStarted=0;
static constexpr uint32_t PORTAL_IDLE_TIMEOUT=60000; // 60s without a connected client
Adafruit_SSD1306 display(128,64,&Wire,-1);
HardwareSerial gpsUart(1);
TinyGPSPlus gps;
SPIClass sdSPI(FSPI);
Preferences settings;
File csv;
bool oledReady=false,sdReady=false,sessionArmed=false,paused=false,bleReady=false;
bool dirty=true;
uint32_t lastUi=0,lastFlush=0,lastScan=0,lastBle=0,lastGpsDiag=0;
uint32_t wifiTotal=0,bleTotal=0,wifiUnique=0,bleUnique=0,sdErrors=0;
static uint32_t miniCsvBuildErrors=0;
static bool miniCsvWriteBlocked=false;
uint32_t wifi24Unique=0,wifi5Unique=0,dedupTableOverflows=0;
uint32_t sessionStart=0;
uint8_t screen=0; // dashboard, GPS, WiFi/BLE, storage
String csvPath,lastSSID;
int lastRssi=0;
static constexpr size_t SEEN_SIZE=1024;
uint32_t wifiSeen[SEEN_SIZE]={},bleSeen[SEEN_SIZE]={};
uint32_t wifi24Seen[SEEN_SIZE]={},wifi5Seen[SEEN_SIZE]={},wifiLogged[SEEN_SIZE]={},bleLogged[SEEN_SIZE]={};
uint32_t hashId(const String &s){uint32_t h=2166136261UL;for(size_t i=0;i<s.length();++i){h^=(uint8_t)s[i];h*=16777619UL;}return h?h:1;}
struct MiniHistoryState {const uint32_t* table;uint16_t used;bool sorted;};
static MiniHistoryState miniHistory[]={
  {wifiSeen,0,false},{bleSeen,0,false},{wifi24Seen,0,false},
  {wifi5Seen,0,false},{wifiLogged,0,false},{bleLogged,0,false}};
static MiniHistoryState* miniHistoryFor(const uint32_t* table){
  for(auto& state:miniHistory)if(state.table==table)return &state;
  return nullptr;
}
static void resetMiniHistoryState(){for(auto& state:miniHistory){state.used=0;state.sorted=false;}}
bool hasSeen(const uint32_t* table,uint32_t h){
  MiniHistoryState* state=miniHistoryFor(table);
  if(state && state->sorted)return wdh::containsSortedHash(table,SEEN_SIZE,h);
  size_t i=h%SEEN_SIZE;
  for(size_t n=0;n<SEEN_SIZE;++n){size_t j=(i+n)%SEEN_SIZE;if(table[j]==h)return true;if(!table[j])return false;}
  return false;
}
bool markUnique(uint32_t* table,uint32_t h){
  MiniHistoryState* state=miniHistoryFor(table);
  if(state && state->sorted){if(!wdh::containsSortedHash(table,SEEN_SIZE,h))dedupTableOverflows++;return false;}
  size_t i=h%SEEN_SIZE;
  for(size_t n=0;n<SEEN_SIZE;++n){
    size_t j=(i+n)%SEEN_SIZE;
    if(table[j]==h)return false;
    if(!table[j]){
      table[j]=h;
      if(state && ++state->used==SEEN_SIZE){wdh::sortHashes(table,SEEN_SIZE);state->sorted=true;}
      return true;
    }
  }
  dedupTableOverflows++;return false;
}
bool gpsFresh(){return gps.location.isValid() && gps.location.age()<GPS_MAX_AGE;}
bool gpsTimeFresh(){return gps.date.isValid() && gps.time.isValid() && gps.date.age()<GPS_MAX_AGE && gps.time.age()<GPS_MAX_AGE;}
void serviceGps(){while(gpsUart.available())gps.encode((char)gpsUart.read());}
String cleanCsv(String s){s.replace(',',' ');s.replace('\r',' ');s.replace('\n',' ');s.replace('"',' ');return s;}
String stamp(){if(!gpsTimeFresh()||!gpsFresh())return "";char s[25];snprintf(s,sizeof(s),"%04d-%02d-%02dT%02d:%02d:%02dZ",gps.date.year(),gps.date.month(),gps.date.day(),gps.time.hour(),gps.time.minute(),gps.time.second());return String(s);}
void loadUploadConfig(){
  cfgSsid="";cfgPassword="";cfgWigle="";cfgWdg="";cfgSignalFrontier="";cfgSignalFrontierHost="signalfrontier.org";
  if(!sdReady)return;File f=SD.open("/uploadcfg.txt",FILE_READ);if(!f)return;
  while(f.available()){String line=f.readStringUntil('\n');line.trim();if(line.length()>=3&&(uint8_t)line[0]==0xEF&&(uint8_t)line[1]==0xBB&&(uint8_t)line[2]==0xBF)line.remove(0,3);if(!line.length()||line.startsWith("#"))continue;int eq=line.indexOf('=');if(eq<1)continue;String k=line.substring(0,eq),v=line.substring(eq+1);k.trim();k.toUpperCase();v.trim();
    if(k=="WIFI_SSID")cfgSsid=v;else if(k=="WIFI_PASSWORD")cfgPassword=v;else if(k=="WIGLE_API_KEY")cfgWigle=v;else if(k=="WDGWARS_API_KEY"||k=="WDGWARS_KEY")cfgWdg=v;else if(k=="SIGNAL_FRONTIER_API_KEY")cfgSignalFrontier=v;else if(k=="SIGNAL_FRONTIER_HOST")cfgSignalFrontierHost=v;
  }f.close();
}
void closeCsv(){if(csv){csv.flush();csv.close();}csvPath="";}
// New file per boot/session. No append to an existing session after power loss.
bool openCsv(){if(!sdReady)return false;if(csv)return true;String base="/WDH_"+String((unsigned long)millis());for(unsigned n=0;n<1000;++n){String p=base+"_"+String(n)+".csv";if(SD.exists(p))continue;csv=SD.open(p,FILE_WRITE);if(!csv){sdErrors++;return false;}csvPath=p;csv.println("WigleWifi-1.6,appRelease=WardriveHub-Mini-1.7.0,model=ESP32-C5,release=1.7.0,device=WardriveHub-C5-Mini");csv.println("MAC,SSID,AuthMode,FirstSeen,Channel,Frequency,RSSI,CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,RCOIs,MfgrId,Type");csv.flush();return true;}sdErrors++;return false;}
bool logRecord(const String& mac,const String& name,const String& auth,int ch,int rssi,bool ble){
  if(!sessionArmed || paused || miniCsvWriteBlocked || !gpsFresh() || !gpsTimeFresh())return false;
  if(!openCsv()){dirty=true;return false;}
  const int freq=ble?0:((ch<=14)?2407+ch*5:5000+ch*5);
  String line;
  bool built=line.reserve(256) && line.concat(cleanCsv(mac)) && line.concat(",") &&
    line.concat(cleanCsv(name)) && line.concat(",") && line.concat(cleanCsv(auth)) && line.concat(",") &&
    line.concat(stamp()) && line.concat(",") && line.concat(String(ch)) && line.concat(",") &&
    line.concat(String(freq)) && line.concat(",") && line.concat(String(rssi)) && line.concat(",") &&
    line.concat(String(gps.location.lat(),7)) && line.concat(",") && line.concat(String(gps.location.lng(),7)) && line.concat(",") &&
    line.concat(String(gps.altitude.isValid()?gps.altitude.meters():0,1)) && line.concat(",") &&
    line.concat(String(gps.hdop.isValid()?gps.hdop.hdop():99.9,1)) && line.concat(",,0,") && line.concat(ble?"BLE":"WIFI");
  if(!built){miniCsvBuildErrors++;return false;}
  const size_t wrote=csv.println(line);
  if(wrote!=line.length()+2){
    sdErrors++;miniCsvWriteBlocked=true;dirty=true;csv.flush();csv.close();
    return false; // preserve the ambiguous tail; explicit new session starts a new file
  }
  return true;
}
// BLE callbacks run in a separate task: queue bounded records, write SD in loop().
struct BlePending { char mac[20]; char name[48]; int rssi; };
static BlePending bleQueue[24];
static uint8_t bleHead=0,bleTail=0;
static portMUX_TYPE bleMux=portMUX_INITIALIZER_UNLOCKED;
static uint32_t bleQueueDrops=0;
class BleObserver:public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* d) override {
    if(!d||!sessionArmed||paused||!gpsFresh()||!gpsTimeFresh())return;
    BlePending item={};
    snprintf(item.mac,sizeof(item.mac),"%s",d->getAddress().toString().c_str());
    if(d->haveName())snprintf(item.name,sizeof(item.name),"%s",d->getName().c_str());
    item.rssi=d->getRSSI();
    portENTER_CRITICAL(&bleMux);
    uint8_t next=(uint8_t)((bleHead+1)%24);
    if(next==bleTail)bleQueueDrops++;
    else {bleQueue[bleHead]=item;bleHead=next;}
    portEXIT_CRITICAL(&bleMux);
  }
};
void serviceBleQueue(){
  for(int k=0;k<8;k++){
    BlePending item={};bool available=false;
    portENTER_CRITICAL(&bleMux);
    if(bleTail!=bleHead){item=bleQueue[bleTail];bleTail=(uint8_t)((bleTail+1)%24);available=true;}
    portEXIT_CRITICAL(&bleMux);
    if(!available)break;
    if(!sessionArmed||paused||!gpsFresh()||!gpsTimeFresh())continue;
    bleTotal++;
    uint32_t bleId=hashId(String(item.mac));
    if(markUnique(bleSeen,bleId))bleUnique++;
    if(!hasSeen(bleLogged,bleId) && logRecord(item.mac,item.name,"[BLE]",0,item.rssi,true)) markUnique(bleLogged,bleId);
    dirty=true;
  }
}
BleObserver bleObserver;
static bool miniBleActive=false;
static uint32_t miniBleStartedMs=0,miniBleStartFailures=0;
void initBle(){if(bleReady)return;NimBLEDevice::init("WDH-Mini-Standalone");NimBLEScan* s=NimBLEDevice::getScan();s->setScanCallbacks(&bleObserver,true);s->setActiveScan(false);s->setInterval(160);s->setWindow(80);s->setMaxResults(0);bleReady=true;}
void startSession(){
  if(!gpsFresh() || !gpsTimeFresh()) { gpsStartPending=true; dirty=true; return; }
  gpsStartPending=false;
  closeCsv();miniCsvWriteBlocked=false;memset(wifiSeen,0,sizeof(wifiSeen));memset(bleSeen,0,sizeof(bleSeen));wifiTotal=bleTotal=wifiUnique=bleUnique=wifi24Unique=wifi5Unique=0;memset(wifi24Seen,0,sizeof(wifi24Seen));memset(wifi5Seen,0,sizeof(wifi5Seen));memset(wifiLogged,0,sizeof(wifiLogged));memset(bleLogged,0,sizeof(bleLogged));resetMiniHistoryState();portENTER_CRITICAL(&bleMux);bleTail=bleHead;portEXIT_CRITICAL(&bleMux);sessionStart=millis();sessionArmed=true;paused=false;settings.putBool("armed",true);dirty=true;}
void stopSession(){gpsStartPending=false;sessionArmed=false;paused=false;settings.putBool("armed",false);closeCsv();dirty=true;}

// Four uncluttered OLED views. Each row is 8 pixels tall, within 128x64.
// SSD1306 uses a framebuffer; send it only when visible content changes.
static uint8_t previousFrame[1024];
static bool previousFrameValid=false;
// Restored from the original v1.3.4 Mini startup artwork.
void splash(){
  if(!oledReady)return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.fillTriangle(2,3,2,13,11,8,SSD1306_WHITE);
  display.drawFastHLine(0,15,128,SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(10,18); display.print("WARDRIVE");
  display.setCursor(40,34); display.print("HUB");
  display.setTextSize(1);
  display.setCursor(22,51); display.print("MINI - TR1P");
  display.display();
  delay(1600);
}

// Compact counter so neither large value crosses into the other column.
String largeCount(uint32_t n){
  if(n<10000)return String(n);
  if(n<100000)return String(n/1000)+"."+String((n%1000)/100)+"k";
  if(n<1000000)return String(n/1000)+"k";
  return String(n/1000000)+"M";
}
// Persistent top-right session glyph: PLAY / PAUSE / STOP / GPS WAIT.
void drawSessionIcon(){
  const int x=115,y=0;
  if(!sessionArmed){
    display.drawRect(x+2,y+1,8,8,SSD1306_WHITE); // stopped
  }else if(paused){
    display.fillRect(x+2,y+1,3,8,SSD1306_WHITE);
    display.fillRect(x+7,y+1,3,8,SSD1306_WHITE);
  }else if(!gpsFresh()||!gpsTimeFresh()){
    display.drawTriangle(x+1,y+1,x+1,y+9,x+9,y+5,SSD1306_WHITE);
    display.drawPixel(x+11,y+2,SSD1306_WHITE); // awaiting GPS
    display.drawPixel(x+11,y+5,SSD1306_WHITE);
    display.drawPixel(x+11,y+8,SSD1306_WHITE);
  }else{
    display.fillTriangle(x+1,y+1,x+1,y+9,x+10,y+5,SSD1306_WHITE);
  }
}
// Clean four-page OLED layout: short headings, large primary values, no repeated labels.
void bigValue(int x,int y,const String &v){
  display.setTextSize(2);
  display.setCursor(x,y);
  display.print(v);
  display.setTextSize(1);
}
// A 64-pixel column fits five 6px glyphs at size 2, but leave
// margin for the divider. Above four digits, use size 1 with k/M
// notation so adjacent columns never collide.
void scannerCount(int x,int y,uint32_t n){
  String v=String(n);
  int size=(v.length()<=4)?2:1;
  if(size==1)v=largeCount(n);
  display.setTextSize(size);
  display.setCursor(x,y);
  display.print(v);
  display.setTextSize(1);
}
// Fit counters into their allocated pixel width without colliding.
// Keep full precision while it fits; abbreviate only for large values.
void compactCount(int x,int y,uint32_t n,int width,int preferredSize){
  String v=String(n);
  int sz=preferredSize;
  if((int)v.length()*6*sz>width){
    if(sz==2)sz=1;
    if((int)v.length()*6*sz>width)v=largeCount(n);
    if((int)v.length()*6*sz>width){
      if(n>=1000000)v=String(n/1000000)+"M";
      else if(n>=1000)v=String(n/1000)+"k";
    }
  }
  display.setTextSize(sz);
  display.setCursor(x,y);
  display.print(v);
  display.setTextSize(1);
}
void uiLabel(int x,int y,const String &v){display.setCursor(x,y);display.print(v);}
void draw(){
  if(!oledReady)return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  static const char* titles[]={"DASHBOARD","GPS","SCANNER","STORAGE"};
  uiLabel(0,1,titles[screen]);
  drawSessionIcon();
  display.drawFastHLine(0,12,128,SSD1306_WHITE);
  if(portalActive){
    uiLabel(0,17,"AP: WardriveHub-Mini");
    uiLabel(0,30,"PASSWORD:");
    uiLabel(0,41,portalPassword);
    uiLabel(0,54,"192.168.4.1");
    display.display();previousFrameValid=false;dirty=false;return;
  }
  if(screen==0){
    // The main view: independent Wi-Fi band counts stacked on the left,
    // BLE on the right. Reserve the entire bottom row for state and SD.
    display.drawFastVLine(64,15,36,SSD1306_WHITE);
    uiLabel(0,15,"2.4G");
    compactCount(29,14,wifi24Unique,34,1);
    display.drawFastHLine(0,32,62,SSD1306_WHITE);
    uiLabel(0,36,"5G");
    compactCount(29,35,wifi5Unique,34,1);
    uiLabel(72,16,"BLE");
    compactCount(72,29,bleUnique,55,2);
    display.drawFastHLine(0,52,128,SSD1306_WHITE);
    const char* state=miniCsvWriteBlocked?"SD WRITE ERR":gpsStartPending?(!gpsFresh()?"WAIT GPS":"WAIT UTC"):!sessionArmed?"STOP":paused?"PAUSE":!gpsFresh()?"GPS LOST":!gpsTimeFresh()?"UTC LOST":portalActive?"PORTAL":"REC";
    uiLabel(0,55,state);
    uiLabel(84,55,sdReady&&!miniCsvWriteBlocked?"SD OK":"SD ERR");
  }else if(screen==1){
    // GPS essentials only: fix, satellites, speed and timestamp readiness.
    uiLabel(0,16,gpsFresh()?"GPS LOCK":"NO GPS FIX");
    uiLabel(0,29,"SAT");
    bigValue(25,25,String(gps.satellites.isValid()?gps.satellites.value():0));
    uiLabel(67,29,"MPH");
    bigValue(67,36,gpsFresh()&&gps.speed.isValid()?String(gps.speed.mph(),0):"--");
    uiLabel(0,54,gpsTimeFresh()?"TIME OK":"TIME WAIT");
  }else if(screen==2){
    // Large band and BLE counts; same meaning as the dashboard.
    display.drawFastVLine(64,15,49,SSD1306_WHITE);
    uiLabel(0,15,"2.4 GHz");
    scannerCount(0,23,wifi24Unique);
    uiLabel(0,40,"5 GHz");
    scannerCount(0,48,wifi5Unique);
    uiLabel(70,18,"BLE");
    scannerCount(70,32,bleUnique);
  }else{
    uiLabel(0,17,sdReady?"SD READY":"SD ERROR");
    uiLabel(0,30,"CSV");
    bigValue(0,36,miniCsvWriteBlocked?"WRITE ERR":!sdReady?"NO SD":portalActive?"PORTAL":gpsStartPending?"WAIT GPS":!sessionArmed?"STOP":paused?"PAUSE":gpsFresh()&&gpsTimeFresh()?"READY":"WAIT GPS");
    uiLabel(0,55,portalActive?"PORTAL OPEN":("ERRORS "+String(sdErrors)));
  }
  uint8_t* current=display.getBuffer();
  if(!previousFrameValid||memcmp(previousFrame,current,sizeof(previousFrame))!=0){
    display.display();
    memcpy(previousFrame,current,sizeof(previousFrame));
    previousFrameValid=true;
  }
  dirty=false;
}
void togglePortal();
// GPIO0 is also the ESP32-C5 BOOT strapping pin. Ignore unstable boot/DTR
// transitions, require a clean released level, then debounce both edges.
void serviceButton(){
  static bool stableHigh=true, rawHigh=true, ready=false;
  static uint32_t changedAt=0, downAt=0;
  const uint32_t now=millis();
  bool raw=digitalRead(BUTTON_PIN)==HIGH;
  if(raw!=rawHigh){rawHigh=raw;changedAt=now;}
  if(now<1800){ready=false;stableHigh=true;return;}
  if(now-changedAt<45)return;
  if(!ready){
    stableHigh=raw;
    if(raw)ready=true;
    return;
  }
  if(raw==stableHigh)return;
  stableHigh=raw;
  if(!stableHigh){downAt=now;return;}
  lastUi=now-UI_PERIOD; // button actions redraw promptly
  uint32_t held=now-downAt;
  if(held>6000)togglePortal();
  else if(held>2500){if(sessionArmed)stopSession();else startSession();}
  else if(held>650){if(sessionArmed){paused=!paused;dirty=true;}}
  else if(held>35){screen=(screen+1)%4;dirty=true;}
}
// CYD v1.3.84 standalone capture port: passive beacon/probe-response
// sniffing, explicit 2.4/5 GHz channel sweep, and hot-channel revisits.
// Mini-specific: no ESP-NOW, no Core, and SD/GPS work only in loop().
static const uint8_t MINI_CHANNELS[]={1,2,3,4,5,6,7,8,9,10,11,
  36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,
  132,136,140,144,149,153,157,161,165};
static constexpr uint8_t MINI_CH_COUNT=sizeof(MINI_CHANNELS);
struct MiniAp {uint8_t mac[6];char ssid[33];int8_t rssi;uint8_t ch,auth;};
static MiniAp apRing[96];
static volatile uint8_t apHead=0,apTail=0;
static portMUX_TYPE miniCaptureMux=portMUX_INITIALIZER_UNLOCKED;
struct MiniCaptureGuard {
  MiniCaptureGuard(){portENTER_CRITICAL(&miniCaptureMux);}
  ~MiniCaptureGuard(){portEXIT_CRITICAL(&miniCaptureMux);}
};
static volatile uint32_t apDrops=0;
static volatile bool sniffing=false;
// Discovery Intelligence: bounded per-dwell hash table, no silent overflow-as-duplicate.
// Keep callback-critical storage internal (not PSRAM), with full 6-byte BSSID comparisons.
static constexpr uint16_t MINI_DWELL_SEEN_CAP=256;
static uint8_t dwellSeen[MINI_DWELL_SEEN_CAP][6] = {};
static uint8_t dwellOccupied[MINI_DWELL_SEEN_CAP] = {};
static volatile uint32_t miniDedupeHits=0,miniDedupeOverflows=0;
static volatile uint8_t dwellSeenN=0; // legacy diagnostic; reset with occupancy
static uint32_t miniScanResults=0,miniUniqueAtScanStart=0;
static uint32_t miniPsramFreeAtStart=0;

static uint8_t chHits[MINI_CH_COUNT]={};
static uint16_t chScore[MINI_CH_COUNT]={};
static uint8_t revisit[8]={},revisitN=0,revisitPos=0,chIndex=0;
static uint8_t miniSweepPhase=0;
static bool revisitPhase=false,sweepOn=false;
static uint32_t sliceAt=0,sweepDone=0;
static uint16_t activeDwell=75;
static uint32_t scanErrors=0,completedSweeps=0;
static int8_t activeIndex=-1;
static bool sameDwellMac(const uint8_t *m){
  uint32_t h=2166136261UL;
  for(uint8_t i=0;i<6;i++){h^=m[i];h*=16777619UL;}
  uint16_t start=(uint16_t)(h%MINI_DWELL_SEEN_CAP);
  for(uint16_t i=0;i<MINI_DWELL_SEEN_CAP;i++){
    uint16_t slot=(start+i)%MINI_DWELL_SEEN_CAP;
    if(!dwellOccupied[slot]){
      memcpy(dwellSeen[slot],m,6);dwellOccupied[slot]=1;
      if(dwellSeenN<255)dwellSeenN++;
      return false;
    }
    if(memcmp(dwellSeen[slot],m,6)==0){miniDedupeHits++;return true;}
  }
  // Capacity exceeded: do not pretend unseen BSSIDs are duplicates.
  miniDedupeOverflows++;
  return false;
}
static inline void resetMiniDwellDedupe(){memset(dwellOccupied,0,sizeof(dwellOccupied));dwellSeenN=0;}
void onMiniPromisc(void *buf,wifi_promiscuous_pkt_type_t type){
  MiniCaptureGuard guard;
  if(!sniffing||type!=WIFI_PKT_MGMT||!buf)return;
  const wifi_promiscuous_pkt_t *pkt=(const wifi_promiscuous_pkt_t*)buf;
  const uint8_t *p=pkt->payload;
  uint16_t len=pkt->rx_ctrl.sig_len;
  if(len<36||len>2304)return;
  uint8_t subtype=p[0]&0xf0;
  if(subtype!=0x80&&subtype!=0x50)return;
  const uint8_t *mac=p+16;
  if(sameDwellMac(mac))return;
  if(activeIndex>=0&&activeIndex<MINI_CH_COUNT&&chHits[activeIndex]<255)chHits[activeIndex]++;
  MiniAp r={};memcpy(r.mac,mac,6);
  r.rssi=(int8_t)pkt->rx_ctrl.rssi;
  r.ch=(uint8_t)pkt->rx_ctrl.channel;
  uint16_t cap=(uint16_t)p[34]|((uint16_t)p[35]<<8);
  r.auth=(cap&0x10)?1:0;
  uint8_t dsCh=0,htCh=0;
  for(size_t pos=36;pos+2<=len;){
    uint8_t id=p[pos++],n=p[pos++];if(pos+n>len)break;
    if(id==0){uint8_t copy=n>32?32:n;memcpy(r.ssid,p+pos,copy);r.ssid[copy]=0;}
    else if(id==3&&n>=1)dsCh=p[pos];
    else if(id==61&&n>=1)htCh=p[pos];
    else if(id==48)r.auth=3;
    else if(id==221&&n>=4&&p[pos]==0&&p[pos+1]==0x50&&p[pos+2]==0xf2&&p[pos+3]==1&&r.auth<2)r.auth=2;
    pos+=n;
  }
  const uint8_t advertised=dsCh?dsCh:htCh;
  if((advertised>=1&&advertised<=14)||(advertised>=36&&advertised<=177))r.ch=advertised;
  uint8_t next=(uint8_t)((apHead+1)%96);
  if(next==apTail){apDrops++;return;}
  apRing[apHead]=r;apHead=next;
}
void drainMiniAps(uint8_t budget=24){
  while(budget--&&apTail!=apHead){
    MiniAp r={};
    { MiniCaptureGuard guard;
      if(apTail==apHead)break;
      r=apRing[apTail];apTail=(uint8_t)((apTail+1)%96);
    }
    char mac[18];snprintf(mac,sizeof(mac),"%02X:%02X:%02X:%02X:%02X:%02X",r.mac[0],r.mac[1],r.mac[2],r.mac[3],r.mac[4],r.mac[5]);
    String bssid(mac);uint32_t id=hashId(bssid);
    wifiTotal++;
    if(markUnique(wifiSeen,id)){
      wifiUnique++;
      for(uint8_t i=0;i<MINI_CH_COUNT;i++)if(MINI_CHANNELS[i]==r.ch){chScore[i]=(uint16_t)min(255,(int)chScore[i]+12);break;}
    }
    if(r.ch>=1&&r.ch<=14){if(markUnique(wifi24Seen,id))wifi24Unique++;}
    else if(r.ch>14){if(markUnique(wifi5Seen,id))wifi5Unique++;}
    lastSSID=r.ssid;lastRssi=r.rssi;
    const char *auth=r.auth==0?"OPEN":r.auth==1?"WEP":r.auth==2?"WPA":"WPA2/WPA3";
    // Count discoveries without GPS; record each BSSID once when GPS+UTC are valid.
    if(gpsFresh()&&gpsTimeFresh()&&sessionArmed&&!paused&&sdReady&&!hasSeen(wifiLogged,id)) {
      if(logRecord(bssid,String(r.ssid),auth,r.ch,r.rssi,false)) markUnique(wifiLogged,id);
    }
    dirty=true;serviceGps();
  }
}
void stopMiniSweep(){
  {MiniCaptureGuard guard;sniffing=false;}
  esp_wifi_set_promiscuous(false);
  drainMiniAps(96);sweepOn=false;activeIndex=-1;
  sweepDone=millis();completedSweeps++;miniScanResults=wifiUnique-miniUniqueAtScanStart;
  Serial.printf("[SCAN] sweep %lu WiFi unique=%lu 2.4=%lu 5=%lu queueDrops=%lu channelErrors=%lu\\n",
    (unsigned long)completedSweeps,(unsigned long)wifiUnique,(unsigned long)wifi24Unique,
    (unsigned long)wifi5Unique,(unsigned long)apDrops,(unsigned long)scanErrors);
  Serial.printf("[DI] sweep new=%lu dedupeHits=%lu overflow=%lu psramFreeStart=%lu\n",
    (unsigned long)miniScanResults,(unsigned long)miniDedupeHits,
    (unsigned long)miniDedupeOverflows,(unsigned long)miniPsramFreeAtStart);
}
void startMiniSweep(){
  WiFi.mode(WIFI_STA);WiFi.disconnect(false,false);
#if CONFIG_SOC_WIFI_SUPPORT_5G
  WiFi.setBandMode(WIFI_BAND_MODE_AUTO);
#endif
  wifi_promiscuous_filter_t f={};f.filter_mask=WIFI_PROMIS_FILTER_MASK_MGMT;
  if(esp_wifi_set_promiscuous_filter(&f)!=ESP_OK||
     esp_wifi_set_promiscuous_rx_cb(&onMiniPromisc)!=ESP_OK){scanErrors++;return;}
  {MiniCaptureGuard guard;sniffing=false;apHead=apTail=0;resetMiniDwellDedupe();activeIndex=-1;}
  chIndex=0;miniSweepPhase=(uint8_t)((miniSweepPhase+1)%MINI_CH_COUNT);
  miniUniqueAtScanStart=wifiUnique;miniPsramFreeAtStart=psramFound()?ESP.getFreePsram():0;
  revisitN=revisitPos=0;revisitPhase=false;sliceAt=0;
  memset(chHits,0,sizeof(chHits));
  for(uint8_t i=0;i<MINI_CH_COUNT;i++)chScore[i]=(uint16_t)(chScore[i]*3/4);
  sweepOn=true; // capture starts only after the first channel is tuned
}
void serviceWifi(){
  if(portalActive||!sessionArmed||paused||!gpsFresh()||!gpsTimeFresh()){if(sweepOn)stopMiniSweep();return;}
  if(!sweepOn){if(millis()-sweepDone>=SCAN_PERIOD)startMiniSweep();return;}
  drainMiniAps();
  uint32_t now=millis();
  if(sliceAt&&now-sliceAt<activeDwell+5)return;
  {MiniCaptureGuard guard;sniffing=false;}
  esp_wifi_set_promiscuous(false);
  drainMiniAps(); // finish this dwell before scoring/reusing its capture state
  if(chIndex>=MINI_CH_COUNT&&!revisitPhase){
    if(smartAggressive){
      for(uint8_t i=0;i<MINI_CH_COUNT;i++)chScore[i]=(uint16_t)min(255,(int)chScore[i]+(int)chHits[i]*3);
      for(uint8_t k=0;k<8;k++){
        int best=-1,rank=0;
        for(uint8_t i=0;i<MINI_CH_COUNT;i++){
          if(chHits[i]<2)continue;
          bool used=false;for(uint8_t j=0;j<revisitN;j++)if(revisit[j]==i)used=true;
          int r=chScore[i]+chHits[i]*8;
          if(!used&&r>rank){best=i;rank=r;}
        }
        if(best<0)break;revisit[revisitN++]=(uint8_t)best;
      }
    }
    revisitPhase=revisitN>0;
  }
  if(chIndex>=MINI_CH_COUNT&&(!revisitPhase||revisitPos>=revisitN)){
    stopMiniSweep();return;
  }
  uint8_t index;
  if(revisitPhase&&chIndex>=MINI_CH_COUNT){
    index=revisit[revisitPos++];
    activeDwell=(chHits[index]>=6||chScore[index]>=64)?125:100;
  }else{index=(uint8_t)((chIndex++ + miniSweepPhase)%MINI_CH_COUNT);activeDwell=wifiDwellMs;}
  {MiniCaptureGuard guard;sniffing=false;}
  esp_wifi_set_promiscuous(false);
  esp_err_t e=esp_wifi_set_channel(MINI_CHANNELS[index],WIFI_SECOND_CHAN_NONE);
  {MiniCaptureGuard guard;activeIndex=(int8_t)index;resetMiniDwellDedupe();sniffing=(e==ESP_OK);}
  if(e==ESP_OK)e=esp_wifi_set_promiscuous(true);
  sliceAt=millis();
  if(e!=ESP_OK){MiniCaptureGuard guard;sniffing=false;scanErrors++;sliceAt=millis()-activeDwell-5;}
}
void serviceBle(){
  const bool allowed=!portalActive && !uploadBusy && sessionArmed && !paused && gpsFresh() && gpsTimeFresh();
  if(miniBleActive){
    NimBLEScan* s=NimBLEDevice::getScan();
    if(!s){miniBleActive=false;return;}
    if((!allowed || (uint32_t)(millis()-miniBleStartedMs)>=1000UL) && s->isScanning())s->stop();
    if(s->isScanning())return;
    miniBleActive=false;s->clearResults();
  }
  if(!allowed || (uint32_t)(millis()-lastBle)<5000UL)return;
  lastBle=millis();initBle();NimBLEScan* s=NimBLEDevice::getScan();
  if(!s || !s->start(350,false)){miniBleStartFailures++;return;}
  miniBleActive=true;miniBleStartedMs=millis();
}

// Portal opens automatically for 60 seconds at boot; hold >6 seconds to reopen.
// AP mode and frequent channel scans compete for the same Wi-Fi radio.
// While portal is open, scanning pauses; recording resumes after closing
// when the session remains armed and GPS + UTC are fresh.
String htmlEscape(String s){s.replace("&","&amp;");s.replace("<","&lt;");s.replace(">","&gt;");s.replace("\"","&quot;");return s;}
String safeCsvName(String name){
  if(name=="upload_history.csv"||name.length()<5||name.length()>75||name.indexOf('/')>=0||name.indexOf('\\')>=0||name.indexOf("..")>=0||!name.endsWith(".csv"))return "";
  return name;
}
static const char* MINI_UPLOADED_DIR="/Uploaded CSVs";
static bool archiveMiniCsv(const String& name){
  if(!safeCsvName(name).length())return false;
  String source="/"+name;
  if(!sdReady || !SD.exists(source))return false;
  if(!SD.exists(MINI_UPLOADED_DIR) && !SD.mkdir(MINI_UPLOADED_DIR))return false;
  if(source==csvPath)closeCsv(); // Recording resumes in a new file when portal closes.
  String destination=String(MINI_UPLOADED_DIR)+"/"+name;
  if(SD.exists(destination)){
    String stem=name.substring(0,name.length()-4);bool freeName=false;
    for(int n=1;n<1000;n++){
      destination=String(MINI_UPLOADED_DIR)+"/"+stem+"_"+String(n)+".csv";
      if(!SD.exists(destination)){freeName=true;break;}
    }
    if(!freeName)return false;
  }
  bool ok=SD.rename(source,destination);
  if(ok)Serial.printf("[CSV] Archived %s -> %s\n",source.c_str(),destination.c_str());
  return ok;
}
static bool miniUploadSavedPaused=false;
static void miniUploadMemory(const char* step){
  Serial.printf("[UPLOAD MEM] %s internal=%u largest=%u psram=%u\n",step,
    (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),
    (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),
    (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
}
static void beginMiniUploadPause(){
  miniUploadSavedPaused=paused;paused=true;uploadBusy=true;
  miniUploadMemory("before pause");
  if(sweepOn)stopMiniSweep();
  if(csv)csv.flush();
  if(bleReady){
    NimBLEScan* scan=NimBLEDevice::getScan();if(scan)scan->stop();
    NimBLEDevice::deinit(false);bleReady=false;miniBleActive=false;
  }
  miniUploadMemory("after BLE pause");
}
static void endMiniUploadPause(){
  paused=miniUploadSavedPaused;
  uploadBusy=false;
  miniUploadMemory("after upload");
  // BLE scanner is lazily initialized by serviceBle() after the portal closes.
}
String portalPage(){
  String h=F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'><title>WardriveHub Mini</title><style>body{background:#101a17;color:#e4f9e9;font:17px system-ui;max-width:660px;margin:20px auto;padding:0 14px}section{background:#23352e;border-radius:12px;padding:15px;margin:14px 0}button,select,input{font-size:17px;padding:10px;margin:5px}button{background:#7aefa7;border:0;border-radius:8px}button.danger{background:#ff9a90}label{display:block;padding:7px 0;border-bottom:1px solid #43584b}small{color:#b8cfc0}code{overflow-wrap:anywhere}</style></head><body><h1>WARDRIVE HUB MINI</h1><p>Standalone settings and CSV uploader</p><section><h2>Upload configuration</h2><p>Home Wi-Fi: ");
  h+=cfgSsid.length()?htmlEscape(cfgSsid):"NOT CONFIGURED";
  h+="<br>WiGLE key: ";h+=cfgWigle.length()?"Configured":"Missing";
  h+="<br>WDG Wars key: ";h+=cfgWdg.length()?"Configured":"Missing";
  h+="<br>Signal Frontier key: ";h+=cfgSignalFrontier.length()?"Configured":"Missing / disabled";
  h+="</p><small>Reads /uploadcfg.txt from the SD card. Keys are never displayed here.</small></section><section><h2>Wi-Fi aggressiveness</h2><form method='POST' action='/settings'><select name='dwell'>";
  h+="<option value='smart'"+String(smartAggressive?" selected":"")+">Smart Aggressive (CYD channel scoring + hot-channel revisits)</option>";
  for(int v:{75,100,125})h+="<option value='"+String(v)+"'"+(!smartAggressive&&wifiDwellMs==v?" selected":"")+">"+String(v)+" ms"+(v==75?" - Aggressive (default)":v==100?" - Normal":" - Relaxed")+"</option>";
  h+="</select><p><small>Current dwell: "+String(wifiDwellMs)+" ms. Smart mode scans every channel at 75 ms, then revisits productive channels at 100-125 ms. BLE is unchanged.</small></p>";
  h+=F("<button>Save speed</button></form><small>Scanning pauses in portal mode.</small></section><section><h2>CSV manager</h2><p>Select files, then choose an action.</p><form method='POST' action='/csv'><button type='button' onclick='document.querySelectorAll(`input[name=file]`).forEach(x=>x.checked=true)'>Select all</button><button type='button' onclick='document.querySelectorAll(`input[name=file]`).forEach(x=>x.checked=false)'>Deselect all</button>");
  int count=0;
  if(sdReady){File root=SD.open("/");if(root){File f=root.openNextFile();while(f&&count<60){String name=String(f.name());if(name.startsWith("/"))name.remove(0,1);if(!f.isDirectory()&&safeCsvName(name).length()){
    h+="<label><input type='checkbox' name='file' value='"+htmlEscape(name)+"'> "+htmlEscape(name)+" ("+String(f.size())+" bytes) <a style='color:#7aefa7' href='/download?name="+name+"'>Download</a></label>";count++;
  }f.close();f=root.openNextFile();}root.close();}}
  if(!count)h+="<p>No CSV files found on SD card.</p>";
  h+=F("<p><button name='action' value='testwigle'>Test WiGLE</button><button name='action' value='testwdg'>Test WDG Wars</button><button name='action' value='testsignal'>Test Signal Frontier</button></p><p><button name='action' value='wigle'>Upload WiGLE</button><button name='action' value='wdg'>Upload WDG Wars</button><button name='action' value='signal'>Upload Signal Frontier</button><button name='action' value='allsignal'>Upload all CSVs to Signal Frontier</button><button name='action' value='both'>Upload Both</button><button name='action' value='allthree'>Upload All Three</button></p><p><button class='danger' name='action' value='delete' onclick='return confirm(\"Permanently delete selected CSV files?\")'>Delete selected</button></p></form><small>Signal Frontier test validates the configured API key using GET /api/v1/device/me. WiGLE/WDGWars tests may upload a selected CSV. Successfully uploaded CSVs move to the Uploaded CSVs folder. Failed archives stay in the pending list.</small></section><section><h2>Last operation</h2><pre style='white-space:pre-wrap;overflow-wrap:anywhere'>");
  h+=htmlEscape(uploadReport);h+=F("</pre><p><a href='/history' style='color:#7aefa7'>View persistent upload history</a></p></section><section><h2>Import CSV</h2><form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='file' accept='.csv'><button>Import to SD</button></form></section><p>Portal closes after 60 seconds without a connected phone. Hold button 6 seconds to reopen.</p></body></html>");return h;
}
// The AP is only the control interface; use the configured home network for HTTPS.
bool ensureInternet(){
  if(WiFi.status()==WL_CONNECTED)return true;
  if(!cfgSsid.length())return false;
  WiFi.begin(cfgSsid.c_str(),cfgPassword.c_str());
  uint32_t start=millis();while(WiFi.status()!=WL_CONNECTED&&millis()-start<18000UL){serviceGps();delay(30);}
  return WiFi.status()==WL_CONNECTED;
}
// Upload TLS mode restored to legacy behavior: no SD certificate or NTP dependency.
// WARNING: setInsecure() encrypts traffic but DOES NOT authenticate the server.

bool sendCsvToService(const String& name,bool wigle,int& httpCode,String& reason){
  httpCode=0;reason="";String key=wigle?cfgWigle:cfgWdg;
  if(!key.length()){reason="API key missing";return false;}
  if(!ensureInternet()){reason="Home Wi-Fi unavailable";return false;}
  File f=SD.open("/"+name,FILE_READ);if(!f){reason="Cannot open CSV";return false;}
  String host=wigle?"api.wigle.net":"wdgwars.pl",path=wigle?"/api/v2/file/upload":"/api/upload-csv";
  String boundary="----WDHMini149",pre="--"+boundary+"\r\n";
  if(wigle)pre+="Content-Disposition: form-data; name=\"donate\"\r\n\r\nfalse\r\n--"+boundary+"\r\n";
  pre+="Content-Disposition: form-data; name=\"file\"; filename=\""+name+"\"\r\nContent-Type: text/csv\r\n\r\n";
  String post="\r\n--"+boundary+"--\r\n";
  WiFiClientSecure client;
  client.setInsecure(); // Restore pre-audit HTTPS behavior; no SD CA file.
  client.setTimeout(20000);client.setHandshakeTimeout(20);
  if(!client.connect(host.c_str(),443)){f.close();reason="HTTPS connection failed";return false;}
  String auth=wigle?(key.startsWith("Basic ")?key:("Basic "+key)):key;
  client.print("POST "+path+" HTTP/1.1\r\nHost: "+host+"\r\nUser-Agent: WardriveHub-Mini/1.7.0\r\nAccept: application/json\r\n");
  client.print(String(wigle?"Authorization: ":"X-API-Key: ")+auth+"\r\nContent-Type: multipart/form-data; boundary="+boundary+"\r\nContent-Length: "+String(pre.length()+f.size()+post.length())+"\r\nConnection: close\r\n\r\n");
  client.print(pre);uint8_t buf[512];bool sent=true;
  while(f.available()){
    size_t n=f.read(buf,sizeof(buf));if(n&&client.write(buf,n)!=n){sent=false;break;}
    serviceGps();delay(0);
  }
  f.close();if(!sent){client.stop();reason="Connection lost during transfer";return false;}
  client.print(post);
  uint32_t start=millis();while(!client.available()&&client.connected()&&millis()-start<25000UL){serviceGps();delay(10);}
  String status=client.readStringUntil('\n');int pos=status.indexOf(' ');if(pos>0)httpCode=status.substring(pos+1).toInt();
  // Response body is diagnostic only; never echo auth headers or API keys.
  bool inBody=false;uint32_t last=millis();while((client.available()||client.connected())&&millis()-last<2500UL){
    if(!client.available()){serviceGps();delay(5);continue;}
    String line=client.readStringUntil('\n');line.trim();if(!inBody){if(!line.length())inBody=true;}else if(reason.length()<180)reason+=line.substring(0,180-reason.length());last=millis();
  }
  client.stop();if(reason.length()>180)reason.remove(180);
  return httpCode>=200&&httpCode<300;
}

// Signal Frontier v2.5.1 device API: authenticated identity, multipart CSV,
// then status polling. HTTP 202 means QUEUED, not completed.
struct SfHttpResult { int code=0; String body; };

static bool sfSafeHost(String host){
  if(host.length()==0 || host.length()>120 || host.startsWith(".") || host.endsWith("."))return false;
  for(size_t i=0;i<host.length();i++){
    char c=host[i];
    if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='-'))return false;
  }
  return true;
}
static String sfJsonField(const String& json,const String& name){
  int p=json.indexOf(String("\"")+name+"\"");
  if(p<0)return "";
  p=json.indexOf(':',p+name.length()+2);
  if(p<0)return "";
  p++;while(p<(int)json.length() && (json[p]==' '||json[p]=='\r'||json[p]=='\n'))p++;
  if(p>=(int)json.length())return "";
  if(json[p]=='"'){
    int e=json.indexOf('"',p+1);
    if(e<0)return "";
    return json.substring(p+1,e);
  }
  int e=p;
  while(e<(int)json.length() && json[e]!=',' && json[e]!='}' && json[e]!='\r' && json[e]!='\n')e++;
  String v=json.substring(p,e);v.trim();return v;
}
static int sfReadResponse(WiFiClientSecure& c,String& body){
  body="";
  uint32_t began=millis();
  while(!c.available() && c.connected() && (uint32_t)(millis()-began)<25000UL){delay(10);}
  if(!c.available())return -4;
  String first=c.readStringUntil('\n');int sp=first.indexOf(' ');
  int code=(sp>=0)?first.substring(sp+1).toInt():0;
  bool endedHeaders=false;
  for(int i=0;i<50;i++){
    if(!c.connected() && !c.available())break;
    String line=c.readStringUntil('\n');line.trim();
    if(!line.length()){endedHeaders=true;break;}
  }
  if(!endedHeaders)return code?code:-4;
  uint32_t lastByte=millis();
  while((uint32_t)(millis()-began)<30000UL){
    bool any=false;
    while(c.available()){
      int ch=c.read(); if(ch<0)break;
      if(body.length()<3072)body+=(char)ch;
      any=true;
    }
    if(any)lastByte=millis();
    if(!c.connected()&&!c.available())break;
    if((uint32_t)(millis()-lastByte)>1800UL)break;
    delay(5);
  }
  return code?code:-4;
}
static int sfHttpsGet(const String& host,const String& key,const String& path,String& response){
  response="";
  if(!sfSafeHost(host)||key.indexOf('\n')>=0||key.indexOf('\r')>=0)return -8;
  WiFiClientSecure c;c.setInsecure();c.setTimeout(20000);c.setHandshakeTimeout(20);
  if(!c.connect(host.c_str(),443)){
    char err[100]={0};int n=c.lastError(err,sizeof(err));
    Serial.printf("[SIGNAL FRONTIER] TLS error %d: %s | internal_free=%u\n",n,err,
      (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
    c.stop();return -3;
  }
  c.print("GET "+path+" HTTP/1.1\r\nHost: "+host+"\r\nAccept: application/json\r\nUser-Agent: WardriveHub-SF\r\nX-API-Key: "+key+"\r\nConnection: close\r\n\r\n");
  int code=sfReadResponse(c,response);c.stop();return code;
}
static int sfValidateKey(const String& host,const String& key){
  if(key.length()==0)return 0;
  String body;int code=sfHttpsGet(host,key,"/api/v1/device/me",body);
  if(code==200 && sfJsonField(body,"ok")!="true")return -7;
  return code;
}
static bool sfValidJobId(const String& id){
  if(id.length()!=36)return false;
  for(size_t i=0;i<id.length();i++){
    char c=id[i];
    if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')||c=='-'))return false;
  }
  return true;
}
static String sfPendingMarker(const String& path,const String& host,const String& key,size_t size){
  uint32_t h=2166136261UL;
  String seed=path+"|"+host+"|"+String((unsigned long)size)+"|"+key;
  for(size_t i=0;i<seed.length();i++){h^=(uint8_t)seed[i];h*=16777619UL;}
  char name[30];snprintf(name,sizeof(name),"/sf_job_%08lx.txt",(unsigned long)h);
  return String(name);
}
static bool sfWaitForJob(const String& host,const String& key,const String& id,int& code,String& result){
  for(int attempt=0;attempt<12;attempt++){
    String body;
    code=sfHttpsGet(host,key,"/api/v1/uploads/"+id,body);
    if(code==200){
      String status=sfJsonField(body,"status");
      if(status=="done") {result="SERVER DONE";return true;}
      if(status=="failed") {result="SERVER PROCESSING FAILED";return false;}
      if(status!="queued" && status!="processing") {result="UNKNOWN JOB STATUS";return false;}
      result=status=="queued"?"QUEUED, NOT ARCHIVED":"PROCESSING, NOT ARCHIVED";
    }else if(code==401||code==403){result="INVALID API KEY";return false;}
    else if(code==404){result="JOB NOT FOUND";return false;}
    else if(code==429){result="RATE LIMITED";return false;}
    else if(code<0){result="JOB STATUS CONNECTION ERROR";return false;}
    else {result="JOB STATUS HTTP "+String(code);return false;}
    delay(1750);
  }
  code=202;return false; // Keep the job marker and original CSV for later verification.
}
static bool sfSubmitCsv(const String& csv,const String& host,const String& key,int& code,String& result){
  code=0;result="";
  if(key.length()==0){result="SIGNAL FRONTIER KEY MISSING";return false;}
  if(!sfSafeHost(host)||key.indexOf('\r')>=0||key.indexOf('\n')>=0){result="INVALID SIGNAL FRONTIER HOST/KEY";return false;}
  File f=SD.open(csv,FILE_READ);
  if(!f){result="CSV NOT FOUND";return false;}
  const size_t fileSize=f.size();
  if(fileSize==0){f.close();result="EMPTY CSV";return false;}
  String marker=sfPendingMarker(csv,host,key,fileSize);
  String jobId="";
  if(SD.exists(marker)){
    File existing=SD.open(marker,FILE_READ);
    if(existing){jobId=existing.readStringUntil('\n');jobId.trim();existing.close();}
    if(!sfValidJobId(jobId)){SD.remove(marker);jobId="";}
  }
  if(!jobId.length()){
    String filename=csv;
    int slash=filename.lastIndexOf('/');if(slash>=0)filename=filename.substring(slash+1);
    filename.replace('"','_');filename.replace('\n','_');filename.replace('\r','_');
    String boundary="----WDHFrontier"+String((unsigned long)esp_random(),HEX);
    String before="--"+boundary+"\r\nContent-Disposition: form-data; name=\"file\"; filename=\""+filename+"\"\r\nContent-Type: text/csv\r\n\r\n";
    String after="\r\n--"+boundary+"--\r\n";
    WiFiClientSecure c;c.setInsecure();c.setTimeout(20000);c.setHandshakeTimeout(20);
    if(!c.connect(host.c_str(),443)) {f.close();c.stop();code=-3;result="TLS CONNECT FAIL";return false;}
    c.print("POST /api/upload-csv HTTP/1.1\r\nHost: "+host+"\r\nAccept: application/json\r\nUser-Agent: WardriveHub-SF\r\nX-API-Key: "+key+"\r\nContent-Type: multipart/form-data; boundary="+boundary+"\r\nContent-Length: "+String((unsigned long)(before.length()+fileSize+after.length()))+"\r\nConnection: close\r\n\r\n");
    if(c.print(before)!=before.length()){f.close();c.stop();code=-2;result="MULTIPART HEADER SEND FAILED";return false;}
    bool sent=true;uint8_t chunk[768];
    while(f.available()){
      size_t n=f.read(chunk,sizeof(chunk));
      if(!n)break;
      size_t off=0;
      while(off<n){size_t written=c.write(chunk+off,n-off);if(!written){sent=false;break;}off+=written;}
      if(!sent)break;
      delay(0);
    }
    f.close();
    if(sent && c.print(after)!=after.length())sent=false;
    if(!sent){c.stop();code=-2;result="CSV SEND FAILED";return false;}
    String body;code=sfReadResponse(c,body);c.stop();
    if(code!=202){result=code==401||code==403?"INVALID API KEY":("UPLOAD HTTP "+String(code));return false;}
    jobId=sfJsonField(body,"job_id");
    if(!sfValidJobId(jobId)){result="QUEUED, JOB ID MISSING";return false;}
    File pending=SD.open(marker,FILE_WRITE);
    if(!pending){result="QUEUED, CANNOT SAVE JOB ID";return false;}
    pending.println(jobId);pending.close();
  }else f.close();
  bool completed=sfWaitForJob(host,key,jobId,code,result);
  if(completed || result=="SERVER PROCESSING FAILED" || result=="JOB NOT FOUND")SD.remove(marker);
  return completed;
}

static int miniSignalFrontierTest(String& result){
  if(cfgSignalFrontier.length()==0){result="OFF / KEY MISSING";return 0;}
  if(!ensureInternet()){result="HOME WIFI UNAVAILABLE";return -1;}
  int code=sfValidateKey(cfgSignalFrontierHost,cfgSignalFrontier);
  result=code==200?"VALID":(code==401||code==403?"INVALID":("HTTP "+String(code)));
  return code;
}
void miniUploadHistory(const String& filename,const String& service,int code,bool ok){
  if(!sdReady)return;
  File h=SD.open("/upload_history.csv",FILE_APPEND);
  if(!h)return;
  String safe=filename;safe.replace(',', '_');safe.replace('\r','_');safe.replace('\n','_');
  h.print(millis());h.print(',');h.print(safe);h.print(',');h.print(service);
  h.print(',');h.print(code);h.print(',');h.println(ok?"OK":"FAILED");h.close();
}
// Signal Frontier only archives when background CSV processing reaches DONE.
bool sendCsvToSignalFrontier(const String& name,int& httpCode,String& reason){
  if(!ensureInternet()){httpCode=-1;reason="HOME WIFI UNAVAILABLE";return false;}
  return sfSubmitCsv("/"+name,cfgSignalFrontierHost,cfgSignalFrontier,httpCode,reason);
}

void togglePortal(){
  if(portalActive){if(uploadBusy)return;WiFi.disconnect(false,false);portal.stop();portalDns.stop();WiFi.softAPdisconnect(true);WiFi.mode(WIFI_STA);portalActive=false;sweepDone=0;dirty=true;Serial.println("[PORTAL] closed; dual-band discovery resumes; CSV GPS-gated");return;}
  if(sweepOn)stopMiniSweep();
  if(csv)csv.flush();
  WiFi.mode(WIFI_AP_STA);
  if(!WiFi.softAP("WardriveHub-Mini",portalPassword.c_str())){Serial.println("[PORTAL] AP failed");WiFi.mode(WIFI_STA);return;}
  portalDns.start(53,"*",WiFi.softAPIP());
  portal.begin();portalStarted=millis();portalActive=true;dirty=true;
  Serial.printf("[PORTAL] connect WardriveHub-Mini / %s, open http://192.168.4.1\n",portalPassword.c_str());
}
void initPortal(){
  portal.on("/",HTTP_GET,[]{portalStarted=millis();portal.send(200,"text/html",portalPage());});
  portal.on("/history",HTTP_GET,[]{
    portalStarted=millis();
    String html="<!doctype html><meta name='viewport' content='width=device-width,initial-scale=1'><style>body{background:#101a17;color:#e4f9e9;font:16px system-ui;padding:20px}a{color:#7aefa7}pre{white-space:pre-wrap;overflow-wrap:anywhere}</style><h2>UPLOAD HISTORY</h2><p>ms since boot, filename, service, HTTP, outcome</p><pre>";
    File h=SD.open("/upload_history.csv",FILE_READ);
    if(h){if(h.size()>6000)h.seek(h.size()-6000);String content=h.readString();h.close();html+=htmlEscape(content);}
    else html+="No uploads recorded";
    html+="</pre><a href='/'>Back to dashboard</a>";
    portal.send(200,"text/html",html);
  });
  portal.on("/generate_204",HTTP_GET,[]{portal.sendHeader("Location","http://192.168.4.1/",true);portal.send(302,"text/plain","");});
  portal.on("/hotspot-detect.html",HTTP_GET,[]{portal.sendHeader("Location","http://192.168.4.1/",true);portal.send(302,"text/plain","");});
  portal.on("/csv",HTTP_POST,[]{
    portalStarted=millis();if(uploadBusy){portal.send(409,"text/plain","Upload already running");return;}
    String action=portal.arg("action");bool test=action=="testwigle"||action=="testwdg"||action=="testsignal";
    bool wigle=action=="wigle"||action=="both"||action=="allthree"||action=="testwigle";
    bool wdg=action=="wdg"||action=="both"||action=="allthree"||action=="testwdg";
    bool signalFrontier=action=="signal"||action=="allthree"||action=="testsignal"||action=="allsignal";
    bool del=action=="delete";
    if(!wigle&&!wdg&&!signalFrontier&&!del){portal.send(400,"text/plain","Invalid action");return;}
    String names[60];int count=0;
    for(int i=0;i<portal.args()&&count<60;i++)if(portal.argName(i)=="file"){
      String n=safeCsvName(portal.arg(i));if(!n.length()||!SD.exists("/"+n))continue;
      bool duplicate=false;for(int j=0;j<count;j++)if(names[j]==n)duplicate=true;
      if(!duplicate)names[count++]=n;
    }
    if(action=="allsignal"){
      beginMiniUploadPause();uploadReport="";
      String pending[60];int pendingCount=0;
      File root=SD.open("/");int success=0,fail=0;
      if(root){File e=root.openNextFile();
        while(e && pendingCount<60){String n=String(e.name());if(n.startsWith("/"))n.remove(0,1);
          if(!e.isDirectory()&&safeCsvName(n).length())pending[pendingCount++]=n;
          e.close();e=root.openNextFile();
        }
        if(e)e.close();root.close();
      }
      for(int i=0;i<pendingCount;i++){
        String n=pending[i];int code=0;String reason;
        bool ok=sendCsvToSignalFrontier(n,code,reason);
        miniUploadHistory(n,"SIGNAL_FRONTIER",code,ok);
        if(ok){
          if(archiveMiniCsv(n)){success++;miniUploadHistory(n,"ARCHIVE",0,true);}
          else {fail++;miniUploadHistory(n,"ARCHIVE",0,false);uploadReport+=n+": UPLOAD ACCEPTED, ARCHIVE FAILED (still pending)\n";}
        }else fail++;
        serviceGps();yield();
      }
      uploadReport=String("Signal Frontier all CSVs: ")+success+" uploaded AND archived, "+fail+" failed or not archived. "+uploadReport;
      endMiniUploadPause();portalStarted=millis();portal.sendHeader("Location","/",true);portal.send(303,"text/plain","Finished");return;
    }
    if(!count && action!="testsignal"){portal.send(400,"text/plain","Select at least one CSV file");return;}
    beginMiniUploadPause();uploadReport="";
    if(action=="testsignal"){
      String result;int code=miniSignalFrontierTest(result);
      uploadReport+="Signal Frontier test: "+result+" (HTTP "+String(code)+")\n";
      uploadReport+="VALID confirms authentication at the Signal Frontier device API.\n";
    }
    for(int i=0;i<count;i++){
      String n=names[i];if(test&&i>0)break;
      if(del){if("/"+n==csvPath){uploadReport+=n+": active recording file; not deleted\n";continue;}
        bool ok=SD.remove("/"+n);uploadReport+=n+(ok?": deleted\n":": delete failed\n");continue;}
      bool allOk=true,attempted=false;
      if(wigle){int code;String reason;bool ok=sendCsvToService(n,true,code,reason);uploadReport+=n+" WiGLE: "+(ok?"ACCEPTED ":"FAILED ")+String(code)+" "+reason+"\n";miniUploadHistory(n,"WIGLE",code,ok);allOk &= ok;attempted=true;}
      if(wdg){int code;String reason;bool ok=sendCsvToService(n,false,code,reason);uploadReport+=n+" WDG Wars: "+(ok?"ACCEPTED ":"FAILED ")+String(code)+" "+reason+"\n";miniUploadHistory(n,"WDGWARS",code,ok);allOk &= ok;attempted=true;}
      if(signalFrontier && !test){int code;String reason;bool ok=sendCsvToSignalFrontier(n,code,reason);uploadReport+=n+" Signal Frontier: "+(ok?"DONE ":(code==202?"QUEUED ":"FAILED "))+String(code)+" "+reason+"\n";miniUploadHistory(n,"SIGNAL_FRONTIER",code,ok);allOk &= ok;attempted=true;}
      if(attempted && allOk){
        if(archiveMiniCsv(n)){uploadReport+=n+": moved to /Uploaded CSVs\n";miniUploadHistory(n,"ARCHIVE",0,true);}
        else {uploadReport+=n+": UPLOADED REMOTELY, ARCHIVE FAILED (still pending)\n";miniUploadHistory(n,"ARCHIVE",0,false);}
      }
      if(uploadReport.length()>3500){uploadReport+="Further results omitted\n";break;}
    }
    endMiniUploadPause();portalStarted=millis();portal.sendHeader("Location","/",true);portal.send(303,"text/plain","Finished");
  });
  portal.on("/settings",HTTP_POST,[]{portalStarted=millis();String selected=portal.arg("dwell");bool smart=(selected=="smart");int v=selected.toInt();if(!smart&&v!=75&&v!=100&&v!=125){portal.send(400,"text/plain","Invalid dwell setting");return;}smartAggressive=smart;wifiDwellMs=smart?75:v;settings.putBool("smartAgg",smart);settings.putUShort("wifiDwell",wifiDwellMs);portal.sendHeader("Location","/",true);portal.send(303,"text/plain","Saved");dirty=true;});
  portal.on("/download",HTTP_GET,[]{portalStarted=millis();String name=portal.arg("name");if(name.length()==0||name.indexOf('/')>=0||name.indexOf('\\')>=0||name.indexOf("..")>=0||!name.endsWith(".csv")){portal.send(400,"text/plain","Invalid filename");return;}if(csv)csv.flush();File f=SD.open("/"+name,FILE_READ);if(!f){portal.send(404,"text/plain","File not found");return;}portal.sendHeader("Content-Disposition","attachment; filename=\""+name+"\"");portal.streamFile(f,"text/csv");f.close();});
  portal.on("/upload",HTTP_POST,[]{portalStarted=millis();if(uploadCsv){uploadCsv.close();}portal.sendHeader("Location","/",true);portal.send(303,"text/plain","Import completed; check SD list");},[]{
    HTTPUpload &u=portal.upload();
    if(u.status==UPLOAD_FILE_START){
      uploadName=u.filename;uploadName.replace("\\","/");int slash=uploadName.lastIndexOf('/');if(slash>=0)uploadName=uploadName.substring(slash+1);
      if(uploadName.length()>55||uploadName.indexOf("..")>=0||!uploadName.endsWith(".csv")||!sdReady){uploadName="";return;}
      // Do not overwrite existing data or the current recording file.
      String base="/IMPORT_"+String(millis())+"_"+uploadName;
      if(SD.exists(base)){uploadName="";return;}
      uploadCsv=SD.open(base,FILE_WRITE);
    }else if(u.status==UPLOAD_FILE_WRITE){if(uploadCsv&&u.currentSize&&uploadCsv.write(u.buf,u.currentSize)!=u.currentSize){sdErrors++;uploadCsv.close();}}
    else if(u.status==UPLOAD_FILE_END||u.status==UPLOAD_FILE_ABORTED){if(uploadCsv){uploadCsv.flush();uploadCsv.close();}}
  });
  portal.onNotFound([]{portal.sendHeader("Location","http://192.168.4.1/",true);portal.send(302,"text/plain","");});

}

void setup(){Serial.begin(115200);pinMode(BUTTON_PIN,INPUT_PULLUP);Wire.begin(OLED_SDA,OLED_SCL);oledReady=display.begin(SSD1306_SWITCHCAPVCC,0x3C);splash();gpsUart.setRxBufferSize(1024);gpsUart.begin(9600,SERIAL_8N1,GPS_RX,GPS_TX);sdSPI.begin(SD_SCK,SD_MISO,SD_MOSI,SD_CS);sdReady=SD.begin(SD_CS,sdSPI,8000000);settings.begin("wdh-mini-stand",false);
  portalPassword=settings.getString("portalPass","");
  if(portalPassword.length()<12){
    char pwd[21];snprintf(pwd,sizeof(pwd),"WDH-%08lX%08lX",(unsigned long)esp_random(),(unsigned long)esp_random());
    portalPassword=String(pwd);settings.putString("portalPass",portalPassword);
  }
  loadUploadConfig();smartAggressive=settings.getBool("smartAgg",false);wifiDwellMs=settings.getUShort("wifiDwell",75);if(wifiDwellMs!=75&&wifiDwellMs!=100&&wifiDwellMs!=125)wifiDwellMs=75;if(smartAggressive)wifiDwellMs=75;bool interrupted=settings.getBool("armed",false);sessionArmed=false;paused=false;WiFi.mode(WIFI_STA);WiFi.disconnect(false,false);WiFi.setAutoReconnect(false);WiFi.setScanActiveMinTime(0);gpsStartPending=true;initPortal();togglePortal();Serial.printf("WDH Mini 1.7.0 standalone | GPS RX %d TX %d | SD %s | previous session %s\n",GPS_RX,GPS_TX,sdReady?"OK":"FAIL",interrupted?"INTERRUPTED":"CLOSED");Serial.println("WAIT GPS: scanning starts only after fresh GPS position and UTC time.");dirty=true;draw();}
void loop() {
  static bool oldFix = false;
  static uint32_t oldSats = 0;

  serviceGps();
  serviceButton();
  if(portalActive){portalDns.processNextRequest();portal.handleClient();}
  // One-time auto-arm as soon as a valid GPS position and UTC arrive.
  // If the configuration portal has no connected clients, close it so the
  // first Wi-Fi sweep starts promptly instead of waiting its 60s timeout.
  if(gpsStartPending && !sessionArmed && gpsFresh() && gpsTimeFresh()) {
    startSession();
    if(portalActive && !uploadBusy && WiFi.softAPgetStationNum()==0) togglePortal();
    Serial.println("[GPS] GPS+UTC ready; capture started automatically");
  }
  serviceWifi();
  serviceBle();
  serviceBleQueue();

  const uint32_t now = millis();
  // Keep the portal available while a phone is associated, even if idle.
  // Once the last phone leaves, give another full 60 seconds to reconnect.
  if(portalActive){
    static bool hadClient=false;
    bool hasClient=WiFi.softAPgetStationNum()>0;
    if(hasClient){portalStarted=now;}
    else if(hadClient){portalStarted=now;}
    hadClient=hasClient;
    if(!uploadBusy&&!hasClient && now-portalStarted>=PORTAL_IDLE_TIMEOUT)togglePortal();
  }
  // Attempt card recovery periodically after a failed initial mount or removal.
  // Do not remount while an active CSV file or portal transfer holds SD handles.
  static uint32_t lastSdRemount = 0;
  if (!sdReady && !uploadBusy && !uploadCsv && !csv &&
      (uint32_t)(now - lastSdRemount) >= 15000UL) {
    lastSdRemount = now;
    SD.end();
    sdReady = SD.begin(SD_CS, sdSPI, 8000000);
    if (sdReady) { loadUploadConfig(); Serial.println("[SD] card recovered"); dirty=true; }
  }
  if (csv && now - lastFlush >= SD_FLUSH_MS) {
    lastFlush = now;
    csv.flush();
  }
  if (now - lastGpsDiag > 5000) {
    lastGpsDiag = now;
    if(screen==1) dirty=true;
    Serial.printf("GPS chars=%lu pass=%lu fail=%lu fix=%u age=%lu sat=%lu\\n",
                  (unsigned long)gps.charsProcessed(),
                  (unsigned long)gps.passedChecksum(),
                  (unsigned long)gps.failedChecksum(),
                  (unsigned)gpsFresh(),
                  (unsigned long)gps.location.age(),
                  (unsigned long)(gps.satellites.isValid() ? gps.satellites.value() : 0));
  }

  const bool fix = gpsFresh();
  const uint32_t sats = gps.satellites.isValid() ? gps.satellites.value() : 0;
  if (fix != oldFix || sats != oldSats) {
    oldFix = fix;
    oldSats = sats;
    dirty = true;
  }
  if((screen==0||screen==1) && now-lastUi>=1000)dirty=true;
  if (dirty && (uint32_t)(now-lastUi)>=UI_PERIOD) {
    lastUi = now;
    draw();
  }
  static uint32_t lastCaptureHealthMs=0;
  if((uint32_t)(now-lastCaptureHealthMs)>=10000UL){
    lastCaptureHealthMs=now;
    Serial.printf("[CAPTURE] bleQueueDrops=%lu bleStartErrors=%lu rowAllocErrors=%lu writesBlocked=%u\n",
      (unsigned long)bleQueueDrops,(unsigned long)miniBleStartFailures,(unsigned long)miniCsvBuildErrors,(unsigned)miniCsvWriteBlocked);
  }
  delay(1);
}
