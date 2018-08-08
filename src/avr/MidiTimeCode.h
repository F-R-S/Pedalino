//                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /*
// Thanks to https://github.com/adanselm/padchokola
//
#ifndef _MIDI_CLOCK_MTC_H_
#define _MIDI_CLOCK_MTC_H_

#include <Arduino.h>

// TAP_NUM_READINGS doesn't mean we have to wait for this many samples
// to change BPM, just that smoothing operates on this value.
#define TAP_NUM_READINGS 5

/////////////////////////////////////
class TapTempo
{
  public:
    TapTempo();
    ~TapTempo();

    float         tap(byte ppqn = 1);
    void          reset();

  private:
    int           mCurrentReadingPos;
    float         mReadings[TAP_NUM_READINGS];
    unsigned long mLastTap;

    bool          timeout(const unsigned long currentTime) const;
    float         calcBpmFromTime(unsigned long currentTime) const;
    float         computeAverage() const;
};

/////////////////////////////////////
class MidiTimeCode
{
  public:
    enum MidiSynchro
    {
      SynchroNone = 0,
      SynchroClockMaster,
      SynchroClockSlave,
      SynchroMTCMaster,
      SynchroMTCSlave
    };

    MidiTimeCode();
    ~MidiTimeCode();

    // To be called on main program setup
    void setup();

    // Only active in Midi Clock mode
    void        setBpm(const float iBpm);
    const float tapTempo();
    byte        getBeat();
    //

    static void setMode(MidiSynchro newMode);
    static MidiSynchro getMode();

    // Only active in clock and MTC :
    void sendPlay();
    void sendStop();
    void sendContinue();
    bool isPlaying() const;
    //

    // Only active in MTC :
    void sendPosition(byte hours, byte minutes, byte seconds, byte frames);
    byte getHours();
    byte getMinutes();
    byte getSeconds();
    byte getFrames();
    //

    static void doSendMidiClock();
    static void doSendMTC();

  private:
    enum MidiType
    {
      InvalidType           = 0x00,    ///< For notifying errors
      NoteOff               = 0x80,    ///< Note Off
      NoteOn                = 0x90,    ///< Note On
      AfterTouchPoly        = 0xA0,    ///< Polyphonic AfterTouch
      ControlChange         = 0xB0,    ///< Control Change / Channel Mode
      ProgramChange         = 0xC0,    ///< Program Change
      AfterTouchChannel     = 0xD0,    ///< Channel (monophonic) AfterTouch
      PitchBend             = 0xE0,    ///< Pitch Bend
      SystemExclusive       = 0xF0,    ///< System Exclusive
      TimeCodeQuarterFrame  = 0xF1,    ///< System Common - MIDI Time Code Quarter Frame
      SongPosition          = 0xF2,    ///< System Common - Song Position Pointer
      SongSelect            = 0xF3,    ///< System Common - Song Select
      TuneRequest           = 0xF6,    ///< System Common - Tune Request
      Clock                 = 0xF8,    ///< System Real Time - Timing Clock
      Start                 = 0xFA,    ///< System Real Time - Start
      Continue              = 0xFB,    ///< System Real Time - Continue
      Stop                  = 0xFC,    ///< System Real Time - Stop
      ActiveSensing         = 0xFE,    ///< System Real Time - Active Sensing
      SystemReset           = 0xFF,    ///< System Real Time - System Reset
    };

    enum MTCQuarterFrameType
    {
      FramesLow             = 0x00,
      FramesHigh            = 0x10,
      SecondsLow            = 0x20,
      SecondsHigh           = 0x30,
      MinutesLow            = 0x40,
      MinutesHigh           = 0x50,
      HoursLow              = 0x60,
      HoursHighAndSmpte     = 0x70,
    };

    enum SmpteMask
    {
      Frames24              = B0000,
      Frames25              = B0010,
      Frames30drop          = B0100,
      Frames30              = B0110,
    };

    struct Playhead
    {
      byte frames;
      byte seconds;
      byte minutes;
      byte hours;
    };

  private:
    static void sendMTCQuarterFrame(int index);
    static void sendMTCFullFrame();
    static void updatePlayhead();
    static void resetPlayhead();
    static void setPlayhead(byte hours, byte minutes, byte seconds, byte frames);
    static void setTimer(const double frequency);

  private:
    static MidiSynchro                mMode;

    // Midi Clock Stuff
    TapTempo                          mTapTempo;
    static const int                  mMidiClockPpqn;
    static volatile unsigned long     mEventTime;
    static volatile MidiType          mNextEvent;
    static int                        mPrescaler;
    static unsigned char              mSelectBits;
    static volatile byte              mClick;
    static volatile byte              mBeat;
    static volatile bool              mPlaying;

    // MTC stuff
    static const SmpteMask            mCurrentSmpteType;
    static volatile Playhead          mPlayhead;
    static volatile int               mCurrentQFrame;
    static const MTCQuarterFrameType  mMTCQuarterFrameTypes[8];
};

#endif

