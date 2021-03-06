#pragma once

#ifndef TSOUNDSAMPLE_INCLUDED
#define TSOUNDSAMPLE_INCLUDED

#include "tsound.h"

#undef DVAPI
#undef DVVAR
#ifdef TSOUND_EXPORTS
#define DVAPI DV_EXPORT_API
#define DVVAR DV_EXPORT_VAR
#else
#define DVAPI DV_IMPORT_API
#define DVVAR DV_IMPORT_VAR
#endif

// forward declarations
class TMono8SignedSample;
class TMono8UnsignedSample;
class TStereo8SignedSample;
class TStereo8UnsignedSample;
class TMono16Sample;
class TStereo16Sample;
class TMono24Sample;
class TStereo24Sample;
class TMono32Sample;
class TStereo32Sample;
class TMono32floatSample;
class TStereo32floatSample;

//==============================================================================

class DVAPI TMono8SignedSample {
  SCHAR value;

public:
  typedef SCHAR ChannelValueType;
  typedef TMono8SignedSample ChannelSampleType;

  TMono8SignedSample(SCHAR v = 0) : value(v) {}
  ~TMono8SignedSample(){};

  static bool isSampleSigned() { return true; }
  static int getBitPerSample() { return 8; }

  inline SCHAR getValue(TSound::Channel /*chan*/) const { return value; }

  inline void setValue(TSound::Channel /*chan*/, SCHAR v) { value = v; }

  inline double getPressure(TSound::Channel chan) const {
    return (getValue(chan));
  }

  inline TMono8SignedSample &operator+=(const TMono8SignedSample &s) {
    int ival = value + s.value;
    value    = tcrop(ival, (int)-128, (int)127);
    return *this;
  }

  inline TMono8SignedSample &operator*=(double a) {
    int ival = (int)(value * a);
    value    = tcrop(ival, (int)-128, (int)127);
    return *this;
  }

  inline TMono8SignedSample operator*(double a) { return (*this) *= a; }

  static TMono8SignedSample mix(const TMono8SignedSample &s1, double a1,
                                const TMono8SignedSample &s2, double a2) {
    return TMono8SignedSample(
        tcrop((int)(s1.value * a1 + s2.value * a2), -128, 127));
  }

  static inline TMono8SignedSample from(const TMono8SignedSample &sample);
  static inline TMono8SignedSample from(const TMono8UnsignedSample &sample);
  static inline TMono8SignedSample from(const TStereo8SignedSample &sample);
  static inline TMono8SignedSample from(const TStereo8UnsignedSample &sample);
  static inline TMono8SignedSample from(const TMono16Sample &sample);
  static inline TMono8SignedSample from(const TStereo16Sample &sample);
  static inline TMono8SignedSample from(const TMono24Sample &sample);
  static inline TMono8SignedSample from(const TStereo24Sample &sample);
  static inline TMono8SignedSample from(const TMono32Sample &sample);
  static inline TMono8SignedSample from(const TStereo32Sample &sample);
  static inline TMono8SignedSample from(const TMono32floatSample &sample);
  static inline TMono8SignedSample from(const TStereo32floatSample &sample);
};

inline TMono8SignedSample operator+(const TMono8SignedSample &s1,
                                    const TMono8SignedSample &s2) {
  TMono8SignedSample s = s1;
  return s += s2;
}

//==============================================================================

class DVAPI TMono8UnsignedSample {
  UCHAR value;

public:
  typedef UCHAR ChannelValueType;
  typedef TMono8UnsignedSample ChannelSampleType;

  TMono8UnsignedSample(UCHAR v = 127) : value(v) {}
  ~TMono8UnsignedSample(){};

  static bool isSampleSigned() { return false; }
  static int getBitPerSample() { return 8; }

  inline UCHAR getValue(TSound::Channel /*chan*/) const { return value; }

  inline void setValue(TSound::Channel /*chan*/, UCHAR v) { value = v; }

  inline double getPressure(TSound::Channel chan) const {
    return (getValue(chan) - 128);
  }

  inline TMono8UnsignedSample &operator+=(const TMono8UnsignedSample &s) {
    value = tcrop((int)(value - 128 + s.value), 0, 255);
    return *this;
  }

  inline TMono8UnsignedSample &operator*=(double a) {
    value = tcrop((int)((value - 128) * a + 128), 0, 255);
    return *this;
  }

  inline TMono8UnsignedSample operator*(double a) {
    int ival = tcrop((int)((value - 128) * a + 128), 0, 255);
    return TMono8UnsignedSample(ival);
  }

  static TMono8UnsignedSample mix(const TMono8UnsignedSample &s1, double a1,
                                  const TMono8UnsignedSample &s2, double a2) {
    return TMono8UnsignedSample(tcrop(
        (int)((s1.value - 128) * a1 + (s2.value - 128) * a2 + 128), 0, 255));
  }

  static inline TMono8UnsignedSample from(const TMono8SignedSample &sample);
  static inline TMono8UnsignedSample from(const TMono8UnsignedSample &sample);
  static inline TMono8UnsignedSample from(const TStereo8SignedSample &sample);
  static inline TMono8UnsignedSample from(const TStereo8UnsignedSample &sample);
  static inline TMono8UnsignedSample from(const TMono16Sample &sample);
  static inline TMono8UnsignedSample from(const TStereo16Sample &sample);
  static inline TMono8UnsignedSample from(const TMono24Sample &sample);
  static inline TMono8UnsignedSample from(const TStereo24Sample &sample);
  static inline TMono8UnsignedSample from(const TMono32Sample &sample);
  static inline TMono8UnsignedSample from(const TStereo32Sample &sample);
  static inline TMono8UnsignedSample from(const TMono32floatSample &sample);
  static inline TMono8UnsignedSample from(const TStereo32floatSample &sample);
};

inline TMono8UnsignedSample operator+(const TMono8UnsignedSample &s1,
                                      const TMono8UnsignedSample &s2) {
  TMono8UnsignedSample s = s1;
  return s += s2;
}

//==============================================================================

class DVAPI TStereo8SignedSample {
  SCHAR channel[2];  // l'ordine dei canali e' left,right

public:
  typedef SCHAR ChannelValueType;
  typedef TMono8SignedSample ChannelSampleType;

  TStereo8SignedSample(short v) {
    channel[0] = v >> 8;
    channel[1] = v & 0xFF;
  }

  TStereo8SignedSample(SCHAR lchan = 0, SCHAR rchan = 0) {
    channel[0] = lchan;
    channel[1] = rchan;
  }

  ~TStereo8SignedSample(){};

  static bool isSampleSigned() { return true; }
  static int getBitPerSample() { return 8; }

  inline SCHAR getValue(TSound::Channel chan) const {
    assert(chan <= 1);
    return channel[chan];
  }

  inline void setValue(TSound::Channel chan, SCHAR v) {
    assert(chan <= 1);
    channel[chan] = v;
  }

  inline double getPressure(TSound::Channel chan) const {
    return (getValue(chan));
  }

  inline TStereo8SignedSample &operator+=(const TStereo8SignedSample &s) {
    int iLeftVal  = channel[0] + s.channel[0];
    int iRightVal = channel[1] + s.channel[1];
    channel[0]    = tcrop(iLeftVal, (int)-128, (int)127);
    channel[1]    = tcrop(iRightVal, (int)-128, (int)127);
    return *this;
  }

  inline TStereo8SignedSample &operator*=(double a) {
    int iLeftVal  = (int)(a * channel[0]);
    int iRightVal = (int)(a * channel[1]);
    channel[0]    = tcrop(iLeftVal, (int)-128, (int)127);
    channel[1]    = tcrop(iRightVal, (int)-128, (int)127);
    return *this;
  }

  inline TStereo8SignedSample operator*(double a) {
    return TStereo8SignedSample(*this) *= a;
  }

