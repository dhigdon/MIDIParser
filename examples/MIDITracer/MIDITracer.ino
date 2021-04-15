// MIDI Tracer
// Connect MIDI RX=PIN 2, TX=PIN 3

#include <SoftwareSerial.h>
#include <MIDIparser.h>

// NOTE: use software serial so we can still use TX/RX for communications with the PC
// Arduino UNO only has interrupts on pins 2&3, so use those for our input.
SoftwareSerial midi_port( 2, 3 );

// Our parser
MIDI::Parser parser;

// For commands of 0xf0 and greater (Both SYS and RS names)
char const midi_sysmsg_names[][7] =
{
   "SYSEX ", "TMCODE", "SNGPOS", "SNGSEL", "      ", "      ", "TUNE  ", "ENDEX ",
   "CLOCK ", "      ", "START ", "CONT  ", "STOP  ", "      ", "SENSE ", "RESET "
};

// Otherwise, these are channel messages
char const midi_msg_names[][8] =
{
   "NOTOFF ", "NOTEON ", "ATOUCH ", "CC CHG ", "PRGCHG ", "CTOUCH ", "P BEND "
};

void setup()
{
   Serial.begin(9600);
   parser.reset();
   // Set up the MIDI serial rate and wait for the part to be ready
   midi_port.begin(31250);
   delay(100);

   Serial.println("Waiting for data...");
}

void loop()
{
   while (midi_port.available() > 0)
   {
      byte data = midi_port.read();
      byte msg = parser.accept(data);

      if (msg != 0)
      {
         if (msg < MIDI::SYSEX)
         {
            Serial.print((int)msg, HEX);
            Serial.print(" CH ");
            Serial.print(MIDI::msg_channel(msg));
            Serial.print(": ");
            Serial.print(midi_msg_names[(msg >> 4) & 0x07]);
            Serial.print(' ');
            if (MIDI::msg_value(msg) == MIDI::PITCH_BEND)
            {
               Serial.print(parser.get_int14(), HEX);
            }
            else
            {
               Serial.print(parser.get_dataA(), HEX);
               Serial.print(", ");
               Serial.print(parser.get_dataB(), HEX);
            }
            Serial.println("");
         }
         else if (msg != MIDI::RT_CLOCK)
         {
            Serial.print("SYS: ");
            Serial.print(midi_sysmsg_names[msg & 0x0f]);
            Serial.println("");
         }
      }
   }
}

