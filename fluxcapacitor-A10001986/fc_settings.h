/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Flux Capacitor
 * (C) 2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Flux-Capacitor
 *
 * Settings handling
 *
 * -------------------------------------------------------------------
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

#ifndef _FC_SETTINGS_H
#define _FC_SETTINGS_H

extern bool haveSD;
extern bool FlashROMode;

extern uint8_t musFolderNum;

#define MS(s) XMS(s)
#define XMS(s) #s

// Default settings

#define DEF_PLAY_FLUX_SND   1     // 1: Play "flux" sound permantly, 0: Do not
#define DEF_PLAY_TT_SND     1     // 1: Play time travel sounds (0: Do not; for use with external equipment)
#define DEF_SS_TIMER        0     // "Screen saver" timeout in minutes; 0 = ss off
#define DEF_VKNOB           0     // 0: Don't use knob for audio volume, 1: do
#define DEF_SKNOB           0     // 0: Don't use knob for chase speed; 1: do
#define DEF_DISDIR          0     // 0: Do not disable default IR remote control; 1: do
#define DEF_TCD_PRES        0     // 0: No TCD connected, 1: connected via GPIO
#define DEF_HOSTNAME        "flux"
#define DEF_WIFI_RETRY      3     // 1-10; Default: 3 retries
#define DEF_WIFI_TIMEOUT    7     // 7-25; Default: 7 seconds
#define DEF_SHUFFLE         0     // Music Player: Do not shuffle by default
#define DEF_TCD_IP          ""    // TCD ip address for networked polling
#define DEF_WAIT_FOR_TCD    0     // 0: Boot normally  1: Delay WiFi setup for a few seconds (to wait for TCD if powered up simultaneously)
#define DEF_USE_GPSS        0     // 0: Ignore GPS speed; 1: Use it for chase speed
#define DEF_USE_NM          0     // 0: Ignore TCD night mode; 1: Follow TCD night mode
#define DEF_USE_FPO         0     // 0: Ignore TCD fake power; 1: Follow TCD fake power
#define DEF_WAIT_FPO        0     // 0: Don't wait for fake power on during boot, 1: Do
#define DEF_CFG_ON_SD       1     // Default: Save vol/spd/IR/mbl settings on SD card
#define DEF_SD_FREQ         0     // SD/SPI frequency: Default 16MHz
#define DEF_BLEDSWAP        0     // 0: Use box led connectors for box leds; 1: use "panel light" connector (both PWM!)

struct Settings {
    char playFLUXsnd[4]     = MS(DEF_PLAY_FLUX_SND);
    char playTTsnds[4]      = MS(DEF_PLAY_TT_SND);
    char ssTimer[6]         = MS(DEF_SS_TIMER);

    char usePLforBL[4]      = MS(DEF_BLEDSWAP);
    char useVknob[4]        = MS(DEF_VKNOB);
    char useSknob[4]        = MS(DEF_SKNOB);
    char disDIR[4]          = MS(DEF_DISDIR);

    char TCDpresent[4]      = MS(DEF_TCD_PRES);
    
    char hostName[32]       = DEF_HOSTNAME;
    char systemID[8]        = "";
    char appw[10]           = "";
    char wifiConRetries[4]  = MS(DEF_WIFI_RETRY);
    char wifiConTimeout[4]  = MS(DEF_WIFI_TIMEOUT);

    char tcdIP[16]          = DEF_TCD_IP;
    //char wait4TCD[4]        = MS(DEF_WAIT_FOR_TCD);
    char useGPSS[4]         = MS(DEF_USE_GPSS);
    char useNM[4]           = MS(DEF_USE_NM);
    char useFPO[4]          = MS(DEF_USE_FPO);
    char wait4FPOn[4]       = MS(DEF_WAIT_FPO);

    char shuffle[4]         = MS(DEF_SHUFFLE);
    char CfgOnSD[4]         = MS(DEF_CFG_ON_SD);
    char sdFreq[4]          = MS(DEF_SD_FREQ);

#ifdef FC_HAVEMQTT  
    char useMQTT[4]         = "0";
    char mqttServer[80]     = "";  // ip or domain [:port]  
    char mqttUser[128]      = "";  // user[:pass] (UTF8)
#endif    
};

struct IPSettings {
    char ip[20]       = "";
    char gateway[20]  = "";
    char netmask[20]  = "";
    char dns[20]      = "";
};

extern struct Settings settings;
extern struct IPSettings ipsettings;

void settings_setup();
void write_settings();
bool checkConfigExists();

bool loadCurVolume();
void saveCurVolume(bool useCache = true);

bool loadCurSpeed();
void saveCurSpeed(bool useCache = true);

bool loadBLLevel();
void saveBLLevel(bool useCache = true);

bool loadIRLock();
void saveIRLock(bool useCache = true);

bool loadMusFoldNum();
void saveMusFoldNum();

void copySettings();

bool saveIRKeys();

bool loadIpSettings();
void writeIpSettings();
void deleteIpSettings();

void doCopyAudioFiles();
bool copy_audio_files();

bool check_allow_CPA();
void delete_ID_file();

bool audio_files_present();

void formatFlashFS();

void rewriteSecondarySettings();

#endif
