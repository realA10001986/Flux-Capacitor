/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Flux Capacitor
 * (C) 2023-2025 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Flux-Capacitor
 * https://fc.out-a-ti.me
 *
 * License: MIT NON-AI
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
 *   For the SLAB CP210x (which is used by NodeMCU-boards distributed by CircuitSetup)
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
 *   - Go to Config Portal, click "Update" and upload the sound-pack (FCA.bin, extracted
 *     from install/sound-pack-xxxxxxxx.zip) through the bottom file selector.
 *     An SD card must be present in the slot during this operation.
 *   Method 2:
 *   - Copy FCA.bin to the top folder of a FAT32 (not ExFAT!) formatted SD card (max 
 *     32GB) and put this card into the slot while the FC is powered down. 
 *   - Now power-up. The sound-pack will now be installed. When finished, the FC will 
 *     reboot.
 */

/*  Changelog
 *
 *  2025/10/31 (A10001986) [1.86]
 *    - Play sound on volume change (if nothing is played currently)
 *    - New sound pack (FC04)
 *  2025/10/30 (A10001986)
 *    - Fix deleting a bad .bin file after upload
 *    - Save flux sound mode when changed (10 seconds after change in fact) so
 *      it's restored on power-up.
 *  2025/10/29 (A10001986)
 *    - Execute *77 only if coming from IR control .
 *  2025/10/26 (A10001986) [1.85.2]
 *    - BTTFN: Fix hostname length issues; code optimizations; minor fix for mc 
 *      notifications. Breaks support for TCD firmwares < 3.2.
 *      Recommend to update all props' firmwares for similar fixes.
 *  2025/10/24 (A10001986)
 *    - Add WiFi power saving for AP-mode, and user-triggered WiFi connect retry. 
 *      Command sequence *77OK is to 1) restart WiFi after entering PS mode, 2)
 *      trigger a connection attempt if configured WiFi could not be connected
 *      to during boot.
 *    - WM: Fix AP shutdown; handle mDNS
 *  2025/10/21 (A10001986) [1.85.1]
 *    - Simplify remoteAllow logic
 *    - Wakeup on GPS speed changes from <= 0 to >= 0
 *    - Tweak chase speed ramp for time travel; increase speed adaption freq
 *  2025/10/17 (A10001986) [1.85]
 *    - New sound pack; new startup-sequence, and new re-entry sequence.
 *      Re-done due to flux sound level being adjustable now.
 *    - Chase speed, when determined by the TCD through GPS, rotary encoder or
 *      through Futaba Remote, or while accelerating/decelerating due to a 
 *      time travel, is now based on the idle speed set by the IR control or the
 *      speed knob. Previously, the speed-to-chase-speed conversion was based
 *      on a fixed idle speed. This means that when you select a faster idle
 *      speed (eg using the IR remote), this very chase speed, instead of a
 *      fixed idle speed, will now correspond to 0mph.
 *    - Wipe flash FS if alien VER found; in case no VER is present, check
 *      available space for audio files, and wipe if not enough.
 *  2025/10/16 (A10001986)
 *    - Fix saving of relative Flux volume level
 *    - MQTT: Add two user signals, to be triggered by sending commands "USER1"
 *      or "USER2" to bttf/fc/cmd topic. These signals are accompanied by sound
 *      if the SD card contains "user1.mp3" or "user2.mp3". No default sound.
 *    - Minor code optim (settings)
 *    - WM: More event-based waiting instead of delays
 *  2025/10/15 (A10001986)  
 *    - Some more WM changes. Number of scanned networks listed is now restricted 
 *      in order not to run out of memory.
 *  2025/10/14 (A10001986) [1.84.2]
 *    - WM: Do not garble UTF8 SSID; skip SSIDs with non-printable characters
 *    - Fix regression in CP ("show password")
 *  2025/10/13 (A10001986)
 *    - Config Portal: Minor restyling (message boxes)
 *  2025/10/11 (A10001986) [1.84.1]
 *    - More WM changes: Simplify "Forget" using a checkbox; redo signal quality
 *      assessment; remove over-engineered WM debug stuff.
 *  2025/10/08 (A10001986) [1.84]
 *    - Add control of relative flux volume level; *30-*33. Defaults to
 *      *33 which is the level used before. Saved together with volume.
 *    - "Box light level" control code moved to *400-*404
 *    - "Reset to default speed" control code moved to *80
 *    - WM: Set "world safe" country info, limiting choices to 11 channels
 *    - WM: Add "show all", add channel info (when all are shown) and
 *      proposed AP WiFi channel on WiFi Configuration page.
 *    - Experimental: Change bttfn_checkmc() to return true as long as 
 *      a packet was received (as opposed to false if a packet was received
 *      but not for us, malformed, etc). Also, change the max packet counter
 *      in bttfn_loop(_quick)() from 10 to 100 to get more piled-up old 
 *      packets out of the way.
 *    - Add a delay when connecting to TCD-AP so not all props hammer the
 *      TCD-AP at the very same time
 *    - WM: Use events when connecting, instead of delays
 *  2025/10/07 (A10001986)
 *    - Add emergency firmware update via SD (for dev purposes)
 *    - WM fixes (Upload, etc)
 *  2025/10/06 (A10001986)
 *    - WM: Skip setting static IP params in Save
 *    - Add "No SD present" banner in Config Portal if no SD present
 *  2025/10/05 (A10001986)
 *    - CP: Show msg instead of upload file input if no sd card is present
 *  2025/10/03-05 (A10001986) [1.83.1]
 *    - Let DNS server in AP mode only resolve our domain (hostname)
 *  2025/10/03-05 (A10001986) [1.83]
 *    - More WiFiManager changes. We no longer use NVS-stored WiFi configs, 
 *      all is managed by our own settings. (No details are known, but it
 *      appears as if the core saves some data to NVS on every reboot, this
 *      is totally not needed for our purposes, nor in the interest of 
 *      flash longevity.)
 *    - Save static IP only if changed
 *    - Disable MQTT when connected to "TCD-AP"
 *  2025/09/22-10/03 (A10001986)
 *    - WiFi Manager overhaul; many changes to Config Portal.
 *      WiFi-related settings moved to WiFi Configuration page.
 *      Note: If the FC is in AP-mode, mp3 playback will be stopped when 
 *      accessing Config Portal web pages from now on.
 *      This had lead to sound stutter and incomplete page loads in the past.
 *    - Various code optimizations to minimize code size and used RAM
 *  2025/09/22 (A10001986) [1.82]
 *    - Config Portal: Re-order settings; remove non-checkbox-code.
 *    - Fix TCD hostname length field
 *  2025/09/20 (A10001986)
 *    - Config Portal: Add "install sound pack" banner to main menu
 *  2025/09/19 (A10001986) [1.81]
 *    - Extend mp3 upload by allowing multiple (max 16) mp3 files to be uploaded
 *      at once. The FCA.bin file can be uploaded at the same time as well.
 *  2025/09/17 (A10001986)
 *    - WiFi Manager: Reduce page size by removing "quality icon" styles where
 *      not needed.
 *  2025/09/15 (A10001986) [1.80]
 *    - Refine mp3 upload facility; allow deleting files from SD by prefixing
 *      filename with "delete-".
 *    - WiFi manager: Remove lots of <br> tags; makes Safari display the
 *      pages better.
 *  2025/09/14 (A10001986)
 *    - Allow uploading .mp3 files to SD through config portal. Uses the same
 *      interface as audio container upload. Files are stored in the root
 *      folder of the SD; hence not suitable for music player.
 *    - WiFi manager: Remove (ie skip compilation of) unused code
 *    - WiFi manager: Add callback to Erase WiFi settings, before reboot
 *    - WiFi manager: Build param page with fixed size string to avoid memory 
 *      fragmentation; add functions to calculate String size beforehand.
 *  2025/08/31 (A10001986) [1.73]
 *    - Add option to choose between 6- and 7-lights movie sequence. This allows
 *      using the movie sequence as of before 1.71.2.
 *  2025/08/20 (A10001986) [1.72]
 *    - Add "987654" IR command; triggers IR learning (and spares the user from
 *      installing a time travel button as required for IR learning before)
 *    - Minor fixes
 *  2025/08/16 (A10001986) [1.71.3]
 *    - Properly loop flux sound (new sound pack)
 *  2025/05/09 (A10001986) [1.71.2]
 *    - Change default sequence to match original circuit board (which allegedly was
 *      designed for seven lamps, of which only six ended up in the movies)
 *  2025/02/13 (A10001986) [1.71.1]
 *    - Delete temp file after audio installation
 *  2025/01/15 (A10001986) [1.71]
 *    - Optimize play_key; keyX will be stopped instead of (re)started if it is 
 *      currently played when repeatedly triggered.
 *  2025/01/12-14 (A10001986) [1.70]
 *    - Add support for remote controlling the TCD keypad through the IR remote
 *      control. *95OK starts, "#" quits remote controlling. All keys until # 
 *      are directly sent to the TCD. Holding a key on the TCD keypad is emulated 
 *      by typing * followed by the key to be "held".
 *    - BTTFN: Minor code optimizations
 *    - Flux sound now off by default
 *    - Better feedback on IR/remote command execution. Success is now mostly
 *      signalled by 1 sec of IR feedback, failure either by the usual signal
 *      (chase LEDs) or 2 blinks of the IR feedback LED.
 *    - IR input buffer is no longer reset when receiving commands from TCD
 *  2024/10/27 (A10001986)
 *    - Minor changes (bttfn_loop in delay-loops; etc)
 *  2024/10/26 (A10001986) [1.60]
 *    - Add support for TCD multicast notifications: This brings more immediate speed 
 *      updates (no more polling; TCD sends out speed info when appropriate), and 
 *      less network traffic in time travel sequences.
 *      The TCD supports this since Oct 26, 2024.
 *  2024/10/08 (A10001986)
 *    - MQTT: Add FASTER, SLOWER, RESETSPEED and CHASE_x commands. CHASE_x sets the
 *      chase pattern (x=0-9)
 *  2024/09/11 (A10001986)
 *    - Fix C99-compliance
 *  2024/09/09 (A10001986)
 *    - Tune BTTFN poll interval
 *  2024/08/28 (A10001986)
 *    - Treat TCD-speed from Remote like speed from RotEnc
 *  2024/08/11 (A10001986)
 *    - In addition to the custom "key3" and "key6" sounds, now also "key1", 
 *      "key4", "key7" and "key9" are available/supported; these are triggered 
 *      by previously unused keys on the IR remote (1, 4, 7, 9) and command 
 *      codes 3001, 3004, 3007, 3009.
 *  2024/07/12 (A10001986)
 *    - MQTT: Add MP_FOLDER_x commands to set music folder number
 *    - Remove old, duplicate command codes
 *  2024/07/11 (A10001986)
 *    - Fix typo in filename sorting
 *  2024/06/30 (A10001986)
 *    - Fix box-light level commands
 *  2024/06/05 (A10001986)
 *    - Minor fixes for WiFiManager
 *    * Switched to esp32-arduino 2.0.17 for pre-compiled binary.
 *  2024/05/09 (A10001986)
 *    - Enable internal pull-down for time travel connector
 *  2024/04/10 (A10001986)
 *    - Minor CP optimizations
 *  2024/04/03 (A10001986)
 *    - Rewrite settings upon clearing AP-PW only if AP-PW was actually set.
 *    - Re-phrase "shuffle" Config option
 *  2024/03/26 (A10001986)
 *    - BTTFN device type update
 *  2024/02/26 (A10001986)
 *    - Note: IRAM flags for interrupts disturb firmware update, do not set.
 *  2024/02/08 (A10001986)
 *    - CP: Add header to "Saved" page so one can return to main menu
 *  2024/02/06 (A10001986)
 *    - Fix reading and parsing of JSON document
 *    - Fixes for using ArduinoJSON 7; not used in bin yet, too immature IMHO.
 *  2024/02/05 (A10001986)
 *    - Tweaks for audio upload
 *  2024/02/04 (A10001986)
 *    - Include fork of WiFiManager (2.0.16rc2 with minor patches) in order to cut 
 *      down bin size
 *  2024/02/03 (A10001986)
 *    - Audio data (FCA.bin) can now be uploaded through Config Portal ("UPDATE" page). 
 *      Requires an SD card present.
 *    - Check for audio data present also in FlashROMode; better container validity
 *      check; display hint in CP if current audio not installed
 *  2024/01/30 (A10001986)
 *    - Minor optimizations
 *  2024/01/27 (A10001986) [1.40]
 *    - Make volume control work like on TCD and DG; *300-*319 sets level, *399 enables
 *      the volume knob (3300-3319/3399 from TCD keypad). 
 *      Remove "Use volume knob by default" option from Config Portal, and *80 IR command.
 *  2024/01/26 (A10001986)
 *    - Reformat FlashFS only if audio file installation fails due to a write error
 *    - Add sound-pack versioning; re-installation required with this FW update
 *    - Allow sound installation in Flash-RO-mode
 *  2024/01/22 (A10001986)
 *    - Fix for BTTFN-wide TT vs. TCD connected by wire
 *  2024/01/20 (A10001986)
 *    - Major cleanup, minor fixes
 *  2024/01/18 (A10001986)
 *    - Fix wifi menu size
 *  2024/01/15 (A10001986)
 *    - Flush outstanding delayed saves before rebooting and on fake-power-off
 *    - Remove "Restart" menu item from CP, can't let WifiManager reboot behind
 *      our back.
 *  2023/12/09 (A10001986)
 *    - Limit max chase speed set by IR remote to 3
 *  2023/12/08 (A10001986)
 *    - Add option to trigger a BTTFN-wide TT when pressing 0 on the IR remote
 *      or pressing the TT button (instead of a stand-alone TT).
 *  2023/12/03 (A10001986)
 *    - Yet another sound update
 *  2023/12/01 (A10001986)
 *    - Fix end of special signal when switched off mid-sequence while FC chase
 *      is off
 *  2023/11/30 (A10001986)
 *    - Some better sounds
 *  2023/11/22 (A10001986)
 *    - Wake up only if "speed" is from RotEnc, not when from GPS
 *  2023/11/20 (A10001986)
 *    - More enhancements to speed changes
 *    - Fix: Wake up only if fake power is on
 *    - Suppress IR while center led is on as it apparently causes erroneous 
 *      pseudo-transmissions.
 *  2023/11/19 (A10001986)
 *    - Wake up on GPS/RotEnc speed change
 *    - Soften/smoothen GPS/RotEnc speed/Flux Chase speed changes
 *  2023/11/06 (A10001986)
 *    - Abort audio file installer on first error
 *  2023/11/05 (A10001986)
 *    - Settings: (De)serialize JSON from/to buffer instead of file
 *    - Fix corrupted Shuffle setting
 *  2023/11/04 (A10001986)
 *    - Unmount filesystems before reboot
 *  2023/11/02 (A10001986)
 *    - Start CP earlier to reduce startup delay caused by that darn WiFi scan upon
 *      CP start.
 *    * WiFiManager: Disable pre-scanning of WiFi networks when starting the CP.
 *      Scanning is now only done when accessing the "Configure WiFi" page.
 *      To do that in your own installation, set _preloadwifiscan to false
 *      in WiFiManager.h
 *  2023/11/01 (A10001986)
 *    - Stop audio earlier before rebooting when saving CP settings
 *  2023/10/31 (A10001986)
 *    - BTTFN: User can now enter TCD's hostname instead of IP address. If hostname
 *      is given, TCD must be on same local network. Uses multicast, not DNS.
 *  2023/10/29 (A10001986)
 *    - Remove unneeded stuff
 *  2023/10/05 (A10001986)
 *    - Add support for "wakeup" command (BTTFN/MQTT)
 *  2023/09/30 (A10001986)
 *    - Extend remote commands to 32 bit
 *    - Fix ring buffer handling for remote commands
 *  2023/09/26 (A10001986)
 *    - Make TCD-alarm sound optional
 *    - Clean up options
 *    - Fix TT button scan during TT
 *    - SS only starts flux sound if to be played permanently
 *    - Mute sound and clear display when Config Portal settings are saved
 *  2023/09/25 (A10001986)
 *    - Add option to handle TT triggers from wired TCD without 5s lead. Respective
 *      options on TCD and external props must be set identically.
 *  2023/09/23 (A10001986)
 *    - Add remote control facility through TCD keypad (requires BTTFN connection 
 *      with TCD). Commands for FC are 3000-3999.
 *    - Change some IR command sequences
 *  2023/09/19 (A10001986)
 *    - Fix audio file installer
 *  2023/09/16 (A10001986)
 *    - Simplify SPIFFS/LittleFS code paths in audio
 *  2023/09/13 (A10001986)
 *    - Add option to skip box light animation during tt
 *  2023/09/11 (A10001986)
 *    - Guard SPIFFS/LittleFS calls with FS check
 *  2023/09/10 (A10001986)
 *    - If specific config file not found on SD, read from FlashFS - but only
 *      if it is mounted.
 *  2023/09/09 (A10001986)
 *    - Switch to LittleFS by default
 *    - *654321OK lets FC forget learned IR remote control
 *    - Remove "Wait for TCD fake power on" option; is now implied
 *    - Save current idle pattern to SD for persistence
 *      (only if changed via IR, not MQTT)
 *    - If SD mount fails at 16Mhz, retry at 25Mhz
 *    - If specific config file not found on SD, read from FlashFS
 *  2023/09/07 (A10001986)
 *    - Fix links
 *  2023/09/02 (A10001986)
 *    - Handle dynamic ETTO LEAD for BTTFN-triggered time travels
 *    - Go back to stand-alone mode if BTTFN polling times-out
 *  2023/08/31 (A10001986)
 *    - WiFi connect retry: When no network config'd, set retry to 1
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
