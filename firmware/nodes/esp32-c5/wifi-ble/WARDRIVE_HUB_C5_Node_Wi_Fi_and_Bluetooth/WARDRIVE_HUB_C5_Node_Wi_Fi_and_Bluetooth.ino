// WardriveHub ESP32-C5 Mesh Node
// ESP-NOW receiver: CYD Core supplies scanning authorization and GPS.
// Shared WardNode C5 mesh protocol; see ATTRIBUTION.md for provenance.
// Dual-band Wi-Fi and BLE scanner.

#include <Arduino.h>
#include <stddef.h>
#include <atomic>
#include "ScanHelpers.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h>
#include <NimBLEDevice.h>
#include <esp_heap_caps.h>  // NimBLE-Arduino 2.x, same as working combo v1.1.7

#if !defined(CONFIG_IDF_TARGET_ESP32C5)
#error "WardNode C5 must be compiled for ESP32-C5 (board: XIAO_ESP32C5)."
#endif

static constexpr const char* NODE_FW_VERSION = "1.7.0";

// ---------------- Tunables ----------------
static const uint8_t  HOME_CH          = 6;     // must equal ESPNOW_CH in the hub
static const uint8_t  MAX_2G_CH        = 11;    // set 13 for EU/ROW
static const uint8_t  MAX_5G_CH        = 165;   // raise if your region allows 169+
static const uint32_t ACTIVE_DWELL_MS  = 75; // fixed dwell on active fallback
static const uint32_t PROMISC_DWELL_MS = 75; // fixed passive dwell on all channels
static const uint8_t ACTIVE_FALLBACK_EVERY = 4; // every fourth visit to EACH non-DFS channel
static const uint32_t HOME_MIN_MS      = 40;    // minimum time parked on home channel between steps
static const uint32_t HOME_MAX_MS      = 250;   // max time parked waiting for the TX queue to drain
static const uint32_t HUB_LOST_MS      = 30000; // no hub traffic for this long -> rejoin
static const uint8_t  TX_WINDOW        = 8;     // max un-ACKed records in flight
static const uint32_t TX_GAP_MS        = 8;     // spacing between ESP-NOW frames
static const uint32_t RETRY_MS         = 250;   // resend if no ACK
static const uint8_t  MAX_TRIES        = 6;

