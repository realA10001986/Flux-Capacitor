/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Flux Capacitor
 * (C) 2023-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Flux-Capacitor
 * https://fc.out-a-ti.me
 * This code was written without the use of artificial intelligence.
 *
 * License: Modified MIT NON-AI
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
 * Links inside the Software pointing to the original source must not 
 * be changed or removed.
 *
 * In addition, the following restrictions apply:
 * 
 * 1. The Software and any modifications made to it may not be used 
 * for the purpose of training or improving machine learning algorithms, 
 * including but not limited to artificial intelligence, natural 
 * language processing, or data mining. This condition applies to any 
 * derivatives, modifications, or updates based on the Software code. 
 * Any usage of the Software in an AI-training dataset is considered a 
 * breach of this License.
 *
 * 2. The Software may not be included in any dataset used for 
 * training or improving machine learning algorithms, including but 
 * not limited to artificial intelligence, natural language processing, 
 * or data mining.
 *
 * 3. Any person or organization found to be in violation of these 
 * restrictions will be subject to legal action and may be held liable 
 * for any damages resulting from such use.
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
 *   run Arduino IDE, go to "File" > "Preferences" and add the URL   
 *   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
 *   to "Additional Boards Manager URLs". The list is comma-separated.
 *   
 * - Go to "Tools" > "Board" > "Boards Manager", then search for "esp32", and install 
 *   the latest 2.x version by Espressif Systems. Versions >=3.x are not supported.
 *   Detailed instructions for this step:
 *   https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html
 *   
 * - Go to "Tools" > "Board: ..." -> "ESP32 Arduino" and select your board model (the
 *   CircuitSetup original boards are "NodeMCU-32S")
 *   
 * - Connect your ESP32 board using a suitable USB cable.
 *   Note that NodeMCU ESP32 boards come in two flavors that differ in which serial 
 *   communications chip is used: Either SLAB CP210x USB-to-UART or CH340. Installing
 *   a driver might be required.
 *   Mac: 
 *   For the SLAB CP210x (which is on the NodeMCU-boards distributed by CircuitSetup)
 *   installing a driver is required:
 *   https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads
 *   The port ("Tools -> "Port") is named /dev/cu.SLAB_USBtoUART, and the maximum
 *   upload speed ("Tools" -> "Upload Speed") can be used.
 *   The CH340 is supported out-of-the-box since Mojave. The port is named 
 *   /dev/cu.usbserial-XXXX (XXXX being some random number), and the maximum upload 
 *   speed is 460800.
 *   Windows:
 *   For the SLAB CP210x (which is used by NodeMCU-boards distributed by CircuitSetup)
 *   installing a driver is required:
 *   https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads
 *   After installing this driver, connect your ESP32, start the Device Manager, 
 *   expand the "Ports (COM & LPT)" list and look for the port with the ESP32 name.
 *   Choose this port under "Tools" -> "Port" in Arduino IDE.
 *   For the CH340, another driver is needed. Try connecting the ESP32 and have
 *   Windows install a driver automatically; otherwise search google for a suitable
 *   driver. Note that the maximum upload speed is either 115200, or perhaps 460800.
 *
 * - Install required libraries. In the Arduino IDE, go to "Tools" -> "Manage Libraries" 
 *   and install the following libraries:
 *   - ArduinoJSON (>= 6.19): https://arduinojson.org/v6/doc/installation/
 *
 * - Download the complete firmware source code:
 *   https://github.com/realA10001986/Flux-Capacitor/archive/refs/heads/main.zip
 *   Extract this file somewhere. Enter the "fluxcapacitor-A10001986" folder and 
 *   double-click on "fluxcapacitor-A10001986.ino". This opens the firmware in the
 *   Arduino IDE.
 *
 * - Go to "Sketch" -> "Upload" to compile and upload the firmware to your ESP32 board.
 *
 * - Install the sound-pack: 
 *   Method 1:
 *   - Go to Config Portal, click "Update & Upload" and upload the sound-pack (FCA.bin,
 *     extracted from install/sound-pack-fcXX.zip) through the bottom file selector.
 *     An SD card must be present in the slot during this operation.
 *   Method 2:
 *   - Copy FCA.bin to the top folder of a FAT32 (not ExFAT!) formatted SD card (max 
 *     32GB) and put this card into the slot while the FC is powered down. 
 *   - Now power-up. The sound-pack will now be installed. When finished, the FC will 
 *     reboot.
 */

/*  Changelog
 *
 *  2026/10/18 (A10001986) [2.00]
 *    ********************************************************************************
 *    ** If updating from below 1.100, please see boxed note at version 1.101 below **
 *    ********************************************************************************
 *    - Re-arrange WiFi Configuration page for a better user experience
 *    - BTTFN: "Hostname of TCD" now defaults to "timecircuits" instead of being empty.
 *      As a result, a factory-fresh FC now connects to a TCD in the network without
 *      any user configuration changes.
 *    - IR command sequence *123456ok now only deletes FC-AP password, and *123457ok
 *      deletes static IP config. Both lead to a reboot if applicable.
 *    - IR command sequence *397159ok, when entered twice, resets all settings to
 *      factory defaults.
 *    - New sound-pack (FC07)
 *    - Future firmware updates may only consist of one single file instead of "main 
 *      firmware" plus "sound-pack". Firmware updates will require an SD card.
 *      Future sound-pack uploads, if any, are done through the top file selector, and
 *      no longer through the bottom one. The bottom one is for custom mp3 file upload
 *      only.
 *  2026/09/21 (A10001986) [1.108]
 *    ********************************************************************************
 *    ** If updating from below 1.100, please see boxed note at version 1.101 below **
 *    ********************************************************************************
 *    - Car Mode: Automatically pair FC to currently connected TCD by means of the
 *      TCD's BSSID upon first successful connection in Car Mode. This avoids connecting
 *      to other people's TCDs if in close range (eg on meet-ups).
 *  2026/09/18 (A10001986) [1.107]
 *    ********************************************************************************
 *    ** If updating from below 1.100, please see boxed note at version 1.101 below **
 *    ********************************************************************************
 *    - New sound pack (FC06)
 *    - IR Learning: Each key now needs to pressed twice to sort out unsuitable remotes
 *      sending alternate codes on consecutive key presses.
 *    - Add option to use key "9" on IR remote to refill Plutonium (requires TCD with
 *      firmware >= 3.27, and Dash Gauges).
 *    - Fix minor issues with ID3v1 tags
 *    - Speed up booting with folders containing many files
 *    - Music Player: The "TCD_DONE.TXT" file is now obsolete. The firmware instead uses
 *      a cache file in the top-most folder of the SD card ("musicXc") which needs to be
 *      deleted when files are added to the respective folder.
 *    - Bonjour/mDNS: Send good-bye packet on controlled reboots
 *    - Protect the FC from uploading a wrong firmware by accident. The filename of
 *      the firmware binary now must contain the word "flux". The check is case-
 *      insensitive.
 *    - Minor fixes in connection with feedback signals
 *  2026/08/19 (A10001986) [1.106]
 *    ********************************************************************************
 *    ** If updating from below 1.100, please see boxed note at version 1.101 below **
 *    ********************************************************************************
 *    - New sound pack (FC05)
 *    - IR learning: Keys to be pressed are now announced for more convenience
 *    - IR learning bugfix: Reinstate sound (flux, music) after learning
 *    - Unmute audio if muted while speaking IP address
 *    - More robust error handling in case of malformed config files
 *    - SD: More compatibility fixes
 *    - Code maintenance
 *  2026/07/17 (A10001986) [1.105]
 *    ********************************************************************************
 *    ** If updating from below 1.100, please see boxed note at version 1.101 below **
 *    ********************************************************************************
 *    - Bugfix: Fix SD initialization in ESP32-Arduino to conform to specs.
 *      Sandisk Ultra 32GB cards (and possibly others that previously were
 *      not usable) are now recognized.
 *  2026/06/24 (A10001986) [1.104]
 *    ********************************************************************************
 *    ** If updating from below 1.100, please see boxed note at version 1.101 below **
 *    ********************************************************************************
 *    - IR learning timeout increased to 20 seconds
 *    - MQTT: Add "VOLUME_UP"/"VOLUME_DOWN"/"VOLUME_SET_xxx" commands
 *    - MQTT: Add publishing Music Player status to bttf/fc/mpstatus
 *    - MQTT: Don't queue and instead ignore (most) commands while fake-off or busy
 *    - Music Player: Use new sorting algorithm as the old one caused a crash on a 
 *      large number of already sorted files (due to excessive recursion).
 *  2026/04/27 (A10001986) [1.103]
 *    ********************************************************************************
 *    ** If updating from below 1.100, please see boxed note at version 1.101 below **
 *    ********************************************************************************
 *    - Add Car Mode: In addition to existing WiFi network connection settings, you 
 *      can now pre-configure TCD-AP SSID and password in WiFi Settings and quickly
 *      switch between normal (home, iPhone...) WiFi connection and Car WiFi connection 
 *      (through TCD-AP) by commands *990 (normal) and *991 (Car) or through the
 *      Config Portal.
 *    - Fix race condition in Fake Power logic
 *  2026/04/18 (A10001986) [1.102]
 *    ********************************************************************************
 *    ** If updating from below 1.100, please see boxed note at version 1.101 below **
 *    ********************************************************************************
 *    - Fix cutting off custom startup sound through wakeup() when fake-powered on
 *      through the Remote.
 *  2026/03/25 (A10001986) [1.101]
 *    ********************************************************************************
 *    ** If updating from below 1.100, please install 1.100  first to have your     **
 *    ** settings converted. If 1.100 is skipped, many of your settings (static IP, **
 *    ** volume, chase speed, flux sound mode/level, box light level, ir lock) will **
 *    ** be restored to default values. It suffices to install 1.100 and boot once; **
 *    ** you can then immediately  update to a later version.                       **
 *    ** https://github.com/realA10001986/Flux-Capacitor/releases/tag/V1.100        **
 *    ********************************************************************************
 *    - MQTT: Disable if server can't be resolved
 *    - WiFi: Allow defining a BSSID (AP MAC address) to connect to a specific AP
 *      if multiple APs with identical SSID are available.
 *    - WiFi: Do not power down AP long as a client is connected
 *    - Brush up Config Portal a bit
 *    - Code optimizations and fixes.
 *  2026/02/15 (A10001986) [1.100]
 *    - New file format for secondary and IP settings. This version of the firmware
 *      converts old to new.
 *    - "Shuffle" setting changes through *222/*555 are saved (SD required).
 *      (Shuffle option removed from CP)
 *    - Add option to show positive IR feedback (=successful commands) on display.
 *      Can be changed through Config Portal or *62 (every change is saved).
 *    - Add option to show command entry IR feedback on display. Can be changed 
 *      through Config Portal or *63 (every change is saved).
 *    - Most of event signals changed to make them consistent with SID, and more
 *      intuitive. Errors generally blink, (positive) confirmations are steady.
 *    - Display MAC address (STA) on WiFi Configuration Page
 *    - BTTFN: Avoid falling back to stand-alone mode too early when TCD stops
 *      sending packets.
 *    - Config files are now only written if actually changed which prolongs
 *      Flash life-span.
 *    - Code optimizations and fixes.
 *  [...]
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
    bttfn_loop();
}

void loop()
{    
    main_loop();
    audio_loop();
    wifi_loop();
    audio_loop();
    bttfn_loop();
}

#if defined(FC_DBG) || defined(FC_DBG_NET)
#warning "Debug output is enabled. Binary not suitable for release."
#endif
