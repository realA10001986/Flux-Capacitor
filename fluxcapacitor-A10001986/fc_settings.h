/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Flux Capacitor
 * (C) 2023-2025 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Flux-Capacitor
 * https://fc.out-a-ti.me
 *
 * Settings handling
 *
 * -------------------------------------------------------------------
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

#ifndef _FC_SETTINGS_H
#define _FC_SETTINGS_H

extern bool haveFS;
extern bool haveSD;
extern bool FlashROMode;

extern bool haveAudioFiles;

extern uint8_t musFolderNum;

#define MS(s) XMS(s)
#define XMS(s) #s

// Default settings

#define DEF_PLAY_FLUX_SND   0     // 1: Play "flux" sound permanently, 0: Do not, 2, 3: 30/60 secs after event
#define DEF_ORIG_SEQ        1     // 0: movie sequence for 6 lamps; 1: sequence uses 7th (non-existent) light
#define DEF_STTBL_ANIM      0     // 1: Skip box light animation in tt; 0: Play anim
#define DEF_PLAY_TT_SND     1     // 1: Play time travel sounds (0: Do not; for use with external equipment)
#define DEF_PLAY_ALM_SND    0     // 1: Play TCD-alarm sound, 0: do not
#define DEF_SS_TIMER        0     // "Screen saver" timeout in minutes; 0 = ss off

#define DEF_SHUFFLE         0     // Music Player: Do not shuffle by default

#define DEF_HOSTNAME        "flux"
#define DEF_WIFI_RETRY      3     // 1-10; Default: 3 retries
#define DEF_WIFI_TIMEOUT    7     // 7-25; Default: 7 seconds

#define DEF_TCD_IP          ""    // TCD ip address for networked polling
#define DEF_USE_GPSS        0     // 0: Ignore GPS speed; 1: Use it for chase speed
#define DEF_USE_NM          0     // 0: Ignore TCD night mode; 1: Follow TCD night mode
#define DEF_USE_FPO         0     // 0: Ignore TCD fake power; 1: Follow TCD fake power
#define DEF_BTTFN_TT        1     // 0: '0' on IR remote and TT button trigger stand-alone TT; 1: They trigger BTTFN-wide TT

#define DEF_TCD_PRES        0     // 0: No TCD connected, 1: connected via GPIO
#define DEF_NO_ETTO_LEAD    0     // Default: 0: TCD signals TT with ETTO_LEAD lead time; 1 without

#define DEF_CFG_ON_SD       1     // Default: Save secondary settings on SD card
#define DEF_SD_FREQ         0     // SD/SPI frequency: Default 16MHz

#define DEF_BLEDSWAP        0     // 0: Use box led connectors for box leds; 1: use "panel light" connector (both PWM!)
#define DEF_SKNOB           0     // 0: Don't use knob for chase speed; 1: do
#define DEF_DISDIR          0     // 0: Do not disable default IR remote control; 1: do

struct Settings {
    char playFLUXsnd[4]     = MS(DEF_PLAY_FLUX_SND);
    char origSeq[4]         = MS(DEF_ORIG_SEQ);
    char skipTTBLAnim[4]    = MS(DEF_STTBL_ANIM); 
    char playTTsnds[4]      = MS(DEF_PLAY_TT_SND);
    char playALsnd[4]       = MS(DEF_PLAY_ALM_SND);
    char ssTimer[6]         = MS(DEF_SS_TIMER);

    char shuffle[4]         = MS(DEF_SHUFFLE);
    
    char hostName[32]       = DEF_HOSTNAME;
    char systemID[8]        = "";
    char appw[10]           = "";
    char wifiConRetries[4]  = MS(DEF_WIFI_RETRY);
    char wifiConTimeout[4]  = MS(DEF_WIFI_TIMEOUT);
    
    char tcdIP[32]          = DEF_TCD_IP;
    char useGPSS[4]         = MS(DEF_USE_GPSS);
    char useNM[4]           = MS(DEF_USE_NM);
    char useFPO[4]          = MS(DEF_USE_FPO);
    char bttfnTT[4]         = MS(DEF_BTTFN_TT);

#ifdef FC_HAVEMQTT  
    char useMQTT[4]         = "0";
    char mqttServer[80]     = "";  // ip or domain [:port]  
    char mqttUser[128]      = "";  // user[:pass] (UTF8)
#endif     

    char TCDpresent[4]      = MS(DEF_TCD_PRES);
    char noETTOLead[4]      = MS(DEF_NO_ETTO_LEAD);

    char CfgOnSD[4]         = MS(DEF_CFG_ON_SD);
    char sdFreq[4]          = MS(DEF_SD_FREQ);

    char usePLforBL[4]      = MS(DEF_BLEDSWAP);
    char useSknob[4]        = MS(DEF_SKNOB);
    char disDIR[4]          = MS(DEF_DISDIR);
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

void unmount_fs();

void write_settings();
bool checkConfigExists();

bool saveIRKeys();
void deleteIRKeys();

bool loadCurVolume();
void saveCurVolume(bool useCache = true);

bool loadCurSpeed();
void saveCurSpeed(bool useCache = true);

bool loadBLLevel();
void saveBLLevel(bool useCache = true);

bool loadIdlePat();
void saveIdlePat(bool useCache = true);

bool loadIRLock();
void saveIRLock(bool useCache = true);

bool loadMusFoldNum();
void saveMusFoldNum();

bool loadIpSettings();
void writeIpSettings();
void deleteIpSettings();

void copySettings();

bool check_if_default_audio_present();
bool prepareCopyAudioFiles();
void doCopyAudioFiles();

bool check_allow_CPA();
void delete_ID_file();

#define MAX_SIM_UPLOADS 16
#define UPL_OPENERR 1
#define UPL_NOSDERR 2
#define UPL_WRERR   3
#define UPL_BADERR  4
#define UPL_MEMERR  5
#define UPL_UNKNOWN 6
#define UPL_DPLBIN  7
#include <FS.h>
bool   openUploadFile(String& fn, File& file, int idx, bool haveAC, int& opType, int& errNo);
size_t writeACFile(File& file, uint8_t *buf, size_t len);
void   closeACFile(File& file);
void   removeACFile(int idx);
void   renameUploadFile(int idx);
char   *getUploadFileName(int idx);
int    getUploadFileNameLen(int idx);
void   freeUploadFileNames();

#endif
