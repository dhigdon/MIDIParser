// MIDI Parser - a library for parsing MIDI data message
// (C)2021 Dan Higdon

#ifndef MIDI_PARSER
#define MIDI_PARSER

#include <inttypes.h>

// --------------------------------------------------------------------------

namespace MIDI
{
   // --------------------------------------------------------------------------
   // In the MIDI protocol, all uint8_ts with the high bit set are MESSAGES,
   // and all uint8_ts with the high bit cleared are DATA.

   // Channel messages are 0b1xxxyyyy, where xxx is the opcode, and yyyy is the
   // channel they come from. The values here do not include the channels.
   enum Message
   {
      NONE        = 0x00,  // The NULL message, included for completeness

      // 0x80 - 0xE0 are channel messages, and the low nibble is the channel number.
      NOTE_OFF    = 0x80,  // Note, Velocity
      NOTE_ON     = 0x90,  // Note, Velocity
      AFTERTOUCH  = 0xA0,  // Note, Pressure
      CC_CHANGE   = 0xB0,  // CC, Value
      PROG_CHANGE = 0xC0,  // Patch#, None
      CH_TOUCH    = 0xD0,  // Pressure, None
      PITCH_BEND  = 0xE0,  // 14 bits

      // 0xF0 and above are SYSTEM COMMON messages.
      // Their "channel" field determines the message.
      SYSEX       = 0xF0,  // bulk dump data, ends with MIDI_ENDEX
      MTCQFRAME   = 0xF1,  // expects 1 byte, a "quarter frame"
      SPP         = 0xF2,  // 14 bits of beats (1 beat == 6 Clocks)
      SONG_SELECT = 0xF3,  // expects 1 byte for song#
      UNDEF_1     = 0xF4,
      UNDEF_2     = 0xF5, 
      TUNE_REQ    = 0xF6,  // no data, return oscillators
      ENDEX       = 0xF7,  // ends a SYSEX dump

      // MIDI Realtime messages can interrupt other messages
      RT_CLOCK    = 0xF8,  // 24 PPQ
      RT_UNDEF_1  = 0xF9,
      RT_START    = 0xFA,  // Begin playing the sequence, "From the top!"
      RT_CONTINUE = 0xFB,  // Resume playing from the current position
      RT_STOP     = 0xFC,  // Stop playing, remembering the position for a later RESUME
      RT_UNDEF_2  = 0xFD,
      RT_SENSE    = 0xFE,  // Sent every 300ms on a live connection. Can ignore.
      RT_RESET    = 0xFF,  // Reset any parameters to their power up values

      // "Break points" in the message message space
      SYS_MESSAGES= SYSEX,
      RT_MESSAGES = RT_CLOCK,
   };

   // --------------------------------------------------------------------------
   // Some CC values have predetermined meanings, and represent changes to
   // the actual synthesizer's state, rather than performance values.
   enum CC
   {
      CC_MODWHEEL       = 1,
      CC_BREATH         = 2,
      CC_VOLUME         = 7,     // overall channel volume
      CC_PAN            = 10,    // 64=centered
      CC_EXPRESSION     = 11,    // volume use expressively
      CC_SUSTAIN        = 64,    // 0=off, 127=on
      CC_PORTAMENTO     = 65,    // 0=off, 127=on

      // These CC's are for the SYNTH, not the note
      CC_RESET          = 121,   // None. Also called "All Sound Off"
      CC_MODE_LOCAL     = 122,   // 0=off, 127=on. Keyboard on/off
      CC_ALL_NOTES_OFF  = 123,   // None
      CC_OMNI_OFF       = 124,   // None
      CC_OMNI_ON        = 125,   // None
      CC_POLY_OFF       = 126,   // #channels (MONO ON)
      CC_POLY_ON        = 127,   // None (MONO OFF)
   };

   // --------------------------------------------------------------------------
   // Categorize the MIDI message
   constexpr bool is_cmd( uint8_t cmd )         { return (cmd & 0x80) == 0x80; }
   constexpr bool is_syscmd( uint8_t cmd )      { return (cmd & 0xf8) == 0xf0; }
   constexpr bool is_rtcmd( uint8_t cmd )       { return (cmd & 0xf8) == 0xf8; }
   constexpr uint8_t msg_value( uint8_t cmd )   { return cmd & 0xf0; }
   constexpr uint8_t msg_channel( uint8_t cmd ) { return cmd & 0x0f; }

   // --------------------------------------------------------------------------

   // This class is a "midi machine". Feed it chars from a serial port,
   // and accept()'s return value will tell you when there is a message.
   // This is a very "low rent" piece of code in that it takes few cycles or
   // uint8_ts to parse the midi stream.

   // Instantiate a separate MIDIParser for each physical serial port you
   // will be reading from.

   class Parser
   {
      public:
         Parser() { reset(); }

         // Clear out the parser's data
         void reset();

         // Accept the next character of data.
         // Returns 0 if no message, or an integer equal to the enum Message
         // value of a message which has been parsed.
         uint8_t accept( char data );

         // Messages and access to their parameters.
         // Note that these are only meaningful if accept() returns a nonzero
         // value, but the bytes are always available.
         uint8_t get_message() const  { return mMessage; }
         uint8_t get_dataA() const    { return mData[0]; }
         uint8_t get_dataB() const    { return mData[1]; }

         // Some messages use a 14bit parameter, sent LSB, MSB
         uint16_t get_int14() const;

         // NOTE about SYSEX data
         // If accept() return SYSEX, the caller should process all following
         // bytes until ENDEX is received. The parser will dutifully ignore
         // SYSEX data until the ENDEX is received, so the application CAN
         // continue to accept() data bytes. If the application does not care
         // about SYSEX data, then it can ignore the existence of such data
         // safely.

      private:

         uint8_t  mMessage;   // The message being worked on
         int8_t   mExpected;  // The remaining parameter bytes for this message
         uint8_t  mData[2];   // Message data (up to 2 bytes whose meanings vary)
   };

   // --------------------------------------------------------------------------

   inline uint16_t Parser::get_int14() const
   {
      return ((uint16_t)mData[1] << 7) | (uint16_t)mData[0];
   }
}

#endif
