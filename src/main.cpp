/** MIDI Faders
    Copyright (C) 2020 Andrea Franceschini <andrea.franceschini@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <LiquidCrystal_I2C.h>
#include <IoAbstraction.h>
#include <Rotary.h>
#include <USB-MIDI.h>

#include <math.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

USBMIDI_CREATE_DEFAULT_INSTANCE();
// This might become useful in the future, when I'll want to include an actual
// MIDI port in my little box. For now, MIDI-over-USB will suffice.
// MIDI_CREATE_DEFAULT_INSTANCE();

// Format string for displaying parameters in columns on the LCD display.
char colFormat[] = " %3d  %3d  %3d  %3d ";
// Char buffer to use sprintf() to format the parameters.
char charBuf[21];
// Array of the CC messages to send when moving each fader.
// Initialised by default to
//  1 Modulation
// 11 Expression
//  3 Undefined (can be learned by your DAW)
//  7 Volume
// These can be changed using the rotary encoders.
unsigned char CCNumbers[] = { 1, 11, 3, 7 };
// The rotary encoders to change the MIDI CC numbers.
Rotary CCNumberEncoders[] = { Rotary(12, 11), Rotary(10, 9), Rotary(8, 7), Rotary(6, 5) };
// Array that holds the values of each fader.
unsigned char CCValues[] = { 0, 0, 0, 0 };
// Array that holds the values of each fader at the previous step. Used to see
// if the value changed so we don't send unnecessary MIDI messages.
unsigned char CCOldValues[] = { 0, 0, 0, 0 };
// Arduino analog pins connected to the faders.
unsigned int faderPins[] = { A0, A1, A2, A3 };

// This updates the display every once in a while. Updating the display is slow
// so we only update the lines that need updating, and we only run this function
// a handful of times per second.
bool shouldUpdateDisplay = true;

void updateDisplay() {
   // unsigned long s = millis(); // DEBUG
   if (!shouldUpdateDisplay) return;

   lcd.setCursor(0, 2);
   snprintf(charBuf, 20, colFormat, CCNumbers[0], CCNumbers[1], CCNumbers[2], CCNumbers[3]);
   lcd.print(charBuf);
   lcd.setCursor(0, 3);
   snprintf(charBuf, 20, colFormat, CCValues[0], CCValues[1], CCValues[2], CCOldValues[3]);
   lcd.print(charBuf);

   shouldUpdateDisplay = false;
   // unsigned long e = millis(); // DEBUG
   // if (e-s > 0) {
   //    Serial.print("LCD: ");
   //    Serial.print(e-s);
   //    Serial.println(" ms");
   // } // This seems to take about 60 ms
}

// DEBUG Counter to see how many loops per second the Arduino can achieve.
// unsigned long loops = 0;
// DEBUG Reports the number of loops achieved via the serial port. This also
// takes up CPU cycles, so the count will not be the most accurate figure, but
// it'll be a decent indication.
// void reportLoops() {
//    Serial.println(loops);
//    loops = 0;
// }

void sendMIDIData() {
   // unsigned long s = millis(); // DEBUG
   for (int i = 0; i < 4; ++i) {
      if (CCOldValues[i] != CCValues[i]) {
         CCOldValues[i] = CCValues[i];
         MIDI.sendControlChange(CCNumbers[i], CCOldValues[i], 1);
         shouldUpdateDisplay = true;
      }
   }
   // unsigned long e = millis(); // DEBUG
   // if (e-s > 0) {
   //    Serial.print("MIDI: ");
   //    Serial.print(e-s);
   //    Serial.println(" ms");
   // } // This seems to take about 1-2 ms
}

void setup() {
   // Not sure why but if I select OMNI as the channel here, the library sends
   // nothing. I need to investigate what the MIDI standard says and whether
   // the library does the right thing. Initialising it to any channel seems
   // to work fine with most DAWs if you only want to change one track at a time
   // which is the goal of this project anyway.
   MIDI.begin(1);
   // 31250 is the standard MIDI baud rate. Higher should work with USB but may
   // fail regular MIDI.
   Serial.begin(31250);

   // Initialise and configures the LCD.
   lcd.init();
   lcd.backlight();
   lcd.clear();
   lcd.setCursor(0, 0);
   // ~~~ Pure vaporwave bliss :) ~~~
   lcd.print(F("Faders        \xcc\xaa\xb0\xc0\xde\xb0"));

   // Initialise the rotary encoders.
   for (int i = 0; i < 4; ++i) {
      CCNumberEncoders[i].begin();
   }
   
   // Send MIDI data as fast as possible, but slow enough to allow the rest of
   // the code to run.
   taskManager.scheduleFixedRate(5, &sendMIDIData);

   // This is a low priority task, so we can schedule this every few hundreds of
   // milliseconds.
   taskManager.scheduleFixedRate(50, &updateDisplay);
   
   // DEBUG This schedules the reporting of loops per second. No need to have it
   // if we don't need it.
   // taskManager.scheduleFixedRate(1000, &reportLoops);
}

void loop() {
   // This polls all the controls, faders and encoders. Polling the encoders
   // may be a waste of time as they are basically guaranteed to not change most
   // of the time. We could save some time by moving the encoders to interrupts
   // but that would require a change in the circuitry and so I'll add this to
   // the list of improvements for mk.2.
   for (int i = 0; i < 4; ++i) {
      if (i == 0 || i == 2) {
         CCValues[i] = (unsigned char) 127.0 * (fmax(0.0, (float)analogRead(faderPins[i]) - 10)) / 1012.0;
      } else {
         CCValues[i] = 128 * analogRead(faderPins[i]) / 1024;
      }

      unsigned char direction = CCNumberEncoders[i].process();
      if (direction && direction == DIR_CCW && CCNumbers[i] > 1) {
         CCNumbers[i]--;
         shouldUpdateDisplay = true;
      } else if (direction && direction == DIR_CW && CCNumbers[i] < 127) {
         CCNumbers[i]++;
         shouldUpdateDisplay = true;
      }
   }
   // loops++; // DEBUG
   // This is actually required to run the tasks we set up above.
   taskManager.runLoop();
}
