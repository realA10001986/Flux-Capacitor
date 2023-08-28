/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Flux Capacitor
 * (C) 2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Flux-Capacitor
 *
 * License: MIT
 * 
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of the 
 * Software, and to permit persons to whom the Software is furnished to 
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Build instructions (for Arduino IDE)
 * 
 * - Install the Arduino IDE
 *   https://www.arduino.cc/en/software
 *    
 * - This firmware requires the "ESP32-Arduino" framework. To install this framework, 
 *   in the Arduino IDE, go to "File" > "Preferences" and add the URL   
 *   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
 *   to "Additional Boards Manager URLs". The list is comma-separated.
 *   
 * - Go to "Tools" > "Board" > "Boards Manager", then search for "esp32", and install 
 *   the latest version by Espressif Systems.
 *   Detailed instructions for this step:
 *   https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html
 *   
 * - Go to "Tools" > "Board: ..." -> "ESP32 Arduino" and select your board model (the
 *   CircuitSetup original boards are "NodeMCU-32S")
 *   
 * - Connect your ESP32 board.
 *   Note that NodeMCU ESP32 boards come in two flavors that differ in which serial 
 *   communications chip is used: Either SLAB CP210x USB-to-UART or CH340. Installing
 *   a driver might be required.
 *   Mac: 
 *   For the SLAB CP210x (which is used by NodeMCU-boards distributed by CircuitSetup)
 *   installing a driver is required:
 *   https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads
 *   The port ("Tools -> "Port") is named /dev/cu.SLAB_USBtoUART, and the maximum
 *   upload speed ("Tools" -> "Upload Speed") can be used.
 *   The CH340 is supported out-of-the-box since Mojave. The port is named 
 *   /dev/cu.usbserial-XXXX (XXXX being some random number), and the maximum upload 
 *   speed is 460800.
 *   Windows: No idea. Not been using Windows since 1999.
 *
 * - Install required libraries. In the Arduino IDE, go to "Tools" -> "Manage Libraries" 
 *   and install the following libraries:
 *   - WifiManager (tablatronix, tzapu) https://github.com/tzapu/WiFiManager
 *     (Tested with 2.0.13beta, 2.0.15-rc1, 2.0.16-rc2)
 *   - ArduinoJSON >= 6.19: https://arduinojson.org/v6/doc/installation/
 *
 * - Download the complete firmware source code:
 *   https://github.com/realA10001986/Flux-Capacitor/archive/refs/heads/main.zip
 *   Extract this file somewhere. Enter the "fluxcapacitor-A10001986" folder and 
 *   double-click on "fluxcapacitor-A10001986.ino". This opens the firmware in the
 *   Arduino IDE.
 *
 * - Go to "Sketch" -> "Upload" to compile and upload the firmware to your ESP32 board.
 *
 * - Install the audio files: 
 *   - Copy the contents of install/sound-pack-xxxxxxxx.zip in the top folder of a FAT32 
 *     (not ExFAT!) formatted SD card (max 32GB) and put this card into the slot while
 *     the device is powered down. Now power-up the device.
 *   - The audio files will be installed automatically, SD no longer needed afterwards.
 *     (but is recommended to be left in slot for saving settings and avoiding flash
 *     wear on the ESP32.)
 */

/*  Changelog
 *   
 *  2023/08/28 (A10001986)
 *    - Adapt to TCD's WiFi name appendix option
 *    - Add "AP name appendix" setting; allows unique AP names when running multiple 
 *      FCs in AP mode in close range. 7 characters, 0-9/a-z/A-Z/- only, will be 
 *      added to "FC-AP".
 *    - Add AP password: Allows to configure a WPA2 password for the FC's AP mode
 *      (empty or 8 characters, 0-9/a-z/A-Z/- only)
 *    - *123456OK not only clears static IP config (as before), but also clears AP mode 
 *      WiFi password.
 *  2023/08/25 (A10001986)
 *    - Remove "Wait for TCD WiFi" option - this is not required; if the TCD is acting
 *      access point, it is supposed to be in car mode, and a delay is not required.
 *      (Do not let the TCD search for a configured WiFi network and have the FC rely on 
 *      the TCD falling back to AP mode; this will take long and the FC might time-out 
 *      unless the FC has a couple of connection retrys and long timeouts configured! 
 *      Use car mode, or delete the TCD's configured WiFi network if you power up the
 *      TCD and FC at the same time.)
 *    - Some code cleanups
 *    - Restrict WiFi Retrys to 10 (like WiFiManager)
 *  2023/08/24 (A10001986)
 *    - Add "Wait for fake power on" option; if set, FC only boots
 *      after it received a fake-power-on signal from the TCD
 *      (Needs "Follow fake power" option set)
 *    - Play startup-sequence on fake-power-on
 *    - Don't activate ss when tt is running
 *    - Fix parm handling of FPO and NM in fc_wifi
 *    - Reduce volume in night mode
 *  2023/08/14 (A10001986) [1.0]
 *    - Add config option to disable the default IR control
 *  2023/07/29 (A10001986)
 *    - Changes for board 1.3
 *  2023/07/22 (A10001986)
 *    - BTTFN dev type
 *    - Minor CP design changes
 *  2023/07/11 (A10001986)
 *    - Fix possible div/0
 *    - Fix alarm vs. screen saver
 *    - Remove redundant "startFluxTimer" calls
 *  2023/07/10 (A10001986)
 *    - BTTFN: Fix for ABORT_TT
 *  2023/07/09 (A10001986)
 *    - BTTFN: Add night mode and fake power support (both signalled from TCD)
 *    - Allow chase speed to change with GPS speed from TCD (if available)
 *  2023/07/07 (A10001986)
 *    - Add "BTTF Network" (aka "BTTFN") support: TCD controls other props, such as FC
 *      or SID, wirelessly. Used here for time travel synchronisation and TCD's "Alarm" 
 *      feature. Only needs IP address (not hostname!) of TCD to be entered in CP. 
 *      Either MQTT or BTTFN is used for time travel synchronisation/alarm; if the TCD is 
 *      configured to use MQTT and the "Send commands to other props" option is checked, 
 *      it will not send notifications via BTTFN.
 *      BTTFN allows wireless communication without a broker (such as with MQTT), and
 *      also works when TCD acts as WiFi access point for FC and other props. Therefore,
 *      it is ideal for car setups.
 *    - Add "screen saver": Deactivate all LEDs after a configurable number of minutes
 *      of inactivity. TT button press, IR control key, time travel deactivates screen
 *      saver. (If IR is locked, only '#' key will deactivate ss.)
 *    - Change "minimum box light level" levels.
 *  2023/07/04 (A10001986)
 *    - Minor fixes
 *  2023/07/03 (A10001986)
 *    - Add "minimum box light level": Box lights will always be at that minimum 
 *      level in order to bring some light into the box. 5 Levels available, chosen
 *      by *10 through *14 followed by OK.
 *    - Add flux sound modes 2 and 3 (30/60 seconds after power-up and time travel,
 *      like TCD's beep modes)
 *    - Re-assigned knob usage command sequences from *1x to *8x
 *    - Save ir lock status (*70) and minimum box light level on the fly so that 
 *      they are persistent.
 *  2023/06/27 (A10001986)
 *    - Change names of most config files so that they are device specific
 *    - *70OK locks IR; as long as IR is locked, no keys or commands are accepted
 *      except *70OK to unlock. This makes it possible to use the same type of
 *      remote control for FC and SID.
 *  2023/06/26 (A10001986)
 *    - Fix for isIP()
 *  2023/06/21 (A10001986)
 *    - Add TCD-like mp3-renamer
 *    - Have IP read out loud on *90OK
 *  2023/06/20 (A10001986)
 *    - Add TCD-like key3.mp3 und key6.mp3 playback on pressing 3/6
 *    - Add TCD-like *64738OK reset code
 *  2023/06/19 (A10001986)
 *    - Stop audio upon OTA fw update
 *  2023/06/17 (A10001986)
 *    - Changed TT sequence
 *    - Save speed like volume (unless pot is used)
 *    - *20OK resets speed to default
 *    - New sounds
 *    - (MQTT) Alarm sequence synced with (default) sound
 *  2023/06/16 (A10001986)
 *    - Default IR codes for included remote added
 *    - Time travel: Play flux if music player was active
 *  2023/06/12 (A10001986)
 *    - Show "wait" signal while formatting flash FS
 *    - IR codes now saved on SD if respective config option set (like volume)
 *    - Don't show/run audio file installer if in FlashROMode
 *    - Stop audio before changing music folder; mp_init() can take a while.
 *    - Change TT sequence in re-entry part
 *    - Add option to swap Box Light and Panel Light; using high power LEDs
 *      for box illumination requires external power, which the PL connector
 *      provides, along with PWM. (As long as there is no separate IR feedback
 *      pin, the other connector is used for IR feedback.)
 *  2023/06/11 (A10001986)
 *    - Add IR learning: Hold TT key for 7 seconds to start learning, fc LEDS 
 *      will stop, then blink twice with all LEDs. Now press keys in order 0, 1, 2, 
 *      3, 4, 5, 6, 7, 8, 9, *, #, up, down, left, right, OK/enter, each after 
 *      a double blink, and while the IR feedback LED blinks.
 *      Short pressing the TT button aborts learning, as does inactivity for 10 
 *      seconds.
 *  2023/06/10 (A10001986)
 *    - Speed pot: Use lower granularity to avoid wobble, and use a translation
 *      table for non-linear acceleration
 *  2023/06/09 (A10001986)
 *    - Make IR codes user-configurable (ir_keys.txt on SD)
 *    - Give IR key feed back (currently through "Panel LED")
 *    - Change default chase speed
 *    - Re-do speed up/downv via remote
 *    - Add alternative sequences (*xOK)
 *    - Change some code sequences (knobs are now *1x)
 *    - Fix IR key repeat (needs testing with various remotes)
 *    - etc.
 *  2023/06/08 (A10001986)
 *    - Add IR command structure & commands
 *    - Add TT sequences
 *  2023/05/02 (A10001986)
 *    - Add HA/MQTT code from TCD; change CP header like TCD
 *  2023/04/21 (A10001986)
 *    - Shrink Web page header gfx
 *  2023/03/28 (A10001986)
 *    - Initial version
 */

#include "fc_global.h"

#include <Arduino.h>

#include "fc_audio.h"
#include "fc_settings.h"
#include "fc_wifi.h"
#include "fc_main.h"

void setup()
{
    powerupMillis = millis();
    
    Serial.begin(115200);
    Serial.println();

    main_boot();
    settings_setup();
    wifi_setup();
    audio_setup();
    main_setup();
}

void loop()
{    
    main_loop();
    audio_loop();
    wifi_loop();
    audio_loop();
    bttfn_loop();
}
