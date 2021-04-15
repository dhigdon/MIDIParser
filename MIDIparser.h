// MIDI Parser - a library for parsing MIDI data message
// (C)2021 Dan Higdon

#pragma once
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

      // 0x80 - 0xE0 are channel messages
      NOTE_OFF    = 0x80,  // dataA = Note   dataB = Velocity
      NOTE_ON     = 0x90,  // dataA = Note   dataB = Velocity
      AFTERTOUCH  = 0xA0,  // dataA = Note   dataB = Pressure
      CC_CHANGE   = 0xB0,  // dataA = CC     dataB = Value
      PROG_CHANGE = 0xC0,  //                dataB = Patch#
      CH_TOUCH    = 0xD0,  //                dataB = Pressure
      PITCH_BEND  = 0xE0,  // Bend (14 bits, center on 0x2000)

      // 0xF0 and above are SYSTEM COMMON messages.
      // Their "channel" field determines the specific message.
      SYSEX       = 0xF0,  // bulk dump data, ends with ENDEX
      MTCQFRAME   = 0xF1,  // dataB = quarter frame number
      SPP         = 0xF2,  // 14 bits of beats (1 beat == 6 Clocks)
      SONG_SELECT = 0xF3,  // dataB = song#
      UNDEF_1     = 0xF4,
      UNDEF_2     = 0xF5, 
      TUNE_REQ    = 0xF6,  // no data, return oscillators
      ENDEX       = 0xF7,  // ends a SYSEX dump

      // MIDI Realtime messages can interrupt other messages, and have no data
      RT_CLOCK    = 0xF8,  // 24 PPQ
      RT_UNDEF_1  = 0xF9,
      RT_START    = 0xFA,  // Begin playing, reset current position
      RT_CONTINUE = 0xFB,  // Resume playing from the current position
      RT_STOP     = 0xFC,  // Stop playing, recording current position
      RT_UNDEF_2  = 0xFD,
      RT_SENSE    = 0xFE,  // Sent every 300ms on a live connection. Can ignore.
      RT_RESET    = 0xFF,  // Reset any parameters to their power up values

      // "Break points" in the message message space
      SYS_MSGS    = SYSEX,
      RT_MSGS     = RT_CLOCK,
   };

   // --------------------------------------------------------------------------
   // Some CC values have predetermined meanings, and represent changes to
   // the actual synthesizer's state, rather than performance values.

   enum CC
   {
      CC_MODWHEEL       = 1,     // 0-127
      CC_BREATH         = 2,     // 0-127
      CC_VOLUME         = 7,     // Channel mix volume
      CC_PAN            = 10,    // 0-127, 64=centered
      CC_EXPRESSION     = 11,    // playback dynamics
      CC_SUSTAIN        = 64,    // 0=off, 127=on
      CC_PORTAMENTO     = 65,    // 0=off, 127=on

      // CHANNEL MODE MESSAGES
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

   constexpr bool is_msg( uint8_t msg )         { return (msg & 0x80) == 0x80; }
   constexpr bool is_sysmsg( uint8_t msg )      { return (msg & 0xf8) == 0xf0; }
   constexpr bool is_rtmsg( uint8_t msg )       { return (msg & 0xf8) == 0xf8; }
   constexpr uint8_t msg_value( uint8_t msg )   { return msg & 0xf0; }
   constexpr uint8_t msg_channel( uint8_t msg ) { return msg & 0x0f; }

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
         explicit Parser() { reset(); }

         // Clear out the parser's data
         void reset();

         // Accept the next character of data.
         // Returns 0 if no message, or an integer equal to the enum Message
         // value of a message which has been parsed. Used the get_data
         // functions to retrieve parameters appropriate to each message type.
         uint8_t accept( char data );

         // Messages and access to their parameters.

         // Note that these functions are only meaningful if accept() returns a nonzero
         // value, but the bytes are always available for inspection.
         uint8_t get_message() const  { return mMessage; }

         // Use get_data[A|B] to retrieve message data bytes.
         // 1 byte  - dataB
         // 2 bytes - dataA, dataB
         // There are no 3 byte messages
         uint8_t get_dataA() const    { return mData[0]; }
         uint8_t get_dataB() const    { return mData[1]; }

         // Some messages use a 14bit parameter
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
         int8_t   mExpected;  // The remaining parameter byte count for this message
         uint8_t  mData[2];   // Message data
   };

   // --------------------------------------------------------------------------

   inline uint16_t Parser::get_int14() const
   {
      return ((uint16_t)mData[1] << 7) | (uint16_t)mData[0];
   }
}

#endif