// ---------------- Shared ESP-NOW packet protocol ----------------
#define JCMK_TEXT_MAX 200
static const uint8_t MAGIC[4] = {'E','N','O','W'};
static const uint8_t BCAST[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

enum MsgType : uint8_t {
  MSG_CORE_REQUEST = 1,
  MSG_CORE_REPLY   = 2,
  MSG_HEARTBEAT    = 3,
  MSG_TEXT         = 4,
  MSG_ADMIN        = 5,
  MSG_NODE_CONFIG  = 6,
  MSG_NODE_STATUS  = 7,
  MSG_BLE_RECORD   = 8,
  MSG_GPS_STATUS   = 9,
  MSG_CAPABILITIES = 10,
  MSG_RELIABLE_TEXT = 11,
  MSG_RECORD_ACK   = 12
};

static constexpr uint32_t CAP_RELIABLE_ACK = 0x01UL;
static constexpr uint32_t CAP_WIFI_2G      = 0x02UL;
static constexpr uint32_t CAP_WIFI_5G      = 0x04UL;
static constexpr uint32_t CAP_BLE_SCAN     = 0x08UL;
static constexpr uint8_t  RELIABLE_VERSION = 1;

typedef struct __attribute__((packed)) {
  char magic[4]; uint8_t type; uint32_t counter; uint16_t len; char text[JCMK_TEXT_MAX + 1];
} jcmk_text_msg_t;

typedef struct __attribute__((packed)) {
  char magic[4]; uint8_t type; uint8_t version; uint16_t reserved; uint32_t capabilities;
} jcmk_capability_msg_t;

typedef struct __attribute__((packed)) {
  char magic[4]; uint8_t type; uint8_t version; uint8_t record_type; uint8_t reserved;
  uint32_t sequence; uint16_t len; char text[JCMK_TEXT_MAX + 1];
} jcmk_reliable_text_msg_t;

typedef struct __attribute__((packed)) {
  char magic[4]; uint8_t type; uint8_t version; uint16_t reserved; uint32_t sequence;
} jcmk_record_ack_msg_t;

typedef struct __attribute__((packed)) {
  char magic[4]; uint8_t type; uint8_t assignment_version; uint8_t node_index; uint8_t node_count;
  uint8_t start_channel_idx; uint8_t end_channel_idx;
} jcmk_admin_msg_t;

typedef struct __attribute__((packed)) {
  char magic[4]; uint8_t type; uint8_t version; uint8_t scan_mode; uint8_t run_enabled;
  uint32_t scan_interval_ms; uint32_t generation;
} jcmk_node_config_msg_t;

typedef struct __attribute__((packed)) {
  char magic[4]; uint8_t type; uint8_t version; uint8_t scan_mode; uint8_t flags;
  uint8_t node_index; uint8_t node_count; uint8_t start_channel_idx; uint8_t end_channel_idx;
  uint32_t wifi_records; uint32_t ble_records; uint32_t scan_passes; uint32_t config_generation;
} jcmk_node_status_msg_t;

static_assert(sizeof(jcmk_capability_msg_t) == 12, "capability size");
static_assert(sizeof(jcmk_record_ack_msg_t) == 12, "ack size");

// Status flag bits the hub understands
static const uint8_t ST_SCANNING = 0x04;
static const uint8_t ST_RUN_ACK  = 0x08;

// Channel table: indices in ADMIN refer to THIS table (identical to the hub's CHANNELS[])
static const uint8_t CHANNELS[] = {
  1,2,3,4,5,6,7,8,9,10,11,12,13,14,
  36,40,44,48,52,56,60,64,
  100,112,116,120,124,128,132,136,140,144,
  149,153,157,161,165,169,173,177,
  104,108 // appended: retain existing channel indices; upgrade Core and nodes together
};
static const uint8_t NUM_CHANNELS = sizeof(CHANNELS);

// ---------------- State shared with the receive callback ----------------
static uint8_t hubMac[6] = {};
static volatile bool     hubKnown     = false;   // set in the callback on first CORE_REPLY
static volatile bool     needPeer     = false;   // loop() must add the hub as an ESP-NOW peer
static volatile bool     hubPeerReady = false;   // peer exists -> safe to transmit to the hub
static volatile uint32_t lastHubRxMs  = 0;

static volatile bool     runEnabled   = false;
static volatile bool     configReceived = false;  // distinguish a missing RUN from an explicit PAUSE
static uint32_t          lastConfigRequestMs = 0;
static constexpr uint32_t CONFIG_RETRY_MS = 3000;
static volatile uint32_t scanIntervalMs = 150;
static volatile uint32_t cfgGeneration = 0;
static volatile uint8_t  startIdx = 0, endIdx = NUM_CHANNELS - 1;
static volatile uint8_t  adminIndex = 0, adminCount = 1;
static volatile bool     sendStatusNow = false, sendCapsNow = false;

static volatile uint32_t ackRing[16];
static std::atomic<uint8_t> ackHead{0}, ackTail{0};

// Receive diagnostics (written by the callback, printed by loop)
static volatile uint32_t rxFrames = 0, rxCoreReply = 0, rxAdmin = 0, rxConfig = 0, rxAck = 0, rxOther = 0;
static volatile uint8_t  rxLastType = 0;

// ---------------- Counters ----------------
static uint32_t bleRecords = 0;
static uint32_t wifiRecords = 0, scanPasses = 0, droppedRecords = 0, lostRecords = 0, retryCount = 0;
static volatile uint32_t lastScanProgressMs = 0;
static uint32_t deliveredRecords = 0, reconnects = 0, queueHighWater = 0;
// Discovery Intelligence: observational diagnostics only. These never gate scanning.
static uint32_t diagWifiPasses = 0, diagBleAttempts = 0, diagBleSkipped = 0;
static uint32_t diagWiFiChannelFailures = 0;
static uint32_t lastWiFiObservationMs = 0, lastBleObservationMs = 0;
static constexpr size_t DI_HISTORY_CAP = 512;
static uint32_t* diHistory = nullptr;  // Optional PSRAM telemetry cache; never used by the RX callback.
static size_t diHistoryPos = 0;
static bool diPsramHistoryReady = false;
static void diRemember(uint32_t key) {
  if (!diHistory) return;
  diHistory[diHistoryPos++] = key;
  if (diHistoryPos >= DI_HISTORY_CAP) diHistoryPos = 0;
}

static uint32_t txFail = 0, reqSent = 0;
static esp_err_t lastTxErr = ESP_OK;

// ---------------- Reliable TX queue ----------------
struct Rec {
  uint32_t seq;
  uint32_t lastTxMs;
  uint8_t  tries;
  uint8_t  len;
  bool     acked;
  uint8_t  recordType;
  char     text[96];
};
static const uint8_t QCAP = 128;
static Rec     q[QCAP];
static uint8_t qHead = 0, qCount = 0;
static uint32_t nextSeq = 1;
static uint32_t sequenceLimit=1;
static bool sequenceReservationReady=false;
static uint32_t sequenceRetryMs=0,sequenceReservationFailures=0;
static constexpr uint32_t SEQUENCE_RESERVATION_SIZE=1UL<<20;

// Reserve a large sequence range BEFORE using it. A reboot skips unused values
// instead of starting at 1 and having the Core mistake new records for old retries.
// One NVS commit per boot, then per million records; reconnects keep the RAM queue.
static bool reserveRecordSequences() {
  if(sequenceRetryMs && (int32_t)(millis()-sequenceRetryMs)<0) return false;
  Preferences prefs;
  bool ok=prefs.begin("wdh-txseq",false);
  uint32_t first=0,limit=0;
  if(ok) {
    first=prefs.getUInt("next",1);
    limit=first+SEQUENCE_RESERVATION_SIZE;
    ok=prefs.putUInt("next",limit)==sizeof(uint32_t);
    prefs.end();
  }
  if(!ok) {
    sequenceReservationFailures++;
    sequenceRetryMs=millis()+30000UL;
    return false; // do not acknowledge/send records with a reused identity
  }
  nextSeq=first; sequenceLimit=limit;
  sequenceReservationReady=true; sequenceRetryMs=0;
  return true;
}
static uint32_t lastTxMs = 0;

static inline Rec& qAt(uint8_t i);
static bool sendRecord(const Rec& r);
static void enqueueRecord(const char* line, size_t len, uint8_t recordType = MSG_TEXT);

static inline Rec& qAt(uint8_t i) { return q[(uint8_t)((qHead + i) % QCAP)]; }

static void enqueueRecord(const char* line, size_t len, uint8_t recordType) {
  if (qCount >= QCAP || len == 0 || len >= sizeof(q[0].text)) { droppedRecords++; return; }
  if ((!sequenceReservationReady || nextSeq==sequenceLimit) && !reserveRecordSequences()) {
    droppedRecords++; return;
  }
  Rec& r = qAt(qCount);
  r.seq = nextSeq++;
  r.lastTxMs = 0;
  r.tries = 0;
  r.len = (uint8_t)len;
  r.acked = false;
  r.recordType = recordType;
  memcpy(r.text, line, len);
  r.text[len] = '\0';
  qCount++;
  if (qCount > queueHighWater) queueHighWater = qCount;
}

// Promiscuous management-frame capture. Callback only copies compact observations.
struct PromiscObs { uint8_t bssid[6]; int8_t rssi; uint8_t channel; uint8_t authOpen; char ssid[33]; };
static constexpr uint8_t PROMISC_QCAP = 48;
static PromiscObs promiscQ[PROMISC_QCAP];
static volatile uint8_t promiscHead=0, promiscTail=0;
static volatile uint32_t promiscSeen=0, promiscQueued=0, promiscDropped=0;
static uint32_t primarySniffSteps=0, activeFallbackSteps=0, channelSetFailures=0;
static portMUX_TYPE promiscMux = portMUX_INITIALIZER_UNLOCKED;

static void IRAM_ATTR onPromisc(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (!buf || type != WIFI_PKT_MGMT) return;
  const wifi_promiscuous_pkt_t* p=(const wifi_promiscuous_pkt_t*)buf;
  const uint8_t* f=p->payload; const uint16_t sigLen=p->rx_ctrl.sig_len;
  if (sigLen < 36 || sigLen > 2304) return;
  const uint8_t subtype=f[0]&0xF0;
  if (subtype!=0x80 && subtype!=0x50) return;
  PromiscObs o={}; memcpy(o.bssid,f+16,6); o.rssi=(int8_t)p->rx_ctrl.rssi; o.channel=(uint8_t)p->rx_ctrl.channel;
  const uint16_t cap=(uint16_t)f[34]|((uint16_t)f[35]<<8); o.authOpen=(cap&0x0010)?0:1;
  size_t pos=36;
  while(pos+2<=sigLen){ uint8_t id=f[pos],len=f[pos+1]; pos+=2; if(pos+len>sigLen)break;
    if(id==0){ uint8_t n=len>32?32:len; for(uint8_t i=0;i<n;i++){char c=(char)f[pos+i];o.ssid[i]=(c==','||(uint8_t)c<32||(uint8_t)c==127)?'_':c;} o.ssid[n]='\0';}
    else if ((id==3 || id==61) && len>=1) {
      const uint8_t advertised=f[pos];
      if ((advertised>=1 && advertised<=14) || (advertised>=36 && advertised<=177)) o.channel=advertised;
    }
    pos+=len; }
  portENTER_CRITICAL_ISR(&promiscMux); promiscSeen++;
  uint8_t i=promiscTail;
  while(i!=promiscHead){ if(memcmp(promiscQ[i].bssid,o.bssid,6)==0){ if(o.rssi>promiscQ[i].rssi)promiscQ[i].rssi=o.rssi; portEXIT_CRITICAL_ISR(&promiscMux); return;} i=(uint8_t)((i+1)%PROMISC_QCAP); }
  uint8_t nx=(uint8_t)((promiscHead+1)%PROMISC_QCAP);
  if(nx==promiscTail)promiscDropped++; else {promiscQ[promiscHead]=o;promiscHead=nx;promiscQueued++;}
  portEXIT_CRITICAL_ISR(&promiscMux);
}
static void drainPromiscQueue(uint8_t budget=24){
  while(budget-- && qCount < QCAP){PromiscObs o={};bool have=false;portENTER_CRITICAL(&promiscMux);if(promiscTail!=promiscHead){o=promiscQ[promiscTail];promiscTail=(uint8_t)((promiscTail+1)%PROMISC_QCAP);have=true;}portEXIT_CRITICAL(&promiscMux);if(!have)break;
    char line[96];int L=snprintf(line,sizeof(line),"%02X:%02X:%02X:%02X:%02X:%02X,%s,%s,%u,%d",o.bssid[0],o.bssid[1],o.bssid[2],o.bssid[3],o.bssid[4],o.bssid[5],o.ssid,o.authOpen?"OPEN":"SECURED",(unsigned)o.channel,(int)o.rssi);
    if(L<=0||L>=(int)sizeof(line))continue;uint8_t before=qCount;enqueueRecord(line,(size_t)L);if(qCount>before){wifiRecords++;lastWiFiObservationMs=millis();}
  }
}
static void setPromiscCapture(bool on){
  if(on){wifi_promiscuous_filter_t f={};f.filter_mask=WIFI_PROMIS_FILTER_MASK_MGMT;esp_wifi_set_promiscuous_filter(&f);esp_wifi_set_promiscuous_rx_cb(onPromisc);esp_wifi_set_promiscuous(true);}
  else esp_wifi_set_promiscuous(false);
}

static void markAcked(uint32_t seq) {
  for (uint8_t i = 0; i < qCount; i++) {
    if (qAt(i).seq == seq) { if (!qAt(i).acked) deliveredRecords++; qAt(i).acked = true; return; }
  }
}

static void drainAcks() {
  while (ackTail != ackHead) {
    uint32_t s = ackRing[ackTail];
    ackTail = (uint8_t)((ackTail + 1) & 15);
    markAcked(s);
  }
  while (qCount && qAt(0).acked) { qHead = (uint8_t)((qHead + 1) % QCAP); qCount--; }
}

// ---------------- Radio helpers ----------------
static bool bandIsAuto = false;

// 2.4 GHz-only while idle / talking ESP-NOW (same radio state as the hub); AUTO only for 5 GHz scans.
static void setBandAuto(bool wantAuto) {
#if CONFIG_SOC_WIFI_SUPPORT_5G
  if (bandIsAuto == wantAuto) return;
  WiFi.setBandMode(wantAuto ? WIFI_BAND_MODE_AUTO : WIFI_BAND_MODE_2G_ONLY);
  bandIsAuto = wantAuto;
#endif
}

static uint8_t currentChannel() {
  uint8_t c = 0; wifi_second_chan_t s = WIFI_SECOND_CHAN_NONE;
  esp_wifi_get_channel(&c, &s);
  return c;
}

// Park on the ESP-NOW home channel, verify it stuck, retry once if it did not.
static void goHome() {
  esp_wifi_set_channel(HOME_CH, WIFI_SECOND_CHAN_NONE);
  if (currentChannel() != HOME_CH) {
    delay(2);
    esp_wifi_set_channel(HOME_CH, WIFI_SECOND_CHAN_NONE);
  }
}

static bool addPeer(const uint8_t* mac) {
  if (esp_now_is_peer_exist(mac)) return true;
  esp_now_peer_info_t p = {};
  memcpy(p.peer_addr, mac, 6);
  p.channel = 0;            // 0 = follow the current channel
  p.ifidx = WIFI_IF_STA;
  p.encrypt = false;
  return esp_now_add_peer(&p) == ESP_OK;
}

static inline void noteTx(esp_err_t e) {
  if (e != ESP_OK) { txFail++; lastTxErr = e; }
}

static void sendCoreRequest() {
  static uint32_t ctr = 0;
  jcmk_text_msg_t m = {};
  memcpy(m.magic, MAGIC, 4);
  m.type = MSG_CORE_REQUEST;
  m.counter = ++ctr;
  const char* t = "C5NODE";
  m.len = strlen(t);
  memcpy(m.text, t, m.len);
  m.text[m.len] = '\0';
  reqSent++;
  noteTx(esp_now_send(BCAST, (const uint8_t*)&m, offsetof(jcmk_text_msg_t, text) + m.len + 1));
}

static void sendCapabilities() {
  if (!hubPeerReady) return;
  jcmk_capability_msg_t c = {};
  memcpy(c.magic, MAGIC, 4);
  c.type = MSG_CAPABILITIES;
  c.version = RELIABLE_VERSION;
  c.capabilities = CAP_RELIABLE_ACK | CAP_WIFI_2G | CAP_WIFI_5G | CAP_BLE_SCAN;
  noteTx(esp_now_send(hubMac, (const uint8_t*)&c, sizeof(c)));
}

static void sendStatus() {
  if (!hubPeerReady) return;
  jcmk_node_status_msg_t s = {};
  memcpy(s.magic, MAGIC, 4);
  s.type = MSG_NODE_STATUS;
  s.version = 1;
  s.scan_mode = 0;
  s.flags = (runEnabled ? ST_RUN_ACK : 0) | ((runEnabled && hubPeerReady && lastScanProgressMs &&
    (uint32_t)(millis() - lastScanProgressMs) < 15000UL) ? ST_SCANNING : 0);
  s.node_index = adminIndex;
  s.node_count = adminCount;
  s.start_channel_idx = startIdx;
  s.end_channel_idx = endIdx;
  s.wifi_records = wifiRecords;
  s.ble_records = bleRecords;
  s.scan_passes = scanPasses;
  s.config_generation = cfgGeneration;
  noteTx(esp_now_send(hubMac, (const uint8_t*)&s, sizeof(s)));
}

static bool sendRecord(const Rec& r) {
  if (!hubPeerReady) return false;
  jcmk_reliable_text_msg_t m = {};
  memcpy(m.magic, MAGIC, 4);
  m.type = MSG_RELIABLE_TEXT;
  m.version = RELIABLE_VERSION;
  m.record_type = r.recordType;      // Preserve Wi-Fi vs BLE record type for the Core
  m.sequence = r.seq;
  m.len = r.len;
  memcpy(m.text, r.text, r.len);
  m.text[r.len] = '\0';
  esp_err_t e = esp_now_send(hubMac, (const uint8_t*)&m,
                             offsetof(jcmk_reliable_text_msg_t, text) + r.len + 1);
  noteTx(e);
  return e == ESP_OK;
}

// One frame per call, oldest due record first, bounded in-flight window.
static void pumpTx(uint32_t now) {
  if ((uint32_t)(now - lastTxMs) < TX_GAP_MS) return;
  uint8_t lim = qCount < TX_WINDOW ? qCount : TX_WINDOW;
  for (uint8_t i = 0; i < lim; i++) {
    Rec& r = qAt(i);
    if (r.acked) continue;
    if (r.tries && (uint32_t)(now - r.lastTxMs) < RETRY_MS) continue;
    if (r.tries >= MAX_TRIES) { r.acked = true; lostRecords++; continue; }
    if (sendRecord(r)) {
      if (r.tries) retryCount++;
      r.tries++;
      r.lastTxMs = now;
    }
    lastTxMs = now; // throttle immediate driver errors as well
    return;
  }
}

// ---------------- ESP-NOW receive (Wi-Fi task: copy and flag only, never transmit) ----------------
static void onRecv(const esp_now_recv_info_t* info, const uint8_t* d, int len) {
  if (!info || !d || len < 5 || memcmp(d, MAGIC, 4) != 0) return;
  const uint8_t type = d[4];
  const uint8_t* src = info->src_addr;

  rxFrames++;
  rxLastType = type;

  if (type == MSG_CORE_REPLY) {
    rxCoreReply++;
    if (!hubKnown) {
      // Accept the hub right here so the ADMIN / NODE_CONFIG frames that follow are not dropped.
      memcpy(hubMac, src, 6);
      hubPeerReady = false;
      needPeer = true;
      lastHubRxMs = millis();
      hubKnown = true;
    } else if (memcmp(src, hubMac, 6) == 0) {
      lastHubRxMs = millis();
      sendCapsNow = true;
      sendStatusNow = true;
    }
    return;
  }

  if (!hubKnown || memcmp(src, hubMac, 6) != 0) { rxOther++; return; }
  lastHubRxMs = millis();

  switch (type) {
    case MSG_ADMIN:
      rxAdmin++;
      if (len >= (int)sizeof(jcmk_admin_msg_t)) {
        const jcmk_admin_msg_t* a = (const jcmk_admin_msg_t*)d;
        if (a->start_channel_idx <= a->end_channel_idx && a->end_channel_idx < NUM_CHANNELS) {
          startIdx = a->start_channel_idx;
          endIdx = a->end_channel_idx;
          adminIndex = a->node_index;
          adminCount = a->node_count;
          sendStatusNow = true;
        }
      }
      break;
    case MSG_NODE_CONFIG:
      rxConfig++;
      if (len >= (int)sizeof(jcmk_node_config_msg_t)) {
        const jcmk_node_config_msg_t* c = (const jcmk_node_config_msg_t*)d;
        if (c->version != 1) break;
        uint32_t iv = c->scan_interval_ms;
        if (iv < 150) iv = 150;
        if (iv > 30000) iv = 30000;
        scanIntervalMs = iv;
        configReceived = true;
        runEnabled = (c->run_enabled != 0);
        cfgGeneration = c->generation;
        sendStatusNow = true;
      }
      break;
    case MSG_RECORD_ACK:
      rxAck++;
      if (len >= (int)sizeof(jcmk_record_ack_msg_t)) {
        const jcmk_record_ack_msg_t* a = (const jcmk_record_ack_msg_t*)d;
        if (a->version != RELIABLE_VERSION) break;
        uint8_t nx = (uint8_t)((ackHead + 1) & 15);
        if (nx != ackTail) { ackRing[ackHead] = a->sequence; ackHead = nx; }
      }
      break;
    default:   // HEARTBEAT, GPS_STATUS, CAPABILITIES, TEXT: only used as a liveness signal
      rxOther++;
      break;
  }
}

// ---------------- Scanning ----------------
static bool channelAllowed(uint8_t ch) {
  return (ch <= 14) ? (ch <= MAX_2G_CH) : (ch <= MAX_5G_CH);
}

static const char* authName(wifi_auth_mode_t m) {
  switch (m) {
    case WIFI_AUTH_OPEN:            return "OPEN";
    case WIFI_AUTH_WEP:             return "WEP";
    case WIFI_AUTH_WPA_PSK:         return "WPA";
    case WIFI_AUTH_WPA2_PSK:        return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-E";
    case WIFI_AUTH_WPA3_PSK:        return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/WPA3";
    default:                        return "SECURED";
  }
}

static void reportResults(int n) {
  for (int i = 0; i < n; i++) {
    const uint8_t* b = WiFi.BSSID(i);
    if (!b) continue;
    // The hub splits on commas, so scrub them (and control chars) out of the SSID.
    char ssid[33];
    String s = WiFi.SSID(i);
    size_t sl = s.length() > 32 ? 32 : s.length();
    for (size_t k = 0; k < sl; k++) {
      char c = s[k];
      ssid[k] = (c == ',' || c < 32 || c == 127) ? '_' : c;
    }
    ssid[sl] = '\0';

    char line[96];
    int L = snprintf(line, sizeof(line), "%02X:%02X:%02X:%02X:%02X:%02X,%s,%s,%d,%d",
                     b[0], b[1], b[2], b[3], b[4], b[5], ssid,
                     authName(WiFi.encryptionType(i)), (int)WiFi.channel(i), (int)WiFi.RSSI(i));
    if (L <= 0 || L >= (int)sizeof(line)) continue;
    const uint8_t before = qCount;
    enqueueRecord(line, (size_t)L);
    if (qCount > before) {
      wifiRecords++;
      lastWiFiObservationMs = millis();
      // Observational history only. No BSSID is ever dropped by this cache.
      diRemember(((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
                 ((uint32_t)b[2] << 8) | b[3]);
    }
  }
}

static uint32_t lastStepStartMs = 0, lastStepEndMs = 0;

static void scanStep() {
  static uint8_t idx = 0xFF;
  uint8_t s = startIdx, e = endIdx;
  if (e >= NUM_CHANNELS) e = NUM_CHANNELS - 1;
  if (s > e) s = e;
  if (idx == 0xFF || idx < s || idx > e) idx = s;

  uint8_t ch = 0;
  for (uint8_t tries = 0; tries < NUM_CHANNELS; tries++) {
    ch = CHANNELS[idx];
    idx++;
    if (idx > e) { idx = s; scanPasses++; lastScanProgressMs = millis(); }
    if (channelAllowed(ch)) break;
    ch = 0;
  }
  if (!ch) return;
  diagWifiPasses++;

  const bool dfs = (ch >= 52 && ch <= 144);   // passive on DFS, active elsewhere
  setBandAuto(ch > 14);                        // AUTO only while a 5 GHz channel is being scanned
  // Promiscuous capture is PRIMARY: tune to one channel and listen without
  // running WiFi.scanNetworks(). Keep a short, bounded off-home window.
  // The occasional active scan retains detailed auth information and helps
  // find quiet APs. DFS channels remain passive only.
  static uint8_t channelVisits[NUM_CHANNELS] = {};
  static bool visitsInitialized=false;
  if(!visitsInitialized) {
    for(uint8_t i=0;i<NUM_CHANNELS;i++) channelVisits[i]=i%ACTIVE_FALLBACK_EVERY;
    visitsInitialized=true;
  }
  uint8_t visitIndex=0;
  while (visitIndex < NUM_CHANNELS && CHANNELS[visitIndex] != ch) ++visitIndex;
  const bool fallback = visitIndex < NUM_CHANNELS &&
    wdh::activeVisit(channelVisits[visitIndex], ACTIVE_FALLBACK_EVERY, dfs);
  int n = 0;
  if (fallback) {
    // Periodic active-scanning pass for enrichment.
    ++activeFallbackSteps;
    n = WiFi.scanNetworks(false, true, false, ACTIVE_DWELL_MS, ch);
  } else {
    const esp_err_t tuned = esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
    if (tuned == ESP_OK && currentChannel() == ch) {
      ++primarySniffSteps;
      // Fixed 75 ms for every passive channel, including DFS.
      // No adaptive dwell counters or busy/quiet tracking.
      setPromiscCapture(true);
      delay(PROMISC_DWELL_MS);
      setPromiscCapture(false);
    } else {
      ++channelSetFailures;
      ++diagWiFiChannelFailures;
      // Do not mislabel home-channel traffic as a different scan channel.
    }
  }
  setBandAuto(false);                          // 2.4 GHz radio state for ESP-NOW
  goHome();                                    // ESP-NOW rendezvous on channel 6
  if (n > 0) reportResults(n);
  if (fallback) WiFi.scanDelete();
  drainPromiscQueue(32);
}

// Nonblocking NimBLE scan while parked on the ESP-NOW home channel.
// BLE advertisements are copied into a bounded ring; only loop() enqueues
// reliable records. Never transmit ESP-NOW from a BLE callback.
static bool comboBleReady = false;
static bool comboBleActive = false;
static uint32_t comboBleStartedMs = 0;
static uint32_t comboBleStartFailures = 0;
static uint32_t comboLastBleMs = 0;
static uint32_t comboBlePasses = 0;
static volatile uint32_t comboBleDropped = 0;
// Interleave short BLE bursts between Wi-Fi channel visits, not after a long idle wait.
static constexpr uint32_t COMBO_BLE_MIN_GAP_MS = 1800;  // minimum time between bursts
static constexpr uint32_t COMBO_BLE_MAX_GAP_MS = 6500;  // prioritize BLE when opportunities are scarce
static constexpr uint32_t COMBO_BLE_BURST_MS = 300;     // 300 ms interleaved BLE burst
static constexpr uint8_t COMBO_BLE_WIFI_STEPS = 4;     // normally 4 Wi-Fi visits per BLE burst
static constexpr uint32_t COMBO_BLE_HOME_MS = 60;      // let ESP-NOW run before BLE
static uint8_t comboWifiStepsSinceBle = 0;
struct ComboBleItem { char mac[18]; char name[49]; int16_t rssi; };
static ComboBleItem comboBleQueue[24];
static volatile uint8_t comboBleHead = 0, comboBleTail = 0;
static portMUX_TYPE comboBleMux = portMUX_INITIALIZER_UNLOCKED;
class ComboBleCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* d) override {
    if (!hubKnown || !runEnabled) return;
    ComboBleItem x = {};
    const auto mac = d->getAddress().toString();
    memcpy(x.mac, mac.c_str(), 17); x.mac[17]=0;
    if (d->haveName()) {
      const auto name = d->getName();
      wdh::copyBleName(x.name, sizeof(x.name), name.data(), name.size());
    }
    x.rssi = d->getRSSI();
    portENTER_CRITICAL(&comboBleMux);
    uint8_t next = (uint8_t)((comboBleHead + 1) % 24);
    if (next != comboBleTail) { comboBleQueue[comboBleHead] = x; comboBleHead = next; }
    else comboBleDropped++;
    portEXIT_CRITICAL(&comboBleMux);
  }
};
static ComboBleCallbacks comboBleCallbacks;
static void comboDrainBle() {
  for (uint8_t n = 0; n < 12 && qCount < QCAP; n++) {
    ComboBleItem x; bool have = false;
    portENTER_CRITICAL(&comboBleMux);
    if (comboBleTail != comboBleHead) {
      x = comboBleQueue[comboBleTail];
      comboBleTail = (uint8_t)((comboBleTail + 1) % 24);
      have = true;
    }
    portEXIT_CRITICAL(&comboBleMux);
    if (!have) break;
    char line[96];
    int len = snprintf(line, sizeof(line), "%s,%s,%d,B", x.mac, x.name, (int)x.rssi);
    if (len > 0 && len < (int)sizeof(line)) {
      uint8_t before = qCount;
      enqueueRecord(line, (size_t)len, MSG_BLE_RECORD);
      if (qCount > before) { bleRecords++; lastBleObservationMs = millis(); }
    }
  }
}
static void comboScanBle() {
  if (comboBleActive || !hubPeerReady || !runEnabled || qCount >= QCAP / 2) { diagBleSkipped++; return; }
  diagBleAttempts++;
  if (!comboBleReady) {
    if (ESP.getFreeHeap() < 35000) return;
    NimBLEDevice::init("WDH-C5-DualBand-BLE");
    NimBLEScan* scanner = NimBLEDevice::getScan();
    if (!scanner) return;
    scanner->setScanCallbacks(&comboBleCallbacks, true);
    scanner->setActiveScan(false);
    scanner->setInterval(160);
    scanner->setWindow(48);
    scanner->setMaxResults(0); // callback-only: do not retain a second device list
    comboBleReady = true;
  }
  goHome();
  NimBLEScan* scanner = NimBLEDevice::getScan();
  if (!scanner) return;
  // start() is asynchronous: loop() drains observations and ACKs during the burst.
  if (!scanner->start(COMBO_BLE_BURST_MS, false)) {
    comboBleStartFailures++;
    return;
  }
  comboBleStartedMs=millis();
  comboBleActive=true;
}

// Only loop() owns scan lifecycle; callbacks only copy observations.
static void comboBleTick() {
  if (!comboBleActive) return;
  NimBLEScan* scanner=NimBLEDevice::getScan();
  if (!scanner) { comboBleActive=false; return; }
  const bool cancel=!runEnabled || !hubKnown || !hubPeerReady || qCount >= QCAP*3/4;
  const bool overdue=(uint32_t)(millis()-comboBleStartedMs) >= COMBO_BLE_BURST_MS+500UL;
  if ((cancel || overdue) && scanner->isScanning()) scanner->stop();
  if (scanner->isScanning()) return;
  comboBleActive=false;
  comboBlePasses++;
  scanner->clearResults();
  goHome();
}

// ---------------- Setup / loop ----------------
static void dropHub() {
  hubKnown = false;
  comboBleTick(); // cancel any scan before changing peers/radio state
  hubPeerReady = false;
  needPeer = false;
  runEnabled = false;
  configReceived = false;
  lastConfigRequestMs = 0;
  // Preserve unacknowledged records during brief radio outages. The hub may
  // have received a record whose ACK was lost; its reliable dedupe handles retries.
  for (uint8_t i = 0; i < qCount; ++i) {
    Rec& r = qAt(i);
    if (!r.acked) { r.tries = 0; r.lastTxMs = 0; }
  }
  ackHead.store(0); ackTail.store(0);
  reconnects++;
  esp_now_del_peer(hubMac);
  setBandAuto(false);
  goHome();
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.printf("\n[NODE] WardNode C5 v%s\n", NODE_FW_VERSION);

  if(!reserveRecordSequences()) Serial.println("[SEQ] NVS reservation failed; records blocked until retry");
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
#if CONFIG_SOC_WIFI_SUPPORT_5G
  WiFi.setBandMode(WIFI_BAND_MODE_2G_ONLY);   // identical radio state to the hub while joining
  bandIsAuto = false;
#endif
  esp_wifi_set_ps(WIFI_PS_NONE);
  goHome();

  // Retry recoverable startup failures rather than hanging forever until unplugged.
  bool radioStarted = false;
  for (uint8_t attempt = 0; attempt < 5; ++attempt) {
    esp_err_t rc = esp_now_init();
    if (rc == ESP_OK) { radioStarted = true; break; }
    Serial.printf("[NODE] ESP-NOW init attempt %u failed (%d)\n", (unsigned)(attempt+1), (int)rc);
    delay(300UL * (attempt + 1));
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    goHome();
  }
  if (!radioStarted) {
    Serial.println("[NODE] Radio startup failed; restarting to retry cleanly");
    delay(1500);
    ESP.restart();
    return;
  }
  esp_now_register_recv_cb(onRecv);
  addPeer(BCAST);
  goHome();   // belt and braces: some cores reset the channel inside esp_now_init()
  Serial.printf("[NODE] MAC %s, home channel %u (read back: %u)\n",
                WiFi.macAddress().c_str(), HOME_CH, currentChannel());
  Serial.println("[NODE] Make sure the XIAO's FPC antenna is plugged into the U.FL connector.");
  // PSRAM is optional on ESP32-C5 hardware. Keep all radio-critical queues in internal RAM.
  if (heap_caps_get_total_size(MALLOC_CAP_SPIRAM) >= DI_HISTORY_CAP * sizeof(uint32_t)) {
    diHistory = static_cast<uint32_t*>(heap_caps_calloc(DI_HISTORY_CAP, sizeof(uint32_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    diPsramHistoryReady = diHistory != nullptr;
  }
  Serial.printf("[DI] optional PSRAM history=%s, internal free=%u, PSRAM free=%u\n",
                diPsramHistoryReady ? "ENABLED" : "OFF (safe fallback)",
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

void loop() {
  uint32_t now = millis();
  static uint32_t nextReqMs = 0, lastCapsMs = 0, lastStatusMs = 0, lastLogMs = 0, lastJoinLogMs = 0;

  // Hub accepted by the callback -> create the ESP-NOW peer, then announce ourselves.
  if (hubKnown && needPeer) {
    bool ok = addPeer(hubMac);
    needPeer = false;
    hubPeerReady = ok;
    lastConfigRequestMs = now;
    lastHubRxMs = now;
    sendCapsNow = true;
    sendStatusNow = true;
    Serial.printf("[JOIN] Hub %02X:%02X:%02X:%02X:%02X:%02X accepted, peer %s\n",
                  hubMac[0], hubMac[1], hubMac[2], hubMac[3], hubMac[4], hubMac[5],
                  ok ? "added" : "ADD FAILED");
  }

  drainAcks();
  now=millis(); // RX callbacks may have refreshed liveness since loop entry

  const uint32_t lastHubSeen=lastHubRxMs;
  if (hubKnown && (uint32_t)(millis() - lastHubSeen) > HUB_LOST_MS) {
    Serial.println("[JOIN] Hub lost, rejoining");
    dropHub();
  }

  if (!hubKnown) {
    if ((int32_t)(now - nextReqMs) >= 0) {
      nextReqMs = now + 400 + (esp_random() % 300);
      if (currentChannel() != HOME_CH) {
        Serial.printf("[JOIN] channel drifted to %u, resetting to %u\n", currentChannel(), HOME_CH);
      }
      goHome();
      sendCoreRequest();
    }
    if ((uint32_t)(now - lastJoinLogMs) >= 5000) {
      lastJoinLogMs = now;
      Serial.printf("[JOIN] waiting for CORE_REPLY: req=%lu txfail=%lu(err %d) ch=%u | heard ENOW frames=%lu lastType=%u\n",
                    (unsigned long)reqSent, (unsigned long)txFail, (int)lastTxErr, currentChannel(),
                    (unsigned long)rxFrames, (unsigned)rxLastType);
    }
    delay(2);
    return;
  }

  if (!hubPeerReady) {
    // A failed peer add must not strand an otherwise connected node.
    static uint32_t lastPeerRetryMs = 0;
    if ((uint32_t)(now - lastPeerRetryMs) >= 2000) {
      lastPeerRetryMs = now;
      hubPeerReady = addPeer(hubMac);
      if (hubPeerReady) { sendCapsNow = true; sendStatusNow = true; }
    }
    delay(2); return;
  }

  if (sendCapsNow || (uint32_t)(now - lastCapsMs) >= 10000) {
    sendCapsNow = false; lastCapsMs = now;
    sendCapabilities();
  }
  if (sendStatusNow || (uint32_t)(now - lastStatusMs) >= 2000) {
    sendStatusNow = false; lastStatusMs = now;
    sendStatus();
  }

  // If the initial NODE_CONFIG was lost, do not remain connected but idle forever.
  // Re-request Core configuration until it arrives. Never scan provisionally;
  // an explicit RUN/PAUSE/STOP configuration controls scanning.
  if (!configReceived && (uint32_t)(now - lastConfigRequestMs) >= CONFIG_RETRY_MS) {
    lastConfigRequestMs = now;
    goHome();
    sendCoreRequest();
    sendCapabilities();
    sendStatus();
  }
  // No autonomous scan fallback: Core NODE_CONFIG controls RUN/PAUSE.
  // Continue requesting config until received; do not scan on link alone.

  drainPromiscQueue(8); // finish passive observations while parked at home
  comboDrainBle();
  comboBleTick();
  pumpTx(now);

  // Interleaved BLE: after ~4 Wi-Fi channel visits, use the NEXT home-channel
  // opportunity for a short BLE burst. Do not require an empty TX queue: that
  // starved BLE in busy areas. Protect Wi-Fi and ESP-NOW by requiring spare queue
  // capacity, a minimum gap, and a short home-channel ACK window.
  const uint32_t sinceBle = (uint32_t)(now - comboLastBleMs);
  const bool bleDue = comboWifiStepsSinceBle >= COMBO_BLE_WIFI_STEPS ||
                      sinceBle >= COMBO_BLE_MAX_GAP_MS;
  const bool bleSlot =
      runEnabled && hubPeerReady && !comboBleActive && bleDue &&
      sinceBle >= COMBO_BLE_MIN_GAP_MS &&
      qCount < QCAP / 2 &&
      (uint32_t)(now - lastStepEndMs) >= COMBO_BLE_HOME_MS;
  if (bleSlot) {
    comboLastBleMs = now;
    comboWifiStepsSinceBle = 0;
    comboScanBle();
    now = millis();
    pumpTx(now);
  }

  if (runEnabled && !comboBleActive) {
    const bool stepDue = (uint32_t)(now - lastStepStartMs) >= scanIntervalMs;
    const uint32_t homeFor = (uint32_t)(now - lastStepEndMs);
    // Park on the home channel long enough to hear ACKs and drain the queue, but never too long.
    const bool homeOk = homeFor >= HOME_MIN_MS && (qCount == 0 || homeFor >= HOME_MAX_MS);
    if (stepDue && homeOk && qCount < (QCAP * 3 / 4)) {
      lastStepStartMs = now;
      scanStep();
      lastStepEndMs = millis();
      lastScanProgressMs = lastStepEndMs;
      sendStatusNow = true; // Report scan progress promptly after a completed channel visit.
      if (comboWifiStepsSinceBle < 255) ++comboWifiStepsSinceBle;
    }
  }

  if ((uint32_t)(now - lastLogMs) >= 5000) {
    lastLogMs = now;
    Serial.printf("[NODE] run=%u iv=%lu slice=%u-%u ch=%u rec=%lu passes=%lu q=%u retry=%lu lost=%lu drop=%lu txfail=%lu | rx adm=%lu cfg=%lu ack=%lu\n",
                  (unsigned)runEnabled, (unsigned long)scanIntervalMs, (unsigned)startIdx, (unsigned)endIdx,
                  currentChannel(), (unsigned long)(wifiRecords+bleRecords), (unsigned long)scanPasses, (unsigned)qCount,
                  (unsigned long)retryCount, (unsigned long)lostRecords, (unsigned long)droppedRecords,
                  (unsigned long)txFail, (unsigned long)rxAdmin, (unsigned long)rxConfig, (unsigned long)rxAck);
  }

  static uint32_t lastDiLogMs = 0;
  if ((uint32_t)(now - lastDiLogMs) >= 10000) {
    lastDiLogMs = now;
    Serial.printf("[DI] WiFiSteps=%lu BLEattempts=%lu BLEpasses=%lu BLEskipped=%lu WiFiErr=%lu RXqueued=%lu RXdropped=%lu queueHigh=%lu heap=%u history=%u\n",
      (unsigned long)diagWifiPasses, (unsigned long)diagBleAttempts,
      (unsigned long)comboBlePasses, (unsigned long)diagBleSkipped,
      (unsigned long)diagWiFiChannelFailures, (unsigned long)promiscQueued,
      (unsigned long)promiscDropped, (unsigned long)queueHighWater,
      (unsigned)ESP.getFreeHeap(), (unsigned)diPsramHistoryReady);
  }

  static uint32_t lastDeliveryLogMs = 0;
  if ((uint32_t)(now - lastDeliveryLogMs) >= 15000) {
    lastDeliveryLogMs = now;
    Serial.printf("[DELIVERY] accepted=%lu acked=%lu queued=%u highwater=%lu reconnects=%lu dropped=%lu lost=%lu\n",
                  (unsigned long)(wifiRecords+bleRecords), (unsigned long)deliveredRecords,
                  (unsigned)qCount, (unsigned long)queueHighWater,
                  (unsigned long)reconnects, (unsigned long)droppedRecords,
                  (unsigned long)lostRecords);
  }
  static uint32_t lastBleHealthMs=0;
  if ((uint32_t)(millis()-lastBleHealthMs)>=15000UL) {
    lastBleHealthMs=millis();
    Serial.printf("[BLEHEALTH] callback_drops=%lu start_failures=%lu active=%u seq_reserve_fail=%lu\n",
      (unsigned long)comboBleDropped,(unsigned long)comboBleStartFailures,(unsigned)comboBleActive,
      (unsigned long)sequenceReservationFailures);
  }
  delay(1);
}
