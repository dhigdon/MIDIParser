// MIDI Parser - library for reading MIDI data messages
// (C)2021 Dan Higdon

#include "MIDIparser.h" 

// ----------------------------------------------------------------------------

namespace
{
   // Each MIDI message expects a certain number of bytes.
   // This table maps the channel messages into their expected bytes.
   // Note that no value may be greater than 2 in this table.
   int8_t const midi_msg_bytes[7] =
   {
      2, 2, 2, 2, 1, 1, 2
   };

   // System messages by and large do not take parameters.
   // Yet because MIDI is cruel, some of them do.
   // SYSEX has a negative expected bytes, which essentially pauses
   // the parser until another MESSAGE byte is seen.
   int8_t const midi_sysmsg_bytes[8] =
   {
      -1, 1, 2, 1, 0, 0, 0, 0
   };

   // Returns the number of bytes expected by the given 
   // CHANNEL MESSAGE or SYSTEM COMMON MESSAGE
   // 'msg' must be a msg byte, and not a REALTIME message
   int8_t expected_bytes(uint8_t msg)
   {
      if (msg < MIDI::SYS_MSGS)
         return midi_msg_bytes[(msg >> 4) & 7];
      else
         return midi_sysmsg_bytes[msg & 0x7];
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

uint8_t MIDI::Parser::accept(char midiByte)
{
   // RT messages interrupt other messages, and do not "trash" their reception.
   if (is_rtmsg(midiByte))
   {
      return midiByte;
   }

   if (is_msg(midiByte))
   {
      // Remember the message and see how many more bytes to expect
      mMessage = midiByte;
      mExpected = expected_bytes(mMessage);

      // SYSEX is a special case, and needs an early return.
      if (mExpected < 0)
         return mMessage;
   }
   else if (mExpected > 0)
   {
      // This is an expected databyte - store it.
      // NOTE: mExpected will never be greater than 2.
      mData[2 - mExpected] = midiByte;
      --mExpected;
   }

   // A message is fully formed when it is not expecting any more data bytes.
   if (mExpected == 0)
   {
      // Reset mExpected for RUNNING STATUS.
      mExpected = expected_bytes(mMessage);
      return mMessage;
   }

   // This is an expected data byte, or a SYSEX data byte.
   // Return 0 so that the caller knows there is no new message available.
   return 0;
}

