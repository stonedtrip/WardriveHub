#pragma once
#include <stdint.h>
#include <stddef.h>

namespace wdh {
inline unsigned char folded(unsigned char c) {
  return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}
inline bool equalFolded(const char* a, const char* b) {
  if (!a) a="";
  if (!b) b="";
  while (*a && *b) {
    if (folded((unsigned char)*a++) != folded((unsigned char)*b++)) return false;
  }
  return *a == *b;
}
inline uint32_t addressHash(const char* a) {
  if (!a) a="";
  uint32_t h = 2166136261UL;
  while (*a) h = (h ^ folded((unsigned char)*a++)) * 16777619UL;
  return h;
}
// Index entries are history positions plus one. Full strings resolve collisions.
// No ownership/allocation; a missing index retains the original linear fallback.
inline bool containsAddress(const char* value, const char (*history)[18],
                            uint16_t used, const uint16_t* index, uint16_t slots) {
  if (!index || !slots) {
    for (uint16_t i=0; i<used; ++i) if (equalFolded(value, history[i])) return true;
    return false;
  }
  const uint32_t start = addressHash(value) % slots;
  for (uint16_t probe=0; probe<slots; ++probe) {
    const uint16_t entry=index[(start+probe)%slots];
    if (!entry) return false;
    if (entry <= used && equalFolded(value, history[entry-1])) return true;
  }
  return false;
}
inline void indexAddress(const char* value, uint16_t position, uint16_t* index, uint16_t slots) {
  if (!index || !slots) return;
  const uint32_t start=addressHash(value)%slots;
  for (uint16_t probe=0; probe<slots; ++probe) {
    uint16_t& entry=index[(start+probe)%slots];
    if (!entry) { entry=position+1; return; }
  }
}
inline bool activeVisit(uint8_t& visits, uint8_t every, bool passiveOnly) {
  if (passiveOnly || !every) return false;
  if (++visits < every) return false;
  visits=0;
  return true;
}
inline bool validPayload(size_t packet, size_t header, size_t declared, size_t maximum) {
  return packet >= header && declared <= maximum && declared <= packet-header;
}
inline void copyBleName(char* out, size_t capacity, const char* in, size_t length) {
  if (!capacity) return;
  size_t i=0;
  for (; i+1<capacity && i<length && in[i]; ++i) {
    const char c=in[i];
    out[i]=(c==',' || c=='\r' || c=='\n') ? ' ' : c;
  }
  out[i]=0;
}
// Unsigned distances avoid signed INT_MIN negation at the half-range boundary.
inline bool sequenceAccepted(uint32_t newest, uint32_t bitmap, uint32_t seq) {
  if (!bitmap) return false;
  const uint32_t ahead=seq-newest;
  if (ahead && ahead < 0x80000000UL) return false;
  const uint32_t behind=newest-seq;
  return behind >= 32U || (bitmap & (1UL << behind));
}
inline void acceptSequence(uint32_t& newest, uint32_t& bitmap, uint32_t seq) {
  if (!bitmap) { newest=seq; bitmap=1; return; }
  const uint32_t ahead=seq-newest;
  if (ahead && ahead < 0x80000000UL) {
    bitmap=ahead >= 32U ? 1UL : (bitmap << ahead) | 1UL;
    newest=seq;
  } else {
    const uint32_t behind=newest-seq;
    if (behind < 32U) bitmap |= 1UL << behind;
  }
}
} // namespace wdh