  static TStereo8SignedSample mix(const TStereo8SignedSample &s1, double a1,
                                  const TStereo8SignedSample &s2, double a2) {
    return TStereo8SignedSample(
        tcrop((int)(s1.channel[0] * a1 + s2.channel[0] * a2), -128, 127),
        tcrop((int)(s1.channel[1] * a1 + s2.channel[1] * a2), -128, 127));
  }

  static inline TStereo8SignedSample from(const TMono8UnsignedSample &sample);
  static inline TStereo8SignedSample from(const TMono8SignedSample &sample);
  static inline TStereo8SignedSample from(const TStereo8SignedSample &sample);
  static inline TStereo8SignedSample from(const TStereo8UnsignedSample &sample);
  static inline TStereo8SignedSample from(const TMono16Sample &sample);
  static inline TStereo8SignedSample from(const TStereo16Sample &sample);
  static inline TStereo8SignedSample from(const TMono24Sample &sample);
  static inline TStereo8SignedSample from(const TStereo24Sample &sample);
  static inline TStereo8SignedSample from(const TMono32Sample &sample);
  static inline TStereo8SignedSample from(const TStereo32Sample &sample);
  static inline TStereo8SignedSample from(const TMono32floatSample &sample);
  static inline TStereo8SignedSample from(const TStereo32floatSample &sample);
};

inline TStereo8SignedSample operator+(const TStereo8SignedSample &s1,
                                      const TStereo8SignedSample &s2) {
  TStereo8SignedSample s = s1;
  return s += s2;
}

//=========================================================

class DVAPI TStereo8UnsignedSample {
  UCHAR channel[2];  // l'ordine dei canali e' left,right

public:
  typedef UCHAR ChannelValueType;
  typedef TMono8UnsignedSample ChannelSampleType;

  TStereo8UnsignedSample(short v) {
    channel[0] = v >> 8;
    channel[1] = v & 0xFF;
  }

  TStereo8UnsignedSample(UCHAR lchan = 127, UCHAR rchan = 127) {
    channel[0] = lchan;
    channel[1] = rchan;
  }

  ~TStereo8UnsignedSample(){};

  static bool isSampleSigned() { return false; }
  static int getBitPerSample() { return 8; }

  inline UCHAR getValue(TSound::Channel chan) const {
    assert(chan <= 1);
    return channel[chan];
  }

  inline void setValue(TSound::Channel chan, UCHAR v) {
    assert(chan <= 1);
    channel[chan] = v;
  }

  inline double getPressure(TSound::Channel chan) const {
    return (((int)getValue(chan)) - 128);
  }

  inline TStereo8UnsignedSample &operator+=(const TStereo8UnsignedSample &s) {
    int iLeftVal  = channel[0] + s.channel[0] - 128;
    int iRightVal = channel[1] + s.channel[1] - 128;
    channel[0]    = tcrop(iLeftVal, 0, 255);
    channel[1]    = tcrop(iRightVal, 0, 255);
    return *this;
  }

  inline TStereo8UnsignedSample &operator*=(double a) {
    int iLeftVal  = (int)(a * (channel[0] - 128));
    int iRightVal = (int)(a * (channel[1] - 128));
    channel[0]    = tcrop(iLeftVal + 128, 0, 255);
    channel[1]    = tcrop(iRightVal + 128, 0, 255);
    return *this;
  }

  inline TStereo8UnsignedSample operator*(double a) {
    return TStereo8UnsignedSample(*this) *= a;
  }

  static TStereo8UnsignedSample mix(const TStereo8UnsignedSample &s1, double a1,
                                    const TStereo8UnsignedSample &s2,
                                    double a2) {
    return TStereo8UnsignedSample(tcrop((int)((s1.channel[0] - 128) * a1 +
                                              (s2.channel[0] - 128) * a2 + 128),
                                        0, 255),
                                  tcrop((int)((s1.channel[1] - 128) * a1 +
                                              (s2.channel[1] - 128) * a2 + 128),
                                        0, 255));
  }

  static inline TStereo8UnsignedSample from(const TMono8UnsignedSample &sample);
  static inline TStereo8UnsignedSample from(const TMono8SignedSample &sample);
  static inline TStereo8UnsignedSample from(
      const TStereo8UnsignedSample &sample);
  static inline TStereo8UnsignedSample from(const TStereo8SignedSample &sample);
  static inline TStereo8UnsignedSample from(const TMono16Sample &sample);
  static inline TStereo8UnsignedSample from(const TStereo16Sample &sample);
  static inline TStereo8UnsignedSample from(const TMono24Sample &sample);
  static inline TStereo8UnsignedSample from(const TStereo24Sample &sample);
  static inline TStereo8UnsignedSample from(const TMono32Sample &sample);
  static inline TStereo8UnsignedSample from(const TStereo32Sample &sample);
  static inline TStereo8UnsignedSample from(const TMono32floatSample &sample);
  static inline TStereo8UnsignedSample from(const TStereo32floatSample &sample);
};

inline TStereo8UnsignedSample operator+(const TStereo8UnsignedSample &s1,
                                        const TStereo8UnsignedSample &s2) {
  TStereo8UnsignedSample s = s1;
  return s += s2;
}

//=========================================================

class DVAPI TMono16Sample {
  short value;

public:
  typedef short ChannelValueType;
  typedef TMono16Sample ChannelSampleType;

  TMono16Sample(short v = 0) : value(v) {}
  ~TMono16Sample(){};

  static bool isSampleSigned() { return true; }
  static int getBitPerSample() { return 16; }

  inline short getValue(TSound::Channel) const { return value; }

  inline void setValue(TSound::Channel /*chan*/, short v) { value = v; }

  inline double getPressure(TSound::Channel chan) const {
    return (getValue(chan));
  }

  inline TMono16Sample &operator+=(const TMono16Sample &s) {
    int iVal = value + s.value;
    value    = tcrop(iVal, -32768, 32767);
    return *this;
  }

  inline TMono16Sample &operator*=(double a) {
    int iVal = (int)(value * a);
    value    = tcrop(iVal, -32768, 32767);
    return *this;
  }

  inline TMono16Sample operator*(double a) { return TMono16Sample(*this) *= a; }

  static TMono16Sample mix(const TMono16Sample &s1, double a1,
                           const TMono16Sample &s2, double a2) {
    return TMono16Sample(
        tcrop((int)(s1.value * a1 + s2.value * a2), -32768, 32767));
  }

  static inline TMono16Sample from(const TMono8UnsignedSample &sample);
  static inline TMono16Sample from(const TMono8SignedSample &sample);
  static inline TMono16Sample from(const TStereo8SignedSample &sample);
  static inline TMono16Sample from(const TStereo8UnsignedSample &sample);
  static inline TMono16Sample from(const TMono16Sample &sample);
  static inline TMono16Sample from(const TStereo16Sample &sample);
  static inline TMono16Sample from(const TMono24Sample &sample);
  static inline TMono16Sample from(const TStereo24Sample &sample);
  static inline TMono16Sample from(const TMono32Sample &sample);
  static inline TMono16Sample from(const TStereo32Sample &sample);
  static inline TMono16Sample from(const TMono32floatSample &sample);
  static inline TMono16Sample from(const TStereo32floatSample &sample);
};

inline TMono16Sample operator+(const TMono16Sample &s1,
                               const TMono16Sample &s2) {
  TMono16Sample s = s1;
  return s += s2;
}

//=========================================================

class DVAPI TStereo16Sample {
  short channel[2];  // l'ordine dei canali e' left,right

public:
  typedef short ChannelValueType;
  typedef TMono16Sample ChannelSampleType;

  TStereo16Sample(TINT32 v) {
    channel[0] = (short)(v << 16);
    channel[1] = (short)(v & 0xFFFF);
  }

  TStereo16Sample(short lchan = 0, short rchan = 0) {
    channel[0] = lchan;
    channel[1] = rchan;
  }

  ~TStereo16Sample(){};

  static bool isSampleSigned() { return true; }
  static int getBitPerSample() { return 16; }

  inline short getValue(TSound::Channel chan) const {
    assert(chan <= 1);
    return channel[chan];
  }

