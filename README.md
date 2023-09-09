# Firmware for Flux Capacitor

This repository holds the most current firmware for CircuitSetup's excellent [Flux Capacitor kit](https://circuitsetup.us).

The kit is available [here](https://circuitsetup.us).

![fMy Flux Capacitor](https://github.com/realA10001986/Flux-Capacitor/assets/76924199/ad197e19-fa48-463a-9231-7e05d6362329)

Features include
- movie-accurate default flux sequence, plus 9 alternative flux sequences
- Optional [flux](#the-flux-sound) sound (4 modes)
- [Time Travel](#time-travel) function, triggered by button, [Time Circuits Display](https://github.com/realA10001986/Time-Circuits-Display/) (TCD) or via [MQTT](#home-assistant--mqtt)
- [IR remote controlled](#ir-remote-control); can learn keys from custom remote
- [Music player](#the-music-player): Play mp3 files located on an SD card
- [SD card](#sd-card) support for custom audio files for effects, and music for the Music Player
- Advanced network-accessible [Config Portal](#the-config-portal) for setup with mDNS support for easy access (http://flux.local, hostname configurable)
- Wireless communication ("[BTTF-Network](#bttf-network-bttfn)") with [Time Circuits Display](https://github.com/realA10001986/Time-Circuits-Display/); used for synchonized time travels, alarm, chase speed, night mode, fake power
- [Home Assistant](#home-assistant--mqtt) (MQTT 3.1.1) support
- Built-in installer for default audio files in addition to OTA firmware updates

## Installation

There are different alternative ways to install this firmware:

1) If a previous version of the Flux Capacitor firmware was installed on your device, you can upload the provided pre-compiled binary to update to the current version: Enter the [Config Portal](#the-config-portal), click on "Update" and select the pre-compiled binary file provided in this repository ("install/fluxcapacitor-A10001986.ino.nodemcu-32s.bin").

2) Using the Arduino IDE or PlatformIO: Download the sketch source code, all required libraries, compile and upload it. This method is the one for fresh ESP32 boards and/or folks familiar with the programming tool chain. Detailed build information is in [fluxcapacitor-A10001986.ino](https://github.com/realA10001986/Flux-Capacitor/blob/main/fluxcapacitor-A10001986/fluxcapacitor-A10001986.ino).

 *Important: After a firmware update, the inner and outer LEDs might blink alternately for up to a minute after reboot. Do NOT unplug the device during this time.*

### Audio file installation

The firmware comes with a number of sound files which need to be installed separately. These sound files are not updated as often as the firmware itself. If you have previously installed the latest version of the sound-pack, you normally don't have to re-install the audio files when you update the firmware. Only if either a new version of the sound-pack is released, or your device is quiet after a firmware update, a re-installation is needed.

- Download "install/sound-pack-xxxxxxxx.zip" and extract it to the root directory of of a FAT32 formatted SD card
- power down the Flux Capacitor,
- insert this SD card into the device's slot and 
- power up the Flux Capacitor.
 
If (and only if) the **exact and complete contents of sound-pack archive** is found on the SD card, the device will install the audio files (automatically).

After installation, the SD card can be re-used for [other purposes](#sd-card).

## Short summary of first steps

A good first step would be to establish access to the Config Portal in order to configure your Flux Capacitor.

As long as the device is unconfigured, as is the case with a brand new Flux Capacitor, or later if it for some reason fails to connect to a configured WiFi network, it starts in "access point" mode, i.e. it creates a WiFi network of its own named "FC-AP". This is called "Access point mode", or "AP-mode".

- Power up the device and wait until the startup sequence has completed.
- Connect your computer or handheld device to the WiFi network "FC-AP".
- Navigate your browser to http://flux.local or http://192.168.4.1 to enter the Config Portal.
 
If you want your Flux Capacitor to connect to another access point, such as your WiFi network, click on "Configure WiFi". The bare minimum is to select an SSID (WiFi network name) and a WiFi password.

Note that the device requests an IP address via DHCP, unless you entered valid data in the fields for static IP addresses (IP, gateway, netmask, DNS). 

After saving the WiFi network settings, the device reboots and tries to connect to your configured WiFi network. If that fails, it will again start in access point mode. 

If the device is inaccessible as a result of incorrect static IPs, wait until the Flux Capacitor has completed its startup sequence, then type \*123456OK on the IR remote; static IP data will be deleted and the device will return to DHCP after a reboot.

If you have your FC, along with a Time Circuits Display, mounted in a car, see also [here](#car-setup).

### The Config Portal

The Config Portal is accessible exclusively through WiFi. As outlined above, if the device is not connected to a WiFi network, it creates its own WiFi network (named "FC-AP"), to which your WiFi-enabled hand held device or computer first needs to connect in order to access the Config Portal.

If the operating system on your handheld or computer supports Bonjour (a.k.a. "mDNS"), you can enter the Config Portal by directing your browser to http://flux.local . (mDNS is supported on Windows 10 version TH2 (1511) [other sources say 1703] and later, Android 13 and later, MacOS, iOS)

If that fails, the way to enter the Config Portal depends on whether the device is in access point mode or not. 
- If it is in access point mode (and your handheld/computer is connected to the WiFi network "FC-AP"), navigate your browser to http://192.168.4.1 
- Otherwise type *90 followed by OK on the remote control and listen, the IP address will be read out loud.

In the main menu, click on "Setup" to configure your Flux Capacitor.
| ![The Config Portal](https://github.com/realA10001986/Flux-Capacitor/assets/76924199/2c29632b-4d2b-4d67-97d5-a398198115d3) |
|:--:| 
| *The Config Portal's Setup page* |

A full reference of the Config Portal is [here](#appendix-a-the-config-portal).

## Basic Operation

Well. It fluxes, ie the LEDs run a chase sequence. By default, this sequence matches the one in the movie in order and speed.

There are various options to control the Flux Capacitor:

- Audio volume: Controlled by either remote control or a knob (if you chose to install one).
- Chase speed: Controlled by either remote control or a knob (if you chose to install one).

For the options to trigger a time travel, see [here](#time-travel).

The main control device, however, is the IR remote control.

### IR remote control

Your FC should have an IR remote control included. This remote works out-of-the-box and needs no setup. 

| ![Default IR remote control](https://github.com/realA10001986/Flux-Capacitor/assets/76924199/dc32787d-de83-4be4-9df8-7442bf623284) |
|:--:| 
| *The default IR remote control* |

This remote is also sold as part of a kit for Arduino and is reasonably priced.

Each time you press a (recognized) key on the remote, an IR feedback LED will briefly light up. This LED is located in the center of the board, next to the center LED.

### IR learning

You can have your FC learn the codes of another IR remote control. Most remotes with a carrier signal of 38kHz (which most IR remotes use) will work. However, some remote controls, expecially ones for TVs, send keys repeatedly and/or send different codes alternately. If you had the FC learn a remote and the keys are not (always) recognized afterwards, that remote is of that type and cannot be used.

First, go to the Config Portal, uncheck *TCD connected by wire* on the Setup page and save. The FC reboots. Afterwards, to start the learning process, hold the Time Travel button for a few seconds, until the chasing LEDs stop and [blink twice](#appendix-b-led-signals). Then press "0" on your remote, which the FC will [visually acknowledge](#appendix-b-led-signals). Then press "1", wait for the acknowledgement, and so on. Enter your keys in the following order:

```0 - 1 - 2 - 3 - 4 - 5 - 6 - 7 - 8 - 9 - * - # - Arrow up - Arrow down - Arrow left - Arrow right - OK``` 

If your remote control lacks the \* (starts command sequence) and \# (aborts command sequence) keys, you can use any other key, of course. \* could be eg. "menu" or "setup", \# could be "exit" or "return".

If no key is pressed for 10 seconds, the learning process aborts, as does briefly pressing the Time Travel button. In thoses cases, the keys already learned are forgotten and nothing is saved.

To make the FC forget a learned IR remote control, type *654321 followed by ENTER.

### Locking IR control

You can have your FC ignore IR commands from any IR remote control (be it the default supplied one, be it one you had the FC learn) by entering *70 followed by OK. After this sequence the FC will ignore all IR commands until *70OK is entered again. The purpose of this function is to enable you to use the same IR control for your FC and other props (such as SID).

Note that the status of the IR lock is saved 10 seconds after its last change, and persistent accross reboots.

In order to only disable the supplied IR remote control, check the option *Disable supplied IR remote control* in the [Config Portal](#-disable-supplied-ir-remote-control). In that case, any learned remote will still work.

### IR remote reference

<table>
    <tr>
     <td align="center" colspan="3">IR remote reference: Single key actions</td>
    </tr>
    <tr>
     <td align="center">1<br>-</td>
     <td align="center">2<br><a href="#the-music-player">Music Player</a>: Previous song</td>
     <td align="center">3<br><a href="#additional-custom-sounds">Play "key3.mp3"</a></td>
    </tr>
    <tr>
     <td align="center">4<br>-</td>
     <td align="center">5<br><a href="#the-music-player">Music Player</a>: Play/Stop</a></td>
     <td align="center">6<br><a href="#additional-custom-sounds">Play "key6.mp3"</a></td>
    </tr>
    <tr>
     <td align="center">7<br>-</td>
     <td align="center">8<br><a href="#the-music-player">Music Player</a>: Next song</td>
     <td align="center">9<br>-</td>
    </tr>
    <tr>
     <td align="center">*<br>Start command sequence</td>
     <td align="center">0<br><a href="#time-travel">Time Travel</a></td>
     <td align="center">#<br>Abort command sequence</td>
    </tr>
    <tr>
     <td align="center"></td>
     <td align="center">&#8593;<br>Increase audio volume</td>
     <td align="center"></td>
    </tr>
    <tr>
     <td align="center">&#8592;<br>Decrease chase speed</td>
     <td align="center">OK<br>Execute command</td>
     <td align="center">&#8594;<br>Increase chase speed</td>
    </tr>
    <tr>
     <td align="center"></td>
     <td align="center">&#8595;<br>Decrease audio volume</td>
     <td align="center"></td>
    </tr>
</table>

<table>
    <tr>
     <td align="center" colspan="2">IR remote reference: Special sequences<br>(&#9166; = OK key)</td>
    </tr>
  <tr>
     <td align="left">*0&#9166;</td>
     <td align="left">Select original chase sequence</td>
    </tr>
   <tr>
     <td align="left">*1&#9166; - *9&#9166;</td>
     <td align="left">Select chase sequences 1-9</td>
    </tr>
    <tr>
     <td align="left">*00&#9166;</td>
     <td align="left">Disable <a href="#the-flux-sound">flux sound</a></td>
    </tr>
   <tr>
     <td align="left">*01&#9166;</td>
     <td align="left">Enable <a href="#the-flux-sound">flux sound</a></td>
    </tr>
    <tr>
     <td align="left">*02&#9166;</td>
     <td align="left">Enable <a href="#the-flux-sound">flux sound</a>  (30 seconds)</td>
    </tr>
   <tr>
     <td align="left">*03&#9166;</td>
     <td align="left">Enable <a href="#the-flux-sound">flux sound</a>  (60 seconds)</td>
    </tr>
    <tr>
     <td align="left">*10&#9166; - *14&#9166;</td>
     <td align="left">Set minimum box light level</td>
    </tr>
     <tr>
     <td align="left">*20&#9166;</td>
     <td align="left">Reset chase speed to default</td>
    </tr>
    <tr>
     <td align="left">*50&#9166; - *59&#9166;</td>
     <td align="left"><a href="#the-music-player">Music Player</a>: Select music folder</td>
    </tr>
    <tr>
     <td align="left">*222&#9166;</td>
     <td align="left"><a href="#the-music-player">Music Player</a>: Shuffle off</td>
    </tr>
    <tr>
     <td align="left">*555&#9166;</td>
     <td align="left"><a href="#the-music-player">Music Player</a>: Shuffle on</td>
    </tr> 
    <tr>
     <td align="left">*888&#9166;</td>
     <td align="left"><a href="#the-music-player">Music Player</a>: Go to song 0</td>
    </tr>
    <tr>
     <td align="left">*888xxx&#9166;</td>
     <td align="left"><a href="#the-music-player">Music Player</a>: Go to song xxx</td>
    </tr>
    <tr>
     <td align="left">*70&#9166;</td>
     <td align="left"><a href="#locking-ir-control">Disable/Enable</a> IR remote commands</td>
    </tr>
    <tr>
     <td align="left">*80&#9166;</td>
     <td align="left">Toggle usage of volume knob</td>
    </tr>
   <tr>
     <td align="left">*81&#9166;</td>
     <td align="left">Toggle usage of speed knob</td>
    </tr>
     <tr>
     <td align="left">*90&#9166;</td>
     <td align="left">Say current IP address</td>
    </tr>
    <tr>
     <td align="left">*64738&#9166;</td>
     <td align="left">Reboot the device</td>
    </tr>
    <tr>
     <td align="left">*123456&#9166;</td>
     <td align="left">Delete static IP address and AP WiFI password</td>
    </tr>
   <tr>
     <td align="left">*654321&#9166;</td>
     <td align="left">Forget learned IR remote control</td>
    </tr>
</table>

[Here](https://github.com/realA10001986/Flux-Capacitor/blob/main/CheatSheet.pdf) is a cheat sheet for printing or screen-use. (Note that MacOS' preview application has a bug that scrambles the links in the document. Acrobat Reader does it correctly.)

## The Flux sound

By default, the FC plays a "flux" sound continously.

The flux sound can be permanently disabled, permanently enabled, or enabled for 30 or 60 seconds

- upon triggering a time travel,
- after switching on the FC.

The different modes are selected by typing *00 (disabled), *01 (enabled), *02 (enabled for 30 secs) or *03 (enabled for 60 secs), followed by OK. The power-up default is selected in the [Config Portal](#appendix-a-the-config-portal).

## Box lighting

The FC features connectors for box lights, ie LEDs that light up the inside of the FC during the time travel sequence. Those should be installed, they are essential part of the time travel sequence. The kit from CircuitSetup will probably contain suitable high-power LEDs for box lighting, and all four of those need to be connected to the "Box LED" connectors. 

As an alternative, one could use four pieces of 3W High-Power KEYES LED modules and drive them via the GPIO14 connector. As those draw quite much power, their power pins should therefore be connected to the power supply directly, and only the PWD input should be wired to the "IO14" pin of the "GPIO14" connector. If you use the GPIO14 connector for your box LEDs, check [this option](#-use-gpio14-for-box-lights).

In normal operation, those LEDs are off. You can, however, configure a minimum box light level to light up the box a little bit if you find it too dark. This level can be chosen out of five, by entering *10 through *14 followed by OK.

## Time travel

To travel through time, type "0" on the remote control. The flux capacitor will play its time travel sequence.

You can also connect a physical button to your FC; the button must shorten "GPIO" and "3.3V" on the "Time Travel" connector. Pressing this button briefly will trigger a time travel.

Other ways of triggering a time travel are available if a [Time Circuits Display is connected](#connecting-a-time-circuits-display).

## SD card

Preface note on SD cards: For unknown reasons, some SD cards simply do not work with this device. For instance, I had no luck with Sandisk Ultra 32GB and  "Intenso" cards. If your SD card is not recognized, check if it is formatted in FAT32 format (not exFAT!). Also, the size must not exceed 32GB (as larger cards cannot be formatted with FAT32). I am currently using Transcend SDHC 4GB cards and those work fine.

The SD card, apart from being used to [install](#audio-file-installation) the default audio files, can be used for substituting default sounds and for music played back by the [Music player](#the-music-player).

Note that the SD card must be inserted before powering up the device. It is not recognized if inserted while the Flux Capacitor is running. Furthermore, do not remove the SD card while the device is powered.

### Sound file substitution

The provided audio files ("sound-pack") are, after [proper installation](#audio-file-installation), integral part of the firmware and stored in the device's flash memory. 

These sounds can be substituted by your own sound files on a FAT32-formatted SD card. These files will be played back directly from the SD card during operation, so the SD card has to remain in the slot. The built-in [Audio file installer](#audio-file-installation) cannot be used to replace default sounds in the device's flash memory with custom sounds.

Your replacements need to be put in the root (top-most) directory of the SD card, be in mp3 format (128kbps max) and named as follows:
- "0.mp3" through "9.mp3", "dot.mp3": Numbers for IP address read-out.
- "flux.mp3". The standard flux sound, played continously.
- "alarm.mp3". Played when the alarm sounds (triggered by a Time Circuits Display via BTTFN or MQTT).

The following sounds are time-sync'd to display action. If you decide to substitute these with your own, be prepared to lose synchronicity:
- "travelstart.mp3". Played when a time travel starts.
- "timetravel.mp3". Played when re-entry of a time travel takes place.
- "startup.mp3". Played when the Flux Capacitor is connected to power and finished booting

### Additional Custom Sounds

The firmware supports some additional user-provided sound effects, which it will load from the SD card. If the respective file is present, it will be used. If that file is absent, no sound will be played.

- "key3.mp3" and/or "key6.mp3": Will be played if you press the "3"/"6" button on your remote

Those files are not provided here. You can use any mp3, with a bitrate of 128kpbs or less.

## The Music Player

The firmware contains a simple music player to play mp3 files located on the SD card. 

In order to be recognized, your mp3 files need to be organized in music folders named *music0* through *music9*. The folder number is 0 by default, ie the player starts searching for music in folder *music0*. This folder number can be changed using the remote control.

The names of the audio files must only consist of three-digit numbers, starting at 000.mp3, in consecutive order. No numbers should be left out. Each folder can hold up to 1000 files (000.mp3-999.mp3). *The maximum bitrate is 128kpbs.*

Since manually renaming mp3 files is somewhat cumbersome, the firmware can do this for you - provided you can live with the files being sorted in alphabetical order: Just copy your files with their original filenames to the music folder; upon boot or upon selecting a folder containing such files, they will be renamed following the 3-digit name scheme (as mentioned: in alphabetic order). You can also add files to a music folder later, they will be renamed properly; when you do so, delete the file "TCD_DONE.TXT" from the music folder on the SD card so that the firmware knows that something has changed. The renaming process can take a while (10 minutes for 1000 files in bad cases). Mac users are advised to delete the ._ files from the SD before putting it back into the FC as this speeds up the process.

To start and stop music playback, press 5 on your remote. Pressing 2 jumps to the previous song, pressing 8 to the next one.

By default, the songs are played in order, starting at 000.mp3, followed by 001.mp3 and so on. By entering \*555 and pressing OK, you can switch to shuffle mode, in which the songs are played in random order. Type \*222 followed by OK to switch back to consecutive mode.

Entering \*888 followed by OK re-starts the player at song 000, and \*888xxx (xxx = three-digit number) jumps to song #xxx.

See [here](#ir-remote-reference) for a list of controls of the music player.

While the music player is playing music, other sound effects are disabled/muted. Initiating a time travel stops the music player. The TCD-triggered alarm will sound as usual and stop the music player.

## Connecting a Time Circuits Display

### Connecting a TCD by wire

Connect GND and GPIO on the Flux Capacitor's "Time Travel" connector to the TCD like in the table below:

<table>
    <tr>
     <td align="center">Flux Capacitor:<br>"Time Travel" connector</td>
     <td align="center">TCD control board 1.2</td>
     <td align="center">TCD control board 1.3</td>
    </tr>
   <tr>
     <td align="center">GND</td>
     <td align="center">GND of "IO13" connector</td>
     <td align="center">GND on "Time Travel" connector</td>
    </tr>
    <tr>
     <td align="center">GPIO</td>
     <td align="center">IO13 of "IO13" connector</td>
     <td align="center">TT OUT on "Time Travel" connector</td>
    </tr>
</table>

Note that a wired connection only allows for synchronized time travel sequences, no other communication takes place.

### BTTF-Network ("BTTFN")

The TCD can communicate with the FC wirelessly, via WiFi. It can send out information about a time travel and an alarm, and the FC queries the TCD for speed and some other data. Unlike with MQTT, no broker or other third party software is needed.

![BTTFN connection](https://github.com/realA10001986/Flux-Capacitor/assets/76924199/93a9c471-d288-4a8f-87df-506ab8d5e619)

In order to connect your FC to the TCD using BTFFN, just enter the TCD's IP address in the *IP address of TCD* field in the FC's Config Portal. On the TCD, no special configuration is required.
  
Afterwards, the FC and the TCD can communicate wirelessly and 
- play time travel sequences in sync,
- both play an alarm-sequence when the TCD's alarm occurs,
- the FC queries the TCD for GPS speed if desired to adapt chase speed to GPS speed,
- the FC queries the TCD for fake power and night mode, in order to react accordingly if so configured.

You can use BTTF-Network and MQTT at the same time, see immediately below.

### Home Assistant/MQTT

The other way of wireless communication is, of course, [Home Assistant/MQTT](#home-assistant--mqtt).

If both TCD and FC are connected to the same broker, and the option *Send event notifications* is checked on the TCD's side, the FC will receive information on time travel and alarm and play their sequences in sync with the TCD. Unlike BTTFN, however, no other communication takes place.

![MQTT connection](https://github.com/realA10001986/Flux-Capacitor/assets/76924199/938c6bdd-f554-4e51-862e-9988e2339222)

MQTT and BTTFN can co-exist. However, the TCD only sends out time travel and alarm notifications through either MQTT or BTTFN, never both. If you have other MQTT-aware devices listening to the TCD's public topic (bttf/tcd/pub) in order to react to time travel or alarm messages, use MQTT (ie check *Send event notifications*). If only BTTFN-aware devices are to be used, uncheck this option to use BTTFN as it has less latency.

## Home Assistant / MQTT

The FC supports the MQTT protocol version 3.1.1 for the following features:

### Control the FC via MQTT

The FC can - to some extent - be controlled through messages sent to topic **bttf/fc/cmd**. Support commands are
- TIMETRAVEL: Start a [time travel](#time-travel)
- FLUX_OFF: Disables the [flux sound](#the-flux-sound)
- FLUX_ON: Enables the [flux sound](#the-flux-sound)
- FLUX_30: Enables the [flux sound](#the-flux-sound) for 30 seconds
- FLUX_60 Enables the [flux sound](#the-flux-sound) for 60 seconds
- MP_PLAY: Starts the [Music Player](#the-music-player)
- MP_STOP: Stops the [Music Player](#the-music-player)
- MP_NEXT: Jump to next song
- MP_PREV: Jump to previous song
- MP_SHUFFLE_ON: Enables shuffle mode in [Music Player](#the-music-player)
- MP_SHUFFLE_OFF: Disables shuffle mode in [Music Player](#the-music-player)

### Receive commands from Time Circuits Display

The TCD can trigger a time travel and tell the FC about an alarm by sending messages to topic **bttf/tcd/pub**. The FC receives these commands and reacts accordingly.

### Setup

In order to connect to a MQTT network, a "broker" (such as [mosquitto](https://mosquitto.org/), [EMQ X](https://www.emqx.io/), [Cassandana](https://github.com/mtsoleimani/cassandana), [RabbitMQ](https://www.rabbitmq.com/), [Ejjaberd](https://www.ejabberd.im/), [HiveMQ](https://www.hivemq.com/) to name a few) must be present in your network, and its address needs to be configured in the Config Portal. The broker can be specified either by domain or IP (IP preferred, spares us a DNS call). The default port is 1883. If a different port is to be used, append a ":" followed by the port number to the domain/IP, such as "192.168.1.5:1884". 

If your broker does not allow anonymous logins, a username and password can be specified.

Limitations: MQTT Protocol version 3.1.1; TLS/SSL not supported; ".local" domains (MDNS) not supported; server/broker must respond to PING (ICMP) echo requests. For proper operation with low latency, it is recommended that the broker is on your local network. 

## Car setup

If your FC, along with a [Time Circuits Display](https://github.com/realA10001986/Time-Circuits-Display/), is mounted in a car, the following network configuration is recommended:

#### TCD

- Run your TCD in [*car mode*](https://github.com/realA10001986/Time-Circuits-Display/blob/main/README.md#car-mode);
- disable WiFi power-saving on the TCD by setting *WiFi power save timer (AP-mode)* to 0 (zero).

#### Flux Capacitor

Enter the Config Portal on the FC (as described above), click on *Setup* and
  - enter *192.168.4.1* into the field *IP address of TCD*
  - check the option *Follow TCD fake power* if you have a fake power switch for the TCD (like eg a TFC switch)
  - click on *Save*.

After the FC has restarted, re-enter the FC's Config Portal (while the TCD is powered and in *car mode*) and
  - click on *Configure WiFi*,
  - select the TCD's access point name in the list at the top or enter *TCD-AP* into the *SSID* field; if you password-protected your TCD's AP, enter this password in the *password* field. Leave all other fields empty,
  - click on *Save*.

Using this setup enables the FC to receive notifications about time travel and alarm wirelessly, and to query the TCD for data.

In order to access the FC's Config Portal in your car, connect your hand held or computer to the TCD's WiFi access point ("TCD-AP"), and direct your browser to http://flux.local ; if that does not work, go to the TCD's keypad menu, press ENTER until "BTTFN CLIENTS" is shown, hold ENTER, and look for the FC's IP address there; then direct your browser to that IP by using the URL http://a.b.c.d (a-d being the IP address displayed on the TCD display).

## Flash Wear

Flash memory has a somewhat limited life-time. It can be written to only between 10.000 and 100.000 times before becoming unreliable. The firmware writes to the internal flash memory when saving settings and other data. Every time you change settings, data is written to flash memory.

In order to reduce the number of write operations and thereby prolong the life of your Flux Capacitor, it is recommended to use a good-quality SD card and to check *[Save volume/speed/IR settings on SD](#-save-volumespeedir-settings-on-sd)* in the Config Portal; alarm and speed settings as well as learned IR codes are then stored on the SD card (which also suffers from wear but is easy to replace). If you want to swap the SD card but preserve your volume/speed/IR settings, go to the Config Portal while the old SD card is still in place, uncheck the *Save volume/speed/IR settings on SD* option, click on Save and wait until the device has rebooted. You can then power down, swap the SD card and power-up again. Then go to the Config Portal, change the option back on and click on Save. Your settings are now on the new SD card.

## Appendix A: The Config Portal

### Main page

##### &#9654; Configure WiFi

Clicking this leads to the WiFi configuration page. On that page, you can connect your FC to your WiFi network by selecting/entering the SSID (WiFi network name) as well as a password (WPA2). By default, the FC requests an IP address via DHCP. However, you can also configure a static IP for the FC by entering the IP, netmask, gateway and DNS server. All four fields must be filled for a valid static IP configuration. If you want to stick to DHCP, leave those four fields empty.

Note that this page has nothing to do with Access Point mode; it is strictly for connecting your FC to an existing WiFi network as a client.

##### &#9654; Setup

This leads to the [Setup page](#setup-page).

##### &#9654; Restart

This reboots the Flux Capacitor. No confirmation dialog is displayed.

##### &#9654; Update

This leads to the firmware update page. You can select a locally stored firmware image file to upload (such as the ones published here in the install/ folder).

##### &#9654; Erase WiFi Config

Clicking this (and saying "yes" in the confirmation dialog) erases the WiFi configuration (WiFi network and password) and reboots the device; it will restart in "access point" mode. See [here](#short-summary-of-first-steps).

---

### Setup page

#### Basic settings

##### &#9654; Default flux sound mode

Selects the power-up "flux" sound mode. "Auto: xx secs" enables the beep for xx seconds after triggering a time travel, and upon power-on. Can be changed at any time by typing *00 (off), *01 (on), *02 (Auto 30secs) or *03 (Auto 60secs) followed by OK.

##### &#9654; Screen saver timer

Enter the number of minutes until the Screen Saver should become active when the FC is idle.

The Screen Saver, when active, stops the flux sound and disables all LEDs, until 
- a key on the IR remote control is pressed; if IR is [locked](#locking-ir-control), only the # key deactivates the Screen Saver;
- the time travel button is briefly pressed,
- a time travel event is triggered from a connected TCD (wire or wirelessly).

The music player will continue to run.
 
#### Hardware configuration settings

##### &#9654; Use 'GPIO14' for box lights

Normally, [box lights](#box-lighting) are connected to the "Box LED" connectors. Check this option if your box lights are instead connected to the "GPIO14" connector.

##### &#9654; Use volume knob by default

Check this if your FC has a pot for volume selection and you want to use this pot. Note that if this option is checked, commands regarding volume from the remote control are ignored.

##### &#9654; Use speed knob by default

Check this if your FC has a pot for chasing speed selection and you want to use this pot. Note that if this option is checked, commands regarding chasing speed from the remote control are ignored.

##### &#9654; Disable supplied IR remote control

Check this to disable the supplied remote control; the FC will only accept commands from a learned IR remote (if applicable). 

Note that this only disables the supplied remote, unlike [IR locking](#locking-ir-control), where IR commands from any known remote are ignored.

#### Network settings

##### &#9654; Hostname

The device's hostname in the WiFi network. Defaults to 'flux'. This also is the domain name at which the Config Portal is accessible from a browser in the same local network. The URL of the Config Portal then is http://<i>hostname</i>.local (the default is http://flux.local)

If you have more than one Flux Capacitors in your local network, please give them unique hostnames.

##### &#9654; AP Mode: Network name appendix

By default, if the FC creates a WiFi network of its own ("AP-mode"), this network is named "FC-AP". In case you have multiple FCs in your vicinity, you can have a string appended to create a unique network name. If you, for instance, enter "-ABC" here, the WiFi network name will be "FC-AP-ABC". Characters A-Z, a-z, 0-9 and - are allowed.

##### &#9654; AP Mode: WiFi password

By default, and if this field is empty, the FC's own WiFi network ("AP-mode") will be unprotected. If you want to protect your FC access point, enter your password here. It needs to be 8 characters in length and only characters A-Z, a-z, 0-9 and - are allowed.

If you forget this password and are thereby locked out of your FC, enter *123456 followed by OK on the IR remote control; this deletes the WiFi password. Then power-down and power-up your FC and the access point will start unprotected.

##### &#9654; WiFi connection attempts

Number of times the firmware tries to reconnect to a WiFi network, before falling back to AP-mode. See [here](#short-summary-of-first-steps)

##### &#9654; WiFi connection timeout

Number of seconds before a timeout occurs when connecting to a WiFi network. When a timeout happens, another attempt is made (see immediately above), and if all attempts fail, the device falls back to AP-mode. See [here](#short-summary-of-first-steps)

#### Settings for prop communication/synchronization

##### &#9654; TCD connected by wire

Check this if you have a Time Circuits Display connected by wire. You can only connect *either* a button *or* the TCD to the "time travel" connector on the FC, but not both.

Note that a wired connection only allows for synchronized time travel sequences, no other communication takes place.

Also note that the process of [learning keys from an IR remote control](#ir-remote-control) requires this option to be unchecked. After learning keys is done, you can, of course, check this option again.

##### &#9654; IP address of TCD

If you want to have your FC to communicate with a Time Circuits Display wirelessly ("BTTF-Network"), enter the IP address of the TCD here. Do NOT enter a host name here.

If you connect your FC to the TCD's access point ("TCD-AP"), the TCD's IP address is 192.168.4.1.

##### &#9654; Change chase speed with GPS speed

If your TCD is equipped with a GPS sensor, the FC can adapt its chase speed to current GPS speed. This option selects if GPS speed should be used for chase speed.

While the FC receives GPS speed from the TCD, IR controls for chase speed are not entirely ignored: They have no visual effect, but they are saved.

##### &#9654; Follow TCD night-mode

If this option is checked, and your TCD goes into night mode, the FC will activate the Screen Saver with a very short timeout, and reduce its audio volume.

##### &#9654; Follow TCD fake power

If this option is checked, and your TCD is equipped with a fake power switch, the FC will also fake-power up/down. If fake power is off, no LED is active and the FC will ignore all input from buttons, knobs and the IR control.

##### &#9654; Play time travel sounds

If other props are connected, they might bring their own time travel sound effects. In this case, you can uncheck this to disable the Flux Capacitor's own time travel sounds. Note that this only covers sounds played during time travel, not other sound effects.

#### Home Assistant / MQTT settings

##### &#9654; Use Home Assistant (MQTT 3.1.1)

If checked, the FC will connect to the broker (if configured) and send and receive messages via [MQTT](#home-assistant--mqtt)

##### &#9654; Broker IP[:port] or domain[:port]

The broker server address. Can be a domain (eg. "myhome.me") or an IP address (eg "192.168.1.5"). The default port is 1883. If different port is to be used, it can be specified after the domain/IP and a colon ":", for example: "192.168.1.5:1884". Specifiying the IP address is preferred over a domain since the DNS call adds to the network overhead. Note that ".local" (MDNS) domains are not supported.

##### &#9654; User[:Password]

The username (and optionally the password) to be used when connecting to the broker. Can be left empty if the broker accepts anonymous logins.

#### Music Player settings

##### &#9654; Shuffle at startup

When checked, songs are shuffled when the device is booted. When unchecked, songs will be played in order.

#### Other settings

##### &#9654; Save volume/speed/IR settings on SD

If this is checked, volume and speed settings, as well as learned IR codes are stored on the SD card. This helps to minimize write operations to the internal flash memory and to prolong the lifetime of your Flux Capacitor. See [Flash Wear](#flash-wear).

## Appendix B: LED signals

<table>
    <tr>
     <td align="left">&#9675; &#9679; &#9679; &#9679; &#9679; &#9679;<br>&#9679; &#9679; &#9679; &#9679; &#9679; &#9675;<br>&#8635;</td>
     <td align="left">Please wait, busy</td>
    </tr>
 <tr>
     <td align="left">&#9679; &#9679; &#9679; &#9679; &#9679; &#9675;<br>&#9679; &#9679; &#9679; &#9679; &#9679; &#9679;<br>&#9679; &#9679; &#9679; &#9679; &#9679; &#9675;<br>&#9679; &#9679; &#9679; &#9679; &#9679; &#9679;</td>
     <td align="left">Error: Audio files <a href="#audio-file-installation">not installed</a></td>
    </tr>
<tr>
     <td align="left">&#9675; &#9675; &#9679; &#9679; &#9679; &#9679;<br>&#9679; &#9679; &#9679; &#9679; &#9675; &#9675;<br>&#8635;</td>
     <td align="left">Error: Audio file copy error</td>
    </tr>
 <tr>
     <td align="left">&#9675; &#9679; &#9679; &#9679; &#9679; &#9679;<br>&#9679; &#9679; &#9679; &#9679; &#9679; &#9679;<br>&#9675; &#9679; &#9679; &#9679; &#9679; &#9679;<br>&#9679; &#9679; &#9679; &#9679; &#9679; &#9679;</td>
     <td align="left">Error: Unknown/illegal input from remote control</td>
    </tr>
 <tr>
     <td align="left">&#9675; &#9675; &#9675; &#9679; &#9679; &#9679;<br>&#9679; &#9679; &#9679; &#9675; &#9675; &#9675;<br>&#8635;</td>
     <td align="left"><a href="#receive-commands-from-time-circuits-display">Alarm</a> (from TCD via BTTFN/MQTT)</td>
    </tr>
<tr>
     <td align="left">&#9675; &#9675; &#9675; &#9675; &#9675; &#9675; (1000ms)<br>&#9679; &#9679; &#9679; &#9679; &#9679; &#9679; (1000ms)<br>&#9675; &#9675; &#9675; &#9675; &#9675; &#9675; (1000ms)<br>&#9679; &#9679; &#9679; &#9679; &#9679; &#9679; (1000ms)</td>
     <td align="left"><a href="#ir-remote-control">IR Learning</a>: Start</td>
    </tr>
<tr>
     <td align="left">&#9679; &#9679; &#9675; &#9675; &#9679; &#9679;<br>&#9679; &#9679; &#9679; &#9679; &#9679; &#9679;<br>&#9679; &#9679; &#9675; &#9675; &#9679; &#9679;<br>&#9679; &#9679; &#9679; &#9679; &#9679; &#9679;</td>
     <td align="left"><a href="#ir-remote-control">IR Learning</a>: Next</td>
    </tr>
<tr>
     <td align="left">&#9675; &#9675; &#9675; &#9675; &#9675; &#9675; (500ms)<br>&#9679; &#9679; &#9679; &#9679; &#9679; &#9679; (500ms)<br>&#9675; &#9675; &#9675; &#9675; &#9675; &#9675; (500ms)<br>&#9679; &#9679; &#9679; &#9679; &#9679; &#9679; (500ms)</td>
     <td align="left"><a href="#ir-remote-control">IR Learning</a>: Done</td>
    </tr>
</table>
