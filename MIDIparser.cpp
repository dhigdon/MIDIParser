// MIDI Parser - library for reading MIDI data messages
// (C)2021 Dan Higdon

#include "MIDIparser.h" 

// ----------------------------------------------------------------------------

// The parser should be fed bytes once a SYSEX is detected,
// but it will return Message::None until the ENDEX happens.
// The caller can use that as a signal to stop storing bytes
// and do something with them, or it can just ignore SYSEX and ENDEX,
// safe in the knoweldge that SYSEX data will not be interpreted
// as MIDI messages.

namespace
{
   // Each MIDI message expects a certain number of bytes.
   // This table maps the channel messages into their expected bytes.
   int8_t const midi_msg_bytes[7] =
   {
      2, 2, 2, 2, 1, 2, 2,
   };

   // System messages by and large do not take parameters.
   // Yet because MIDI is cruel, some of them do.
   // SYSEX accepts bytes until ENDEX happens, and the processing
   // of that message is outside of the scope of this code.
   int8_t const midi_sysmsg_bytes[7] =
   {
      -1, 1, 2, 1, 0, 0, 0
   };

   // Returns the number of bytes expected by the given 
   // CHANNEL MESSAGE or SYSTEM COMMON message
   int8_t expected_bytes(uint8_t message)
   {
      // Note that message MUST be an actual message.

      // CHANNEL MESSAGES are all messsage before SYSTEM COMMON messages,
      // and are distinguished by the low 3 bits of their high nibble.
      if (message < MIDI::SYS_MESSAGES)
         return midi_msg_bytes[(message >> 4) & 7];

      // SYSTEM COMMON messages are distinguished by their low 3 bits.
      if (message < MIDI::RT_MESSAGES)
         return midi_sysmsg_bytes[message & 0x7];

      // REALTIME messages never have any parameter bytes
      return 0;
   }
}

// ----------------------------------------------------------------------------

void MIDI::Parser::reset()
{
   mMessage = 0;
   mExpected = 0;
   mData[0] = mData[1] = 0;
}

// ----------------------------------------------------------------------------

uint8_t MIDI::Parser::accept( char midiByte )
{
   // RT messages interrupt other messages, and do not "trash" their reception.
   if ( is_rtcmd( midiByte ) )
   {
      return midiByte;
   }

   if ( is_cmd( midiByte ) )
   {
      // Remember the message and see how many more bytes to expect
      mMessage = midiByte;
      mExpected = expected_bytes( mMessage );

      // SYSEX is a special case - if mExpected is negative, then
      // return the SYSEX message
      if (mExpected < 0)
         return mMessage;
   }
   else if (mExpected > 0)
   {
      // This is a databyte - if we're expecting it, store it.
      // NOTE: SYSEX data expects -1 bytes, and does not come through here.
      // NOTE: mExpected will never be greater than 2.
      mData[2 - mExpected] = midiByte;
      --mExpected;
   }

   // If we are not expecting any more bytes, then this is a fully
   // formed message - return it.
   // But reset mExpected to support Running Status
   if (mExpected == 0)
   {
      mExpected = expected_bytes( mMessage );
      return mMessage;
   }

   return 0;
}