  inline void setValue(TSound::Channel chan, short v) {
    assert(chan <= 1);
    channel[chan] = v;
  }

  inline double getPressure(TSound::Channel chan) const {
    return (getValue(chan));
  }

  inline TStereo16Sample &operator+=(const TStereo16Sample &s) {
    int iLeftVal  = channel[0] + s.channel[0];
    int iRightVal = channel[1] + s.channel[1];
    channel[0]    = tcrop(iLeftVal, -32768, 32767);
    channel[1]    = tcrop(iRightVal, -32768, 32767);
    return *this;
  }

  inline TStereo16Sample &operator*=(double a) {
    int iLeftVal  = (int)(a * channel[0]);
    int iRightVal = (int)(a * channel[1]);
    channel[0]    = tcrop(iLeftVal, -32768, 32767);
    channel[1]    = tcrop(iRightVal, -32768, 32767);
    return *this;
  }

  inline TStereo16Sample operator*(double a) {
    return TStereo16Sample(*this) *= a;
  }

  static TStereo16Sample mix(const TStereo16Sample &s1, double a1,
                             const TStereo16Sample &s2, double a2) {
    return TStereo16Sample(
        tcrop((int)(s1.channel[0] * a1 + s2.channel[0] * a2), -32768, 32767),
        tcrop((int)(s1.channel[1] * a1 + s2.channel[1] * a2), -32768, 32767));
  }

  static inline TStereo16Sample from(const TMono8UnsignedSample &sample);
  static inline TStereo16Sample from(const TMono8SignedSample &sample);
  static inline TStereo16Sample from(const TStereo8SignedSample &sample);
  static inline TStereo16Sample from(const TStereo8UnsignedSample &sample);
  static inline TStereo16Sample from(const TMono16Sample &sample);
  static inline TStereo16Sample from(const TStereo16Sample &sample);
  static inline TStereo16Sample from(const TMono24Sample &sample);
  static inline TStereo16Sample from(const TStereo24Sample &sample);
  static inline TStereo16Sample from(const TMono32Sample &sample);
  static inline TStereo16Sample from(const TStereo32Sample &sample);
  static inline TStereo16Sample from(const TMono32floatSample &sample);
  static inline TStereo16Sample from(const TStereo32floatSample &sample);
};

inline TStereo16Sample operator+(const TStereo16Sample &s1,
                                 const TStereo16Sample &s2) {
  TStereo16Sample s = s1;
  return s += s2;
}

//=========================================================

class DVAPI TMono24Sample {
  TINT32 value;

public:
  typedef TINT32 ChannelValueType;
  typedef TMono24Sample ChannelSampleType;

  TMono24Sample(TINT32 v = 0)
      : value(tcrop<TINT32>(v, -2147483648, 2147483647)) {}
  ~TMono24Sample(){};

  static bool isSampleSigned() { return true; }
  static int getBitPerSample() { return 32; }  // We converted on import

  inline TINT32 getValue(TSound::Channel) const { return value; }

  inline void setValue(TSound::Channel /*chan*/, TINT32 v) {
    value = tcrop<TINT32>(v, -2147483648, 2147483647);
  }

  inline double getPressure(TSound::Channel chan) const {
    return (getValue(chan) << 8);  // Drop back to 24bit
  }

  inline TMono24Sample &operator+=(const TMono24Sample &s) {
    int iVal = value + s.value;
    value    = tcrop<TINT32>(iVal, -2147483648, 2147483647);
    return *this;
  }

  inline TMono24Sample &operator*=(double a) {
    int iVal = (int)(value * a);
    value    = tcrop<TINT32>(iVal, -2147483648, 2147483647);
    return *this;
  }

  inline TMono24Sample operator*(double a) { return TMono24Sample(*this) *= a; }

  static TMono24Sample mix(const TMono24Sample &s1, double a1,
                           const TMono24Sample &s2, double a2) {
    return TMono24Sample(tcrop<TINT32>((int)(s1.value * a1 + s2.value * a2),
                                       -2147483648, 2147483647));
  }

  static inline TMono24Sample from(const TMono8UnsignedSample &sample);
  static inline TMono24Sample from(const TMono8SignedSample &sample);
  static inline TMono24Sample from(const TStereo8SignedSample &sample);
  static inline TMono24Sample from(const TStereo8UnsignedSample &sample);
  static inline TMono24Sample from(const TMono16Sample &sample);
  static inline TMono24Sample from(const TStereo16Sample &sample);
  static inline TMono24Sample from(const TMono24Sample &sample);
  static inline TMono24Sample from(const TStereo24Sample &sample);
  static inline TMono24Sample from(const TMono32Sample &sample);
  static inline TMono24Sample from(const TStereo32Sample &sample);
  static inline TMono24Sample from(const TMono32floatSample &sample);
  static inline TMono24Sample from(const TStereo32floatSample &sample);
};

inline TMono24Sample operator+(const TMono24Sample &s1,
                               const TMono24Sample &s2) {
  TMono24Sample s = s1;
  return s += s2;
}

//=========================================================

class DVAPI TStereo24Sample {
  TINT32 channel[2];  // l'ordine dei canali e' left,right

public:
  typedef TINT32 ChannelValueType;
  typedef TMono24Sample ChannelSampleType;

  TStereo24Sample(TINT32 lchan = 0, TINT32 rchan = 0) {
    channel[0] = tcrop<TINT32>(lchan, -2147483648, 2147483647);
    channel[1] = tcrop<TINT32>(rchan, -2147483648, 2147483647);
  }

  ~TStereo24Sample(){};

  static bool isSampleSigned() { return true; }
  static int getBitPerSample() { return 32; }  // We converted on import

  inline TINT32 getValue(TSound::Channel chan) const {
    assert(chan <= 1);
    return channel[chan];
  }

  inline void setValue(TSound::Channel chan, TINT32 v) {
    assert(chan <= 1);
    channel[chan] = tcrop<TINT32>(v, -2147483648, 2147483647);
  }

  inline double getPressure(TSound::Channel chan) const {
    return (getValue(chan) << 8);  // Drop back to 24bit
  }

  inline TStereo24Sample &operator+=(const TStereo24Sample &s) {
    int iLeftVal  = channel[0] + s.channel[0];
    int iRightVal = channel[1] + s.channel[1];
    channel[0]    = tcrop<TINT32>(iLeftVal, -2147483648, 2147483647);
    channel[1]    = tcrop<TINT32>(iRightVal, -2147483648, 2147483647);
    return *this;
  }

  inline TStereo24Sample &operator*=(double a) {
    int iLeftVal  = (int)(a * channel[0]);
    int iRightVal = (int)(a * channel[1]);
    channel[0]    = tcrop<TINT32>(iLeftVal, -2147483648, 2147483647);
    channel[1]    = tcrop<TINT32>(iRightVal, -2147483648, 2147483647);
    return *this;
  }

  inline TStereo24Sample operator*(double a) {
    return TStereo24Sample(*this) *= a;
  }

  static TStereo24Sample mix(const TStereo24Sample &s1, double a1,
                             const TStereo24Sample &s2, double a2) {
    return TStereo24Sample(
        tcrop<TINT32>((int)(s1.channel[0] * a1 + s2.channel[0] * a2),
                      -2147483648, 2147483647),
        tcrop<TINT32>((int)(s1.channel[1] * a1 + s2.channel[1] * a2),
                      -2147483648, 2147483647));
  }

  static inline TStereo24Sample from(const TMono8UnsignedSample &sample);
  static inline TStereo24Sample from(const TMono8SignedSample &sample);
  static inline TStereo24Sample from(const TStereo8SignedSample &sample);
  static inline TStereo24Sample from(const TStereo8UnsignedSample &sample);
  static inline TStereo24Sample from(const TMono16Sample &sample);
  static inline TStereo24Sample from(const TStereo16Sample &sample);
  static inline TStereo24Sample from(const TMono24Sample &sample);
  static inline TStereo24Sample from(const TStereo24Sample &sample);
  static inline TStereo24Sample from(const TMono32Sample &sample);
  static inline TStereo24Sample from(const TStereo32Sample &sample);
  static inline TStereo24Sample from(const TMono32floatSample &sample);
  static inline TStereo24Sample from(const TStereo32floatSample &sample);
};

