# MIDI Faders mk.1

A simple but not trivial Arduino project with four sliders and four rotary
encoders to send MIDI CC values to my DAW and better control the performance
of my VST instruments.

Schematics and stuff to come, but basically I plugged four linear slide pots
to the A0-A3 pins of an Arduino Micro to control the values, and four 24-steps
rotary encoders to the D5-D12 pins to select the CC number for each fader.

Then I have a totally overkill 20x4 LCD display which is entirely unnecessary
but so gorgeous I couldn't resist. This is also possibly the biggest bottleneck
of the project, so you may want to be a little more frugal. It's I2C anyway.