inline TStereo24Sample operator+(const TStereo24Sample &s1,
                                 const TStereo24Sample &s2) {
  TStereo24Sample s = s1;
  return s += s2;
}

//=========================================================

class DVAPI TMono32Sample {
  TINT32 value;

public:
  typedef TINT32 ChannelValueType;
  typedef TMono32Sample ChannelSampleType;

  TMono32Sample(TINT32 v = 0)
      : value(tcrop<TINT32>(v, -2147483648, 2147483647)) {}
  ~TMono32Sample(){};

  static bool isSampleSigned() { return true; }
  static int getBitPerSample() { return 32; }

  inline TINT32 getValue(TSound::Channel) const { return value; }

  inline void setValue(TSound::Channel /*chan*/, TINT32 v) {
    value = tcrop<TINT32>(v, -2147483648, 2147483647);
  }

  inline double getPressure(TSound::Channel chan) const {
    return (getValue(chan));
  }

  inline TMono32Sample &operator+=(const TMono32Sample &s) {
    int iVal = value + s.value;
    value    = tcrop<TINT32>(iVal, -2147483648, 2147483647);
    return *this;
  }

  inline TMono32Sample &operator*=(double a) {
    int iVal = (int)(value * a);
    value    = tcrop<TINT32>(iVal, -2147483648, 2147483647);
    return *this;
  }

  inline TMono32Sample operator*(double a) { return TMono32Sample(*this) *= a; }

  static TMono32Sample mix(const TMono32Sample &s1, double a1,
                           const TMono32Sample &s2, double a2) {
    return TMono32Sample(tcrop<TINT32>((int)(s1.value * a1 + s2.value * a2),
                                       -2147483648, 2147483647));
  }

  static inline TMono32Sample from(const TMono8UnsignedSample &sample);
  static inline TMono32Sample from(const TMono8SignedSample &sample);
  static inline TMono32Sample from(const TStereo8SignedSample &sample);
  static inline TMono32Sample from(const TStereo8UnsignedSample &sample);
  static inline TMono32Sample from(const TMono16Sample &sample);
  static inline TMono32Sample from(const TStereo16Sample &sample);
  static inline TMono32Sample from(const TMono24Sample &sample);
  static inline TMono32Sample from(const TStereo24Sample &sample);
  static inline TMono32Sample from(const TMono32Sample &sample);
  static inline TMono32Sample from(const TStereo32Sample &sample);
  static inline TMono32Sample from(const TMono32floatSample &sample);
  static inline TMono32Sample from(const TStereo32floatSample &sample);
};

inline TMono32Sample operator+(const TMono32Sample &s1,
                               const TMono32Sample &s2) {
  TMono32Sample s = s1;
  return s += s2;
}

//=========================================================

class DVAPI TStereo32Sample {
  TINT32 channel[2];  // l'ordine dei canali e' left,right

public:
  typedef TINT32 ChannelValueType;
  typedef TMono32Sample ChannelSampleType;

  TStereo32Sample(TINT32 lchan = 0, TINT32 rchan = 0) {
    channel[0] = tcrop<TINT32>(lchan, -2147483648, 2147483647);
    channel[1] = tcrop<TINT32>(rchan, -2147483648, 2147483647);
  }

  ~TStereo32Sample(){};

  static bool isSampleSigned() { return true; }
  static int getBitPerSample() { return 32; }

  inline TINT32 getValue(TSound::Channel chan) const {
    assert(chan <= 1);
    return channel[chan];
  }

  inline void setValue(TSound::Channel chan, TINT32 v) {
    assert(chan <= 1);
    channel[chan] = tcrop<TINT32>(v, -2147483648, 2147483647);
  }

  inline double getPressure(TSound::Channel chan) const {
    return (getValue(chan));
  }

  inline TStereo32Sample &operator+=(const TStereo32Sample &s) {
    int iLeftVal  = channel[0] + s.channel[0];
    int iRightVal = channel[1] + s.channel[1];
    channel[0]    = tcrop<TINT32>(iLeftVal, -2147483648, 2147483647);
    channel[1]    = tcrop<TINT32>(iRightVal, -2147483648, 2147483647);
    return *this;
  }

  inline TStereo32Sample &operator*=(double a) {
    int iLeftVal  = (int)(a * channel[0]);
    int iRightVal = (int)(a * channel[1]);
    channel[0]    = tcrop<TINT32>(iLeftVal, -2147483648, 2147483647);
    channel[1]    = tcrop<TINT32>(iRightVal, -2147483648, 2147483647);
    return *this;
  }

  inline TStereo32Sample operator*(double a) {
    return TStereo32Sample(*this) *= a;
  }

  static TStereo32Sample mix(const TStereo32Sample &s1, double a1,
                             const TStereo32Sample &s2, double a2) {
    return TStereo32Sample(
        tcrop<TINT32>((int)(s1.channel[0] * a1 + s2.channel[0] * a2),
                      -2147483648, 2147483647),
        tcrop<TINT32>((int)(s1.channel[1] * a1 + s2.channel[1] * a2),
                      -2147483648, 2147483647));
  }

  static inline TStereo32Sample from(const TMono8UnsignedSample &sample);
  static inline TStereo32Sample from(const TMono8SignedSample &sample);
  static inline TStereo32Sample from(const TStereo8SignedSample &sample);
  static inline TStereo32Sample from(const TStereo8UnsignedSample &sample);
  static inline TStereo32Sample from(const TMono16Sample &sample);
  static inline TStereo32Sample from(const TStereo16Sample &sample);
  static inline TStereo32Sample from(const TMono24Sample &sample);
  static inline TStereo32Sample from(const TStereo24Sample &sample);
  static inline TStereo32Sample from(const TMono32Sample &sample);
  static inline TStereo32Sample from(const TStereo32Sample &sample);
  static inline TStereo32Sample from(const TMono32floatSample &sample);
  static inline TStereo32Sample from(const TStereo32floatSample &sample);
};

inline TStereo32Sample operator+(const TStereo32Sample &s1,
                                 const TStereo32Sample &s2) {
  TStereo32Sample s = s1;
  return s += s2;
}

//=========================================================

class DVAPI TMono32floatSample {
  float value;

public:
  typedef float ChannelValueType;
  typedef TMono32floatSample ChannelSampleType;

  TMono32floatSample(float v = 0.0) : value(tcrop<float>(v, -1.0, 1.0)) {}
  ~TMono32floatSample(){};

  static bool isSampleSigned() { return true; }
  static int getBitPerSample() { return 32; }

  inline float getValue(TSound::Channel) const { return value; }

  inline void setValue(TSound::Channel /*chan*/, float v) {
    value = tcrop<float>(v, -1.0, 1.0);
  }

  inline double getPressure(TSound::Channel chan) const {
    return (getValue(chan));
  }

  inline TMono32floatSample &operator+=(const TMono32floatSample &s) {
    float fVal = value + s.value;
    value      = tcrop<float>(fVal, -1.0, 1.0);
    return *this;
  }

  inline TMono32floatSample &operator*=(double a) {
    float fVal = (value * a);
    value      = tcrop<float>(fVal, -1.0, 1.0);
    return *this;
  }

  inline TMono32floatSample operator*(double a) {
    return TMono32floatSample(*this) *= a;
  }

  static TMono32floatSample mix(const TMono32floatSample &s1, double a1,
                                const TMono32floatSample &s2, double a2) {
    return TMono32floatSample(
        tcrop<float>((s1.value * a1 + s2.value * a2), -1.0, 1.0));
  }

  static inline TMono32floatSample from(const TMono8UnsignedSample &sample);
  static inline TMono32floatSample from(const TMono8SignedSample &sample);
  static inline TMono32floatSample from(const TStereo8SignedSample &sample);
  static inline TMono32floatSample from(const TStereo8UnsignedSample &sample);
  static inline TMono32floatSample from(const TMono16Sample &sample);
  static inline TMono32floatSample from(const TStereo16Sample &sample);
  static inline TMono32floatSample from(const TMono24Sample &sample);
  static inline TMono32floatSample from(const TStereo24Sample &sample);
  static inline TMono32floatSample from(const TMono32Sample &sample);
  static inline TMono32floatSample from(const TStereo32Sample &sample);
  static inline TMono32floatSample from(const TMono32floatSample &sample);
  static inline TMono32floatSample from(const TStereo32floatSample &sample);
};

inline TMono32floatSample operator+(const TMono32floatSample &s1,
                                    const TMono32floatSample &s2) {
  TMono32floatSample s = s1;
  return s += s2;
}

//=========================================================

class DVAPI TStereo32floatSample {
  float channel[2];  // l'ordine dei canali e' left,right

public:
  typedef float ChannelValueType;
  typedef TMono32floatSample ChannelSampleType;

  TStereo32floatSample(float lchan = 0.0, float rchan = 0.0) {
    channel[0] = tcrop<float>(lchan, -1.0, 1.0);
    channel[1] = tcrop<float>(rchan, -1.0, 1.0);
  }

  ~TStereo32floatSample(){};

  static bool isSampleSigned() { return true; }
  static int getBitPerSample() { return 32; }

  inline float getValue(TSound::Channel chan) const {
    assert(chan <= 1);
    return channel[chan];
  }

  inline void setValue(TSound::Channel chan, float v) {
    assert(chan <= 1);
    channel[chan] = tcrop<float>(v, -1.0, 1.0);
  }

  inline double getPressure(TSound::Channel chan) const {
    return (getValue(chan));
  }

  inline TStereo32floatSample &operator+=(const TStereo32floatSample &s) {
    float fLeftVal  = channel[0] + s.channel[0];
    float fRightVal = channel[1] + s.channel[1];
    channel[0]      = tcrop<float>(fLeftVal, -1.0, 1.0);
    channel[1]      = tcrop<float>(fRightVal, -1.0, 1.0);
    return *this;
  }

  inline TStereo32floatSample &operator*=(double a) {
    float fLeftVal  = (a * channel[0]);
    float fRightVal = (a * channel[1]);
    channel[0]      = tcrop<float>(fLeftVal, -1.0, 1.0);
    channel[1]      = tcrop<float>(fRightVal, -1.0, 1.0);
    return *this;
  }

  inline TStereo32floatSample operator*(double a) {
    return TStereo32floatSample(*this) *= a;
  }

  static TStereo32floatSample mix(const TStereo32floatSample &s1, double a1,
                                  const TStereo32floatSample &s2, double a2) {
    return TStereo32floatSample(
        tcrop<float>((s1.channel[0] * a1 + s2.channel[0] * a2), -1.0, 1.0),
        tcrop<float>((s1.channel[1] * a1 + s2.channel[1] * a2), -1.0, 1.0));
  }

  static inline TStereo32floatSample from(const TMono8UnsignedSample &sample);
  static inline TStereo32floatSample from(const TMono8SignedSample &sample);
  static inline TStereo32floatSample from(const TStereo8SignedSample &sample);
  static inline TStereo32floatSample from(const TStereo8UnsignedSample &sample);
  static inline TStereo32floatSample from(const TMono16Sample &sample);
  static inline TStereo32floatSample from(const TStereo16Sample &sample);
  static inline TStereo32floatSample from(const TMono24Sample &sample);
  static inline TStereo32floatSample from(const TStereo24Sample &sample);
  static inline TStereo32floatSample from(const TMono32Sample &sample);
  static inline TStereo32floatSample from(const TStereo32Sample &sample);
  static inline TStereo32floatSample from(const TMono32floatSample &sample);
  static inline TStereo32floatSample from(const TStereo32floatSample &sample);
};

inline TStereo32floatSample operator+(const TStereo32floatSample &s1,
                                      const TStereo32floatSample &s2) {
  TStereo32floatSample s = s1;
  return s += s2;
}

//==============================================================================

inline TMono8SignedSample TMono8SignedSample::from(
    const TMono8SignedSample &sample) {
  return sample;
}

//------------------------------------------------------------------------------

inline TMono8SignedSample TMono8SignedSample::from(
    const TMono8UnsignedSample &sample) {
  return TMono8SignedSample(sample.getValue(TSound::LEFT) - 128);
}

//------------------------------------------------------------------------------

inline TMono8SignedSample TMono8SignedSample::from(
    const TStereo8SignedSample &sample) {
  return TMono8SignedSample(
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 1);
}

//------------------------------------------------------------------------------

inline TMono8SignedSample TMono8SignedSample::from(
    const TStereo8UnsignedSample &sample) {
  return TMono8SignedSample(
      ((sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 1) -
      128);
}

//------------------------------------------------------------------------------

inline TMono8SignedSample TMono8SignedSample::from(
    const TMono16Sample &sample) {
  int val = (sample.getValue(TSound::LEFT) >> 8);
  return TMono8SignedSample(val);
}

//------------------------------------------------------------------------------

inline TMono8SignedSample TMono8SignedSample::from(
    const TStereo16Sample &sample) {
  return TMono8SignedSample(
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 9);
}

//------------------------------------------------------------------------------

inline TMono8SignedSample TMono8SignedSample::from(
    const TMono24Sample &sample) {
  int val = (sample.getValue(TSound::LEFT) >> 24);
  return TMono8SignedSample(val);
}

//------------------------------------------------------------------------------

inline TMono8SignedSample TMono8SignedSample::from(
    const TStereo24Sample &sample) {
  int val =
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 25;
  return TMono8SignedSample(val);
}

//------------------------------------------------------------------------------

inline TMono8SignedSample TMono8SignedSample::from(
    const TMono32Sample &sample) {
  int val = (sample.getValue(TSound::LEFT) >> 24);
  return TMono8SignedSample(val);
}

//------------------------------------------------------------------------------

inline TMono8SignedSample TMono8SignedSample::from(
    const TStereo32Sample &sample) {
  int val =
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 25;
  return TMono8SignedSample(val);
}

//------------------------------------------------------------------------------

inline TMono8SignedSample TMono8SignedSample::from(
    const TMono32floatSample &sample) {
  float val = (TINT32)(sample.getValue(TSound::LEFT) * 2147483648) >> 24;
  return TMono8SignedSample(val);
}

//------------------------------------------------------------------------------

inline TMono8SignedSample TMono8SignedSample::from(
    const TStereo32floatSample &sample) {
  float val = (TINT32)((sample.getValue(TSound::LEFT) +
                        sample.getValue(TSound::RIGHT)) *
                       2147483648) >>
              25;
  return TMono8SignedSample(val);
}

//==============================================================================

inline TMono8UnsignedSample TMono8UnsignedSample::from(
    const TMono8SignedSample &sample) {
  return TMono8UnsignedSample(sample.getValue(TSound::LEFT) + 128);
}

//------------------------------------------------------------------------------

inline TMono8UnsignedSample TMono8UnsignedSample::from(
    const TMono8UnsignedSample &sample) {
  return sample;
}

//------------------------------------------------------------------------------

inline TMono8UnsignedSample TMono8UnsignedSample::from(
    const TStereo8SignedSample &sample) {
  int iVal = sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT);
  return TMono8UnsignedSample((iVal >> 1) + 128);
}

//------------------------------------------------------------------------------

inline TMono8UnsignedSample TMono8UnsignedSample::from(
    const TStereo8UnsignedSample &sample) {
  return TMono8UnsignedSample(
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 1);
}

//------------------------------------------------------------------------------

inline TMono8UnsignedSample TMono8UnsignedSample::from(
    const TMono16Sample &sample) {
  return TMono8UnsignedSample((sample.getValue(TSound::MONO) >> 8) + 128);
}

//------------------------------------------------------------------------------

inline TMono8UnsignedSample TMono8UnsignedSample::from(
    const TStereo16Sample &sample) {
  return TMono8UnsignedSample(
      ((sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 9) +
      128);
}

//------------------------------------------------------------------------------

inline TMono8UnsignedSample TMono8UnsignedSample::from(
    const TMono24Sample &sample) {
  return TMono8UnsignedSample(
      (unsigned char)(sample.getValue(TSound::MONO) >> 24) + 128);
}

//------------------------------------------------------------------------------

inline TMono8UnsignedSample TMono8UnsignedSample::from(
    const TStereo24Sample &sample) {
  return TMono8UnsignedSample(
      (unsigned char)((sample.getValue(TSound::LEFT) +
                       sample.getValue(TSound::RIGHT)) >>
                      25) +
      128);
}

//------------------------------------------------------------------------------

inline TMono8UnsignedSample TMono8UnsignedSample::from(
    const TMono32Sample &sample) {
  return TMono8UnsignedSample(
      (unsigned char)(sample.getValue(TSound::MONO) >> 24) + 128);
}

//------------------------------------------------------------------------------

inline TMono8UnsignedSample TMono8UnsignedSample::from(
    const TStereo32Sample &sample) {
  return TMono8UnsignedSample(
      (unsigned char)((sample.getValue(TSound::LEFT) +
                       sample.getValue(TSound::RIGHT)) >>
                      25) +
      128);
}

//------------------------------------------------------------------------------

inline TMono8UnsignedSample TMono8UnsignedSample::from(
    const TMono32floatSample &sample) {
  return TMono8UnsignedSample(
      (unsigned char)((TINT32)(sample.getValue(TSound::MONO) * 2147483648) >>
                      24) +
      128);
}

//------------------------------------------------------------------------------

inline TMono8UnsignedSample TMono8UnsignedSample::from(
    const TStereo32floatSample &sample) {
  return TMono8UnsignedSample(
      (unsigned char)((TINT32)((sample.getValue(TSound::LEFT) +
                                sample.getValue(TSound::RIGHT)) *
                               2147483648) >>
                      25) +
      128);
}

//==============================================================================

inline TStereo8SignedSample TStereo8SignedSample::from(
    const TMono8UnsignedSample &sample) {
  int srcval = sample.getValue(TSound::LEFT) - 128;
  return TStereo8SignedSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo8SignedSample TStereo8SignedSample::from(
    const TMono8SignedSample &sample) {
  int srcval = sample.getValue(TSound::LEFT);
  return TStereo8SignedSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo8SignedSample TStereo8SignedSample::from(
    const TStereo8SignedSample &sample) {
  return sample;
}

//------------------------------------------------------------------------------

inline TStereo8SignedSample TStereo8SignedSample::from(
    const TStereo8UnsignedSample &sample) {
  int srcval =
      ((sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 1) -
      128;
  return TStereo8SignedSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo8SignedSample TStereo8SignedSample::from(
    const TMono16Sample &sample) {
  int srcval = sample.getValue(TSound::LEFT) >> 8;
  return TStereo8SignedSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo8SignedSample TStereo8SignedSample::from(
    const TStereo16Sample &sample) {
  int srcval =
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 9;
  return TStereo8SignedSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo8SignedSample TStereo8SignedSample::from(
    const TMono24Sample &sample) {
  int srcval = sample.getValue(TSound::LEFT) >> 24;
  return TStereo8SignedSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo8SignedSample TStereo8SignedSample::from(
    const TStereo24Sample &sample) {
  int srcval =
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 25;
  return TStereo8SignedSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo8SignedSample TStereo8SignedSample::from(
    const TMono32Sample &sample) {
  int srcval = sample.getValue(TSound::LEFT) >> 24;
  return TStereo8SignedSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo8SignedSample TStereo8SignedSample::from(
    const TStereo32Sample &sample) {
  int srcval =
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 25;
  return TStereo8SignedSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo8SignedSample TStereo8SignedSample::from(
    const TMono32floatSample &sample) {
  int srcval = (TINT32)(sample.getValue(TSound::LEFT) * 2147483648) >> 24;
  return TStereo8SignedSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo8SignedSample TStereo8SignedSample::from(
    const TStereo32floatSample &sample) {
  int srcval = (TINT32)((sample.getValue(TSound::LEFT) +
                         sample.getValue(TSound::RIGHT)) *
                        2147483648) >>
               25;
  return TStereo8SignedSample(srcval, srcval);
}

//==============================================================================

inline TStereo8UnsignedSample TStereo8UnsignedSample::from(
    const TMono8UnsignedSample &sample) {
  int srcval = sample.getValue(TSound::LEFT);

  return TStereo8UnsignedSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo8UnsignedSample TStereo8UnsignedSample::from(
    const TMono8SignedSample &sample) {
  int srcval = sample.getValue(TSound::LEFT) + 128;

  return TStereo8UnsignedSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo8UnsignedSample TStereo8UnsignedSample::from(
    const TStereo8UnsignedSample &sample) {
  return sample;
}

//------------------------------------------------------------------------------

inline TStereo8UnsignedSample TStereo8UnsignedSample::from(
    const TStereo8SignedSample &sample) {
  int srcval =
      ((sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 1) +
      128;
  return TStereo8UnsignedSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo8UnsignedSample TStereo8UnsignedSample::from(
    const TMono16Sample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) >> 8) + 128;
  return TStereo8UnsignedSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo8UnsignedSample TStereo8UnsignedSample::from(
    const TStereo16Sample &sample) {
  int srcval =
      ((sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 9) +
      128;
  return TStereo8UnsignedSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo8UnsignedSample TStereo8UnsignedSample::from(
    const TMono24Sample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) >> 24) + 128;
  return TStereo8UnsignedSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo8UnsignedSample TStereo8UnsignedSample::from(
    const TStereo24Sample &sample) {
  int srcval =
      ((sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 25) +
      128;
  return TStereo8UnsignedSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo8UnsignedSample TStereo8UnsignedSample::from(
    const TMono32Sample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) >> 24) + 128;
  return TStereo8UnsignedSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo8UnsignedSample TStereo8UnsignedSample::from(
    const TStereo32Sample &sample) {
  int srcval =
      ((sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 25) +
      128;
  return TStereo8UnsignedSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo8UnsignedSample TStereo8UnsignedSample::from(
    const TMono32floatSample &sample) {
  int srcval =
      ((TINT32)(sample.getValue(TSound::LEFT) * 2147483648) >> 24) + 128;
  return TStereo8UnsignedSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo8UnsignedSample TStereo8UnsignedSample::from(
    const TStereo32floatSample &sample) {
  int srcval = ((TINT32)((sample.getValue(TSound::LEFT) +
                          sample.getValue(TSound::RIGHT)) *
                         2147483648) >>
                25) +
               128;
  return TStereo8UnsignedSample(srcval, srcval);
}

//==============================================================================

inline TMono16Sample TMono16Sample::from(const TMono8UnsignedSample &sample) {
  return TMono16Sample((sample.getValue(TSound::LEFT) - 128) << 8);
}

//------------------------------------------------------------------------------

inline TMono16Sample TMono16Sample::from(const TMono8SignedSample &sample) {
  return TMono16Sample(sample.getValue(TSound::LEFT) << 8);
}

//------------------------------------------------------------------------------

inline TMono16Sample TMono16Sample::from(const TStereo8SignedSample &sample) {
  return TMono16Sample(
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) << 7);
}

//------------------------------------------------------------------------------

inline TMono16Sample TMono16Sample::from(const TStereo8UnsignedSample &sample) {
  return TMono16Sample(
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT) - 256)
      << 7);
}

//------------------------------------------------------------------------------

inline TMono16Sample TMono16Sample::from(const TMono16Sample &sample) {
  return sample;
}

//------------------------------------------------------------------------------

inline TMono16Sample TMono16Sample::from(const TStereo16Sample &sample) {
  return TMono16Sample(
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 1);
}

//------------------------------------------------------------------------------

inline TMono16Sample TMono16Sample::from(const TMono24Sample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) >> 16);
  return TMono16Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono16Sample TMono16Sample::from(const TStereo24Sample &sample) {
  int srcval =
      ((sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 17);
  return TMono16Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono16Sample TMono16Sample::from(const TMono32Sample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) >> 16);
  return TMono16Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono16Sample TMono16Sample::from(const TStereo32Sample &sample) {
  int srcval =
      ((sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 17);
  return TMono16Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono16Sample TMono16Sample::from(const TMono32floatSample &sample) {
  int srcval = (TINT32)(sample.getValue(TSound::LEFT) * 2147483648) >> 16;
  return TMono16Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono16Sample TMono16Sample::from(const TStereo32floatSample &sample) {
  int srcval = (TINT32)((sample.getValue(TSound::LEFT) +
                         sample.getValue(TSound::RIGHT)) *
                        2147483648) >>
               17;
  return TMono16Sample(srcval);
}

//==============================================================================

inline TStereo16Sample TStereo16Sample::from(
    const TMono8UnsignedSample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) - 128) << 8;
  return TStereo16Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo16Sample TStereo16Sample::from(const TMono8SignedSample &sample) {
  int srcval = sample.getValue(TSound::LEFT) << 8;
  return TStereo16Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo16Sample TStereo16Sample::from(
    const TStereo8SignedSample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT))
               << 7;
  return TStereo16Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo16Sample TStereo16Sample::from(
    const TStereo8UnsignedSample &sample) {
  int srcval =
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT) - 256)
      << 7;
  return TStereo16Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo16Sample TStereo16Sample::from(const TMono16Sample &sample) {
  int srcval = sample.getValue(TSound::LEFT);
  return TStereo16Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo16Sample TStereo16Sample::from(const TStereo16Sample &sample) {
  return sample;
}

//------------------------------------------------------------------------------

inline TStereo16Sample TStereo16Sample::from(const TMono24Sample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) >> 16);
  return TStereo16Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo16Sample TStereo16Sample::from(const TStereo24Sample &sample) {
  int srcval =
      ((sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 17);
  return TStereo16Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo16Sample TStereo16Sample::from(const TMono32Sample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) >> 16);
  return TStereo16Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo16Sample TStereo16Sample::from(const TStereo32Sample &sample) {
  int srcval =
      ((sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 17);
  return TStereo16Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo16Sample TStereo16Sample::from(const TMono32floatSample &sample) {
  int srcval = (TINT32)(sample.getValue(TSound::LEFT) * 2147483648) >> 16;
  return TStereo16Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo16Sample TStereo16Sample::from(
    const TStereo32floatSample &sample) {
  int srcval = (TINT32)((sample.getValue(TSound::LEFT) +
                         sample.getValue(TSound::RIGHT)) *
                        2147483648) >>
               17;
  return TStereo16Sample(srcval, srcval);
}

//==============================================================================

inline TMono24Sample TMono24Sample::from(const TMono8UnsignedSample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) - 128) << 16;
  return TMono24Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono24Sample TMono24Sample::from(const TMono8SignedSample &sample) {
  int srcval = sample.getValue(TSound::LEFT) << 16;
  return TMono24Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono24Sample TMono24Sample::from(const TStereo8SignedSample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT))
               << 15;
  return TMono24Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono24Sample TMono24Sample::from(const TStereo8UnsignedSample &sample) {
  int srcval =
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT) - 256)
      << 15;
  return TMono24Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono24Sample TMono24Sample::from(const TMono16Sample &sample) {
  int srcval = sample.getValue(TSound::LEFT) << 8;
  return TMono24Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono24Sample TMono24Sample::from(const TStereo16Sample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT))
               << 7;
  return TMono24Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono24Sample TMono24Sample::from(const TMono24Sample &sample) {
  return sample;
}

//------------------------------------------------------------------------------

inline TMono24Sample TMono24Sample::from(const TStereo24Sample &sample) {
  int srcval =
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 1;
  return TMono24Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono24Sample TMono24Sample::from(const TMono32Sample &sample) {
  int srcval = sample.getValue(TSound::LEFT) >> 8;
  return TMono24Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono24Sample TMono24Sample::from(const TStereo32Sample &sample) {
  int srcval =
      ((sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 9);
  return TMono24Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono24Sample TMono24Sample::from(const TMono32floatSample &sample) {
  int srcval = (TINT32)(sample.getValue(TSound::LEFT) * 2147483648) >> 8;
  return TMono24Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono24Sample TMono24Sample::from(const TStereo32floatSample &sample) {
  int srcval = (TINT32)((sample.getValue(TSound::LEFT) +
                         sample.getValue(TSound::RIGHT)) *
                        2147483648) >>
               9;
  return TMono24Sample(srcval);
}

//==============================================================================

inline TStereo24Sample TStereo24Sample::from(
    const TMono8UnsignedSample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) - 128) << 16;
  return TStereo24Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo24Sample TStereo24Sample::from(const TMono8SignedSample &sample) {
  int srcval = sample.getValue(TSound::LEFT) << 16;
  return TStereo24Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo24Sample TStereo24Sample::from(
    const TStereo8SignedSample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT))
               << 15;
  return TStereo24Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo24Sample TStereo24Sample::from(
    const TStereo8UnsignedSample &sample) {
  int srcval =
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT) - 256)
      << 15;
  return TStereo24Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo24Sample TStereo24Sample::from(const TMono16Sample &sample) {
  int srcval = sample.getValue(TSound::LEFT) << 8;
  return TStereo24Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo24Sample TStereo24Sample::from(const TStereo16Sample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT))
               << 7;
  return TStereo24Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo24Sample TStereo24Sample::from(const TMono24Sample &sample) {
  int srcval = sample.getValue(TSound::LEFT);
  return TStereo24Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo24Sample TStereo24Sample::from(const TStereo24Sample &sample) {
  return sample;
}

//------------------------------------------------------------------------------

inline TStereo24Sample TStereo24Sample::from(const TMono32Sample &sample) {
  int srcval = sample.getValue(TSound::LEFT) >> 8;
  return TStereo24Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo24Sample TStereo24Sample::from(const TStereo32Sample &sample) {
  int srcval =
      ((sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 9);
  return TStereo24Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo24Sample TStereo24Sample::from(const TMono32floatSample &sample) {
  int srcval = (TINT32)(sample.getValue(TSound::LEFT) * 2147483648) >> 8;
  return TStereo24Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo24Sample TStereo24Sample::from(
    const TStereo32floatSample &sample) {
  int srcval = (TINT32)((sample.getValue(TSound::LEFT) +
                         sample.getValue(TSound::RIGHT)) *
                        2147483648) >>
               9;
  return TStereo24Sample(srcval, srcval);
}

//==============================================================================

inline TMono32Sample TMono32Sample::from(const TMono8UnsignedSample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) - 128) << 24;
  return TMono32Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono32Sample TMono32Sample::from(const TMono8SignedSample &sample) {
  int srcval = sample.getValue(TSound::LEFT) << 24;
  return TMono32Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono32Sample TMono32Sample::from(const TStereo8SignedSample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT))
               << 23;
  return TMono32Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono32Sample TMono32Sample::from(const TStereo8UnsignedSample &sample) {
  int srcval =
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT) - 256)
      << 23;
  return TMono32Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono32Sample TMono32Sample::from(const TMono16Sample &sample) {
  int srcval = sample.getValue(TSound::LEFT) << 16;
  return TMono32Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono32Sample TMono32Sample::from(const TStereo16Sample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT))
               << 15;
  return TMono32Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono32Sample TMono32Sample::from(const TMono24Sample &sample) {
  int srcval = sample.getValue(TSound::LEFT) << 8;
  return TMono32Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono32Sample TMono32Sample::from(const TStereo24Sample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT))
               << 7;
  return TMono32Sample(srcval);
}

//------------------------------------------------------------------------------

inline TMono32Sample TMono32Sample::from(const TMono32Sample &sample) {
  return sample;
}

//------------------------------------------------------------------------------

inline TMono32Sample TMono32Sample::from(const TStereo32Sample &sample) {
  return TMono32Sample(
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) >> 1);
}

//------------------------------------------------------------------------------

inline TMono32Sample TMono32Sample::from(const TMono32floatSample &sample) {
  return TMono32Sample(sample.getValue(TSound::LEFT) * 2147483648);
}

//------------------------------------------------------------------------------

inline TMono32Sample TMono32Sample::from(const TStereo32floatSample &sample) {
  return TMono32Sample(
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) *
      2147483648);
}

//==============================================================================

inline TStereo32Sample TStereo32Sample::from(
    const TMono8UnsignedSample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) - 128) << 24;
  return TStereo32Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo32Sample TStereo32Sample::from(const TMono8SignedSample &sample) {
  int srcval = sample.getValue(TSound::LEFT) << 24;
  return TStereo32Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo32Sample TStereo32Sample::from(
    const TStereo8SignedSample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT))
               << 23;
  return TStereo32Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo32Sample TStereo32Sample::from(
    const TStereo8UnsignedSample &sample) {
  int srcval =
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT) - 256)
      << 23;
  return TStereo32Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo32Sample TStereo32Sample::from(const TMono16Sample &sample) {
  int srcval = sample.getValue(TSound::LEFT) << 16;
  return TStereo32Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo32Sample TStereo32Sample::from(const TStereo16Sample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT))
               << 15;
  return TStereo32Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo32Sample TStereo32Sample::from(const TMono24Sample &sample) {
  int srcval = sample.getValue(TSound::LEFT) << 8;
  return TStereo32Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo32Sample TStereo32Sample::from(const TStereo24Sample &sample) {
  int srcval = (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT))
               << 7;
  return TStereo32Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo32Sample TStereo32Sample::from(const TMono32Sample &sample) {
  int srcval = sample.getValue(TSound::LEFT);
  return TStereo32Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo32Sample TStereo32Sample::from(const TStereo32Sample &sample) {
  return sample;
}

//------------------------------------------------------------------------------

inline TStereo32Sample TStereo32Sample::from(const TMono32floatSample &sample) {
  int srcval = sample.getValue(TSound::LEFT) * 2147483648;
  return TStereo32Sample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo32Sample TStereo32Sample::from(
    const TStereo32floatSample &sample) {
  int srcval =
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) *
      2147483648;
  return TStereo32Sample(srcval, srcval);
}

//==============================================================================

inline TMono32floatSample TMono32floatSample::from(
    const TMono8UnsignedSample &sample) {
  float srcval = ((sample.getValue(TSound::LEFT) - 128) << 24) / 2147483648;
  return TMono32floatSample(srcval);
}

//------------------------------------------------------------------------------

inline TMono32floatSample TMono32floatSample::from(
    const TMono8SignedSample &sample) {
  float srcval = (sample.getValue(TSound::LEFT) << 24) / 2147483648;
  return TMono32floatSample(srcval);
}

//------------------------------------------------------------------------------

inline TMono32floatSample TMono32floatSample::from(
    const TStereo8SignedSample &sample) {
  float srcval =
      ((sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) << 23) /
      2147483648;
  return TMono32floatSample(srcval);
}

//------------------------------------------------------------------------------

inline TMono32floatSample TMono32floatSample::from(
    const TStereo8UnsignedSample &sample) {
  float srcval =
      ((sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT) - 256)
       << 23) /
      2147483648;
  return TMono32floatSample(srcval);
}

//------------------------------------------------------------------------------

inline TMono32floatSample TMono32floatSample::from(
    const TMono16Sample &sample) {
  float srcval = (sample.getValue(TSound::LEFT) << 16) / 2147483648;
  return TMono32floatSample(srcval);
}

//------------------------------------------------------------------------------

inline TMono32floatSample TMono32floatSample::from(
    const TStereo16Sample &sample) {
  float srcval =
      ((sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) << 15) /
      2147483648;
  return TMono32floatSample(srcval);
}

//------------------------------------------------------------------------------

inline TMono32floatSample TMono32floatSample::from(
    const TMono24Sample &sample) {
  float srcval = (sample.getValue(TSound::LEFT) << 8) / 2147483648;
  return TMono32floatSample(srcval);
}

//------------------------------------------------------------------------------

inline TMono32floatSample TMono32floatSample::from(
    const TStereo24Sample &sample) {
  float srcval =
      ((sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) << 7) /
      2147483648;
  return TMono32floatSample(srcval);
}

//------------------------------------------------------------------------------

inline TMono32floatSample TMono32floatSample::from(
    const TMono32Sample &sample) {
  float srcval = sample.getValue(TSound::LEFT) / 2147483648;
  return TMono32floatSample(srcval);
}

//------------------------------------------------------------------------------

inline TMono32floatSample TMono32floatSample::from(
    const TStereo32Sample &sample) {
  float srcval =
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) /
      2147483648;
  return TMono32floatSample(srcval);
}

//------------------------------------------------------------------------------

inline TMono32floatSample TMono32floatSample::from(
    const TMono32floatSample &sample) {
  return sample;
}

//------------------------------------------------------------------------------

inline TMono32floatSample TMono32floatSample::from(
    const TStereo32floatSample &sample) {
  return TMono32floatSample(
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)));
}

//==============================================================================

inline TStereo32floatSample TStereo32floatSample::from(
    const TMono8UnsignedSample &sample) {
  float srcval = ((sample.getValue(TSound::LEFT) - 128) << 24) / 2147483648;
  return TStereo32floatSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo32floatSample TStereo32floatSample::from(
    const TMono8SignedSample &sample) {
  float srcval = (sample.getValue(TSound::LEFT) << 24) / 2147483648;
  return TStereo32floatSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo32floatSample TStereo32floatSample::from(
    const TStereo8SignedSample &sample) {
  float srcval =
      ((sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) << 23) /
      2147483648;
  return TStereo32floatSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo32floatSample TStereo32floatSample::from(
    const TStereo8UnsignedSample &sample) {
  float srcval =
      ((sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT) - 256)
       << 23) /
      2147483648;
  return TStereo32floatSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo32floatSample TStereo32floatSample::from(
    const TMono16Sample &sample) {
  float srcval = (sample.getValue(TSound::LEFT) << 16) / 2147483648;
  return TStereo32floatSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo32floatSample TStereo32floatSample::from(
    const TStereo16Sample &sample) {
  float srcval =
      ((sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) << 15) /
      2147483648;
  return TStereo32floatSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo32floatSample TStereo32floatSample::from(
    const TMono24Sample &sample) {
  float srcval = (sample.getValue(TSound::LEFT) << 8) / 2147483648;
  return TStereo32floatSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo32floatSample TStereo32floatSample::from(
    const TStereo24Sample &sample) {
  float srcval =
      ((sample.getValue(TSound::LEFT) + sample.getValue(TSound::RIGHT)) << 7) /
      2147483648;
  return TStereo32floatSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo32floatSample TStereo32floatSample::from(
    const TMono32Sample &sample) {
  float srcval = sample.getValue(TSound::LEFT) / 2147483648;
  return TStereo32floatSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo32floatSample TStereo32floatSample::from(
    const TStereo32Sample &sample) {
  float srcval =
      (sample.getValue(TSound::LEFT) + sample.getValue(TSound::LEFT)) /
      2147483648;
  return TStereo32floatSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo32floatSample TStereo32floatSample::from(
    const TMono32floatSample &sample) {
  float srcval = sample.getValue(TSound::LEFT);
  return TStereo32floatSample(srcval, srcval);
}

//------------------------------------------------------------------------------

inline TStereo32floatSample TStereo32floatSample::from(
    const TStereo32floatSample &sample) {
  return sample;
}

#endif
