/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Flux Capacitor
 * (C) 2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Flux-Capacitor
 *
 * Settings & file handling
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

#include "fc_global.h"

#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson
#include <SD.h>
#include <SPI.h>
#include <FS.h>
#ifdef USE_SPIFFS
#include <SPIFFS.h>
#else
#define SPIFFS LittleFS
#include <LittleFS.h>
#endif

#include "fc_settings.h"
#include "fc_audio.h"
#include "fc_main.h"

// Size of main config JSON
// Needs to be adapted when config grows
#define JSON_SIZE 1600

/* If SPIFFS/LittleFS is mounted */
static bool haveFS = false;

/* If a SD card is found */
bool haveSD = false;

/* If SD contains default audio files */
static bool allowCPA = false;

/* Music Folder Number */
uint8_t musFolderNum = 0;

/* Save alarm/volume on SD? */
static bool configOnSD = false;

/* Cache for volume */
static uint8_t prevSavedVol = 255;

/* Cache for speed */
static uint16_t prevSavedSpd = 999;

/* Cache for mbll */
static uint16_t prevSavedBLL = 0;

/* Cache for IR lock */
static bool prevSavedIRL = 0;

/* Paranoia: No writes Flash-FS  */
bool FlashROMode = false;

#define NUM_AUDIOFILES 11+8
#define SND_KEY_IDX  11+3
static const char *audioFiles[NUM_AUDIOFILES] = {
      "/0.mp3\0", "/1.mp3\0", "/2.mp3\0", "/3.mp3\0", "/4.mp3\0", 
      "/5.mp3\0", "/6.mp3\0", "/7.mp3\0", "/8.mp3\0", "/9.mp3\0", 
      "/dot.mp3\0", 
      "/flux.mp3\0",
      "/startup.mp3\0",
      "/timetravel.mp3\0",
      "/travelstart.mp3\0",
      "/alarm.mp3\0",
      "/fluxing.mp3\0",
      "/renaming.mp3\0",
      "/installing.mp3\0"
};
static const char *IDFN = "/FC_def_snd.txt";

static const char *cfgName    = "/fcconfig.json";   // Main config (flash)
static const char *volCfgName = "/fcvolcfg.json";   // Volume config (flash/SD)
static const char *spdCfgName = "/fcspdcfg.json";   // Speed config (flash/SD)
static const char *bllCfgName = "/fcbllcfg.json";   // Minimum box light level config (flash/SD)
static const char *musCfgName = "/fcmcfg.json";     // Music config (SD)
static const char *ipCfgName  = "/fcipcfg.json";    // IP config (flash)
static const char *irUCfgName = "/fcirkeys.txt";    // IR keys (user-created) (SD)
static const char *irCfgName  = "/fcirkeys.json";   // IR keys (system-created) (flash/SD)
static const char *irlCfgName = "/fcirlcfg.json";   // IR lock (flash/SD)

static const char *jsonNames[NUM_IR_KEYS] = {
        "key0", "key1", "key2", "key3", "key4", 
        "key5", "key6", "key7", "key8", "key9", 
        "keySTAR", "keyHASH",
        "keyUP", "keyDOWN",
        "keyLEFT", "keyRIGHT",
        "keyOK" 
    };

static const char *fsNoAvail = "File System not available";
static const char *badConfig = "Settings bad/missing/incomplete; writing new file";
static const char *failFileWrite = "Failed to open file for writing";

static bool read_settings(File configFile);

static bool CopyCheckValidNumParm(const char *json, char *text, uint8_t psize, int lowerLim, int upperLim, int setDefault);
static bool CopyCheckValidNumParmF(const char *json, char *text, uint8_t psize, float lowerLim, float upperLim, float setDefault);
static bool checkValidNumParm(char *text, int lowerLim, int upperLim, int setDefault);
static bool checkValidNumParmF(char *text, float lowerLim, float upperLim, float setDefault);

static bool loadIRKeys();

static void open_and_copy(const char *fn, int& haveErr);
static bool filecopy(File source, File dest);
static bool check_if_default_audio_present();

static bool CopyIPParm(const char *json, char *text, uint8_t psize);

/*
 * settings_setup()
 * 
 * Mount SPIFFS/LittleFS and SD (if available).
 * Read configuration from JSON config file
 * If config file not found, create one with default settings
 *
 */
void settings_setup()
{
    const char *funcName = "settings_setup";
    bool writedefault = false;

    #ifdef FC_DBG
    Serial.printf("%s: Mounting flash FS... ", funcName);
    #endif

    if(SPIFFS.begin()) {

        haveFS = true;

    } else {

        #ifdef FC_DBG
        Serial.print(F("failed, formatting... "));
        #endif

        // Show the user some action
        showWaitSequence();

        SPIFFS.format();
        if(SPIFFS.begin()) haveFS = true;

        endWaitSequence();

    }

    if(haveFS) {
      
        #ifdef FC_DBG
        Serial.println(F("ok, loading settings"));
        #endif
        
        if(SPIFFS.exists(cfgName)) {
            File configFile = SPIFFS.open(cfgName, "r");
            if(configFile) {
                writedefault = read_settings(configFile);
                configFile.close();
            } else {
                writedefault = true;
            }
        } else {
            writedefault = true;
        }

        // Write new config file after mounting SD and determining FlashROMode

    } else {

        Serial.println(F("failed.\nThis is very likely a hardware problem."));
        Serial.println(F("*** Since the internal storage is unavailable, all settings/states will be stored on"));
        Serial.println(F("*** the SD card (if available)")); 
    }

    // Set up SD card
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

    haveSD = false;

    uint32_t sdfreq = (settings.sdFreq[0] == '0') ? 16000000 : 4000000;
    #ifdef FC_DBG
    Serial.printf("%s: SD/SPI frequency %dMHz\n", funcName, sdfreq / 1000000);
    #endif

    #ifdef FC_DBG
    Serial.printf("%s: Mounting SD... ", funcName);
    #endif

    if(!SD.begin(SD_CS_PIN, SPI, sdfreq)) {

        Serial.println(F("no SD card found"));

    } else {

        #ifdef FC_DBG
        Serial.println(F("ok"));
        #endif

        uint8_t cardType = SD.cardType();
       
        #ifdef FC_DBG
        const char *sdTypes[5] = { "No card", "MMC", "SD", "SDHC", "unknown (SD not usable)" };
        Serial.printf("SD card type: %s\n", sdTypes[cardType > 4 ? 4 : cardType]);
        #endif

        haveSD = ((cardType != CARD_NONE) && (cardType != CARD_UNKNOWN));

    }

    if(haveSD) {
        if(SD.exists("/FC_FLASH_RO") || !haveFS) {
            bool writedefault2 = false;
            FlashROMode = true;
            Serial.println(F("Flash-RO mode: All settings/states stored on SD. Reloading settings."));
            if(SD.exists(cfgName)) {
                File configFile = SD.open(cfgName, "r");
                if(configFile) {
                    writedefault2 = read_settings(configFile);
                    configFile.close();
                } else {
                    writedefault2 = true;
                }
            } else {
                writedefault2 = true;
            }
            if(writedefault2) {
                Serial.printf("%s: %s\n", funcName, badConfig);
                write_settings();
            }
        }
    }
   
    // Now write new config to flash FS if old one somehow bad
    // Only write this file if FlashROMode is off
    if(haveFS && writedefault && !FlashROMode) {
        Serial.printf("%s: %s\n", funcName, badConfig);
        write_settings();
    }

    // Determine if volume/ir settings are to be stored on SD
    configOnSD = (haveSD && ((settings.CfgOnSD[0] != '0') || FlashROMode));

    // Load user-config's and learned IR keys
    loadIRKeys();

    // Check if SD contains our default sound files
    if(haveFS && haveSD && !FlashROMode) {
        allowCPA = check_if_default_audio_present();
    }

}

static bool read_settings(File configFile)
{
    const char *funcName = "read_settings";
    bool wd = false;
    size_t jsonSize = 0;
    //StaticJsonDocument<JSON_SIZE> json;
    DynamicJsonDocument json(JSON_SIZE);
    
    DeserializationError error = deserializeJson(json, configFile);

    jsonSize = json.memoryUsage();
    if(jsonSize > JSON_SIZE) {
        Serial.printf("%s: ERROR: Config file too large (%d vs %d), memory corrupted, awaiting doom.\n", funcName, jsonSize, JSON_SIZE);
    }
    
    #ifdef FC_DBG
    if(jsonSize > JSON_SIZE - 256) {
          Serial.printf("%s: WARNING: JSON_SIZE needs to be adapted **************\n", funcName);
    }
    Serial.printf("%s: Size of document: %d (JSON_SIZE %d)\n", funcName, jsonSize, JSON_SIZE);
    serializeJson(json, Serial);
    Serial.println(F(" "));
    #endif

    if(!error) {

        wd |= CopyCheckValidNumParm(json["playFLUXsnd"], settings.playFLUXsnd, sizeof(settings.playFLUXsnd), 0, 3, DEF_PLAY_FLUX_SND);
        wd |= CopyCheckValidNumParm(json["playTTsnds"], settings.playTTsnds, sizeof(settings.playTTsnds), 0, 1, DEF_PLAY_TT_SND);
        wd |= CopyCheckValidNumParm(json["ssTimer"], settings.ssTimer, sizeof(settings.ssTimer), 0, 999, DEF_SS_TIMER);

        wd |= CopyCheckValidNumParm(json["usePLforBL"], settings.usePLforBL, sizeof(settings.usePLforBL), 0, 1, DEF_BLEDSWAP);
        wd |= CopyCheckValidNumParm(json["useVknob"], settings.useVknob, sizeof(settings.useVknob), 0, 1, DEF_VKNOB);
        wd |= CopyCheckValidNumParm(json["useSknob"], settings.useSknob, sizeof(settings.useSknob), 0, 1, DEF_SKNOB);
        wd |= CopyCheckValidNumParm(json["disDIR"], settings.disDIR, sizeof(settings.disDIR), 0, 1, DEF_DISDIR);

        wd |= CopyCheckValidNumParm(json["TCDpresent"], settings.TCDpresent, sizeof(settings.TCDpresent), 0, 1, DEF_TCD_PRES);

        if(json["hostName"]) {
            memset(settings.hostName, 0, sizeof(settings.hostName));
            strncpy(settings.hostName, json["hostName"], sizeof(settings.hostName) - 1);
        } else wd = true;
        wd |= CopyCheckValidNumParm(json["wifiConRetries"], settings.wifiConRetries, sizeof(settings.wifiConRetries), 1, 10, DEF_WIFI_RETRY);
        wd |= CopyCheckValidNumParm(json["wifiConTimeout"], settings.wifiConTimeout, sizeof(settings.wifiConTimeout), 7, 25, DEF_WIFI_TIMEOUT);

        if(json["tcdIP"]) {
            memset(settings.tcdIP, 0, sizeof(settings.tcdIP));
            strncpy(settings.tcdIP, json["tcdIP"], sizeof(settings.tcdIP) - 1);
        } else wd = true;

        //wd |= CopyCheckValidNumParm(json["wait4TCD"], settings.wait4TCD, sizeof(settings.wait4TCD), 0, 1, DEF_WAIT_FOR_TCD);
        wd |= CopyCheckValidNumParm(json["useGPSS"], settings.useGPSS, sizeof(settings.useGPSS), 0, 1, DEF_USE_GPSS);
        wd |= CopyCheckValidNumParm(json["useNM"], settings.useNM, sizeof(settings.useNM), 0, 1, DEF_USE_NM);
        wd |= CopyCheckValidNumParm(json["useFPO"], settings.useFPO, sizeof(settings.useFPO), 0, 1, DEF_USE_FPO);
        wd |= CopyCheckValidNumParm(json["wait4FPOn"], settings.wait4FPOn, sizeof(settings.wait4FPOn), 0, 1, DEF_WAIT_FPO);

        #ifdef FC_HAVEMQTT
        wd |= CopyCheckValidNumParm(json["useMQTT"], settings.useMQTT, sizeof(settings.useMQTT), 0, 1, 0);
        if(json["mqttServer"]) {
            memset(settings.mqttServer, 0, sizeof(settings.mqttServer));
            strncpy(settings.mqttServer, json["mqttServer"], sizeof(settings.mqttServer) - 1);
        } else wd = true;
        if(json["mqttUser"]) {
            memset(settings.mqttUser, 0, sizeof(settings.mqttUser));
            strncpy(settings.mqttUser, json["mqttUser"], sizeof(settings.mqttUser) - 1);
        } else wd = true;
        #endif

        wd |= CopyCheckValidNumParm(json["shuffle"], settings.shuffle, sizeof(settings.shuffle), 0, 1, DEF_SHUFFLE);
        
        wd |= CopyCheckValidNumParm(json["CfgOnSD"], settings.CfgOnSD, sizeof(settings.CfgOnSD), 0, 1, DEF_CFG_ON_SD);
        //wd |= CopyCheckValidNumParm(json["sdFreq"], settings.sdFreq, sizeof(settings.sdFreq), 0, 1, DEF_SD_FREQ);

    } else {

        wd = true;

    }

    return wd;
}

void write_settings()
{
    const char *funcName = "write_settings";
    DynamicJsonDocument json(JSON_SIZE);
    //StaticJsonDocument<JSON_SIZE> json;

    if(!haveFS && !FlashROMode) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    #ifdef FC_DBG
    Serial.printf("%s: Writing config file\n", funcName);
    #endif

    json["playFLUXsnd"] = settings.playFLUXsnd;
    json["playTTsnds"] = settings.playTTsnds;

    json["ssTimer"] = settings.ssTimer;

    json["usePLforBL"] = settings.usePLforBL;
    json["useVknob"] = settings.useVknob;
    json["useSknob"] = settings.useSknob;
    json["disDIR"] = settings.disDIR;

    json["TCDpresent"] = settings.TCDpresent;
    
    json["hostName"] = settings.hostName;
    json["wifiConRetries"] = settings.wifiConRetries;
    json["wifiConTimeout"] = settings.wifiConTimeout;

    json["tcdIP"] = settings.tcdIP;
    //json["wait4TCD"] = settings.wait4TCD;
    json["useGPSS"] = settings.useGPSS;
    json["useNM"] = settings.useNM;
    json["useFPO"] = settings.useFPO;
    json["wait4FPOn"] = settings.wait4FPOn;

    #ifdef FC_HAVEMQTT
    json["useMQTT"] = settings.useMQTT;
    json["mqttServer"] = settings.mqttServer;
    json["mqttUser"] = settings.mqttUser;
    #endif

    json["shuffle"] = settings.shuffle;
    
    json["CfgOnSD"] = settings.CfgOnSD;
    //json["sdFreq"] = settings.sdFreq;

    File configFile = FlashROMode ? SD.open(cfgName, FILE_WRITE) : SPIFFS.open(cfgName, FILE_WRITE);

    if(configFile) {

        #ifdef FC_DBG
        serializeJson(json, Serial);
        Serial.println(F(" "));
        #endif
        
        serializeJson(json, configFile);
        configFile.close();

    } else {

        Serial.printf("%s: %s\n", funcName, failFileWrite);

    }
}

bool checkConfigExists()
{
    return FlashROMode ? SD.exists(cfgName) : SPIFFS.exists(cfgName);
}


/*
 *  Helpers for parm copying & checking
 */

static bool CopyCheckValidNumParm(const char *json, char *text, uint8_t psize, int lowerLim, int upperLim, int setDefault)
{
    if(!json) return true;

    memset(text, 0, psize);
    strncpy(text, json, psize-1);
    return checkValidNumParm(text, lowerLim, upperLim, setDefault);
}

static bool CopyCheckValidNumParmF(const char *json, char *text, uint8_t psize, float lowerLim, float upperLim, float setDefault)
{
    if(!json) return true;

    memset(text, 0, psize);
    strncpy(text, json, psize-1);
    return checkValidNumParmF(text, lowerLim, upperLim, setDefault);
}

static bool checkValidNumParm(char *text, int lowerLim, int upperLim, int setDefault)
{
    int i, len = strlen(text);

    if(len == 0) {
        sprintf(text, "%d", setDefault);
        return true;
    }

    for(i = 0; i < len; i++) {
        if(text[i] < '0' || text[i] > '9') {
            sprintf(text, "%d", setDefault);
            return true;
        }
    }

    i = (int)(atoi(text));

    if(i < lowerLim) {
        sprintf(text, "%d", lowerLim);
        return true;
    }
    if(i > upperLim) {
        sprintf(text, "%d", upperLim);
        return true;
    }

    // Re-do to get rid of formatting errors (eg "000")
    sprintf(text, "%d", i);

    return false;
}

static bool checkValidNumParmF(char *text, float lowerLim, float upperLim, float setDefault)
{
    int i, len = strlen(text);
    float f;

    if(len == 0) {
        sprintf(text, "%1.1f", setDefault);
        return true;
    }

    for(i = 0; i < len; i++) {
        if(text[i] != '.' && text[i] != '-' && (text[i] < '0' || text[i] > '9')) {
            sprintf(text, "%1.1f", setDefault);
            return true;
        }
    }

    f = atof(text);

    if(f < lowerLim) {
        sprintf(text, "%1.1f", lowerLim);
        return true;
    }
    if(f > upperLim) {
        sprintf(text, "%1.1f", upperLim);
        return true;
    }

    // Re-do to get rid of formatting errors (eg "0.")
    sprintf(text, "%1.1f", f);

    return false;
}

static bool openCfgFileRead(const char *fn, File& f)
{
    bool haveConfigFile;
    
    if(configOnSD) {
        if(SD.exists(fn)) {
            haveConfigFile = (f = SD.open(fn, "r"));
        }
    } else {
        if(SPIFFS.exists(fn)) {
            haveConfigFile = (f = SPIFFS.open(fn, "r"));
        }
    }

    return haveConfigFile;
}

static bool openCfgFileWrite(const char *fn, File& f)
{
    bool haveConfigFile;
    
    if(configOnSD) {
        haveConfigFile = (f = SD.open(fn, FILE_WRITE));
    } else {
        haveConfigFile = (f = SPIFFS.open(fn, FILE_WRITE));
    }

    return haveConfigFile;
}

/*
 *  Load/save the Volume
 */

bool loadCurVolume()
{
    const char *funcName = "loadCurVolume";
    char temp[6];
    File configFile;

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return false;
    }

    #ifdef FC_DBG
    Serial.printf("%s: Loading from %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    if(openCfgFileRead(volCfgName, configFile)) {
        StaticJsonDocument<512> json;
        if(!deserializeJson(json, configFile)) {
            if(!CopyCheckValidNumParm(json["volume"], temp, sizeof(temp), 0, 19, DEFAULT_VOLUME)) {
                curSoftVol = atoi(temp);
            }
        } 
        configFile.close();
    }

    // Do not write a default file, use pre-set value

    prevSavedVol = curSoftVol;

    return true;
}

void saveCurVolume(bool useCache)
{
    const char *funcName = "saveCurVolume";
    char buf[6];
    File configFile;
    StaticJsonDocument<512> json;

    if(useCache && (prevSavedVol == curSoftVol)) {
        #ifdef FC_DBG
        Serial.printf("%s: Prev. saved vol identical, not writing\n", funcName);
        #endif
        return;
    }

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    #ifdef FC_DBG
    Serial.printf("%s: Writing to %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    sprintf(buf, "%d", curSoftVol);
    json["volume"] = (char *)buf;

    #ifdef FC_DBG
    serializeJson(json, Serial);
    Serial.println(F(" "));
    #endif

    if(openCfgFileWrite(volCfgName, configFile)) {
        serializeJson(json, configFile);
        configFile.close();
        prevSavedVol = curSoftVol;
    } else {
        Serial.printf("%s: %s\n", funcName, failFileWrite);
    }
}

/*
 *  Load/save the Speed
 */

bool loadCurSpeed()
{
    const char *funcName = "loadCurSpeed";
    char temp[6];
    File configFile;

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return false;
    }

    #ifdef FC_DBG
    Serial.printf("%s: Loading from %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    if(openCfgFileRead(spdCfgName, configFile)) {
        StaticJsonDocument<512> json;
        if(!deserializeJson(json, configFile)) {
            if(!CopyCheckValidNumParm(json["speed"], temp, sizeof(temp), FC_SPD_MAX, FC_SPD_MIN, FC_SPD_IDLE)) {
                lastIRspeed = atoi(temp);
            }
        } 
        configFile.close();
    }

    // Do not write a default file, use pre-set value

    prevSavedSpd = lastIRspeed;

    return true;
}

void saveCurSpeed(bool useCache)
{
    const char *funcName = "saveCurSpeed";
    char buf[6];
    File configFile;
    StaticJsonDocument<512> json;

    if(useCache && (prevSavedSpd == lastIRspeed)) {
        #ifdef FC_DBG
        Serial.printf("%s: Prev. saved spd identical, not writing\n", funcName);
        #endif
        return;
    }

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    #ifdef FC_DBG
    Serial.printf("%s: Writing to %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    sprintf(buf, "%d", lastIRspeed);
    json["speed"] = (char *)buf;

    #ifdef FC_DBG
    serializeJson(json, Serial);
    Serial.println(F(" "));
    #endif

    if(openCfgFileWrite(spdCfgName, configFile)) {
        serializeJson(json, configFile);
        configFile.close();
        prevSavedSpd = lastIRspeed;
    } else {
        Serial.printf("%s: %s\n", funcName, failFileWrite);
    }
}

/*
 *  Load/save the Minimum Box Light Level
 */

bool loadBLLevel()
{
    const char *funcName = "loadBLLevel";
    char temp[6];
    File configFile;

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return false;
    }

    #ifdef FC_DBG
    Serial.printf("%s: Loading from %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    if(openCfgFileRead(bllCfgName, configFile)) {
        StaticJsonDocument<512> json;
        if(!deserializeJson(json, configFile)) {
            if(!CopyCheckValidNumParm(json["mbll"], temp, sizeof(temp), 0, 4, 0)) {
                minBLL = atoi(temp);
            }
        } 
        configFile.close();
    }

    // Do not write a default file, use pre-set value

    prevSavedBLL = minBLL;

    return true;
}

void saveBLLevel(bool useCache)
{
    const char *funcName = "saveBLLevel";
    char buf[6];
    File configFile;
    StaticJsonDocument<512> json;

    if(useCache && (prevSavedBLL == minBLL)) {
        #ifdef FC_DBG
        Serial.printf("%s: Prev. saved bll identical, not writing\n", funcName);
        #endif
        return;
    }

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    #ifdef FC_DBG
    Serial.printf("%s: Writing to %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    sprintf(buf, "%d", minBLL);
    json["mbll"] = (char *)buf;

    #ifdef FC_DBG
    serializeJson(json, Serial);
    Serial.println(F(" "));
    #endif

    if(openCfgFileWrite(bllCfgName, configFile)) {
        serializeJson(json, configFile);
        configFile.close();
        prevSavedBLL = minBLL;
    } else {
        Serial.printf("%s: %s\n", funcName, failFileWrite);
    }
}

/*
 *  Load/save the IR lock status
 */

bool loadIRLock()
{
    const char *funcName = "loadIRLock";
    char temp[6];
    File configFile;

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return false;
    }

    #ifdef FC_DBG
    Serial.printf("%s: Loading from %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    if(openCfgFileRead(irlCfgName, configFile)) {
        StaticJsonDocument<512> json;
        if(!deserializeJson(json, configFile)) {
            if(!CopyCheckValidNumParm(json["lock"], temp, sizeof(temp), 0, 1, 0)) {
                irLocked = (atoi(temp) > 0);
            }
        } 
        configFile.close();
    }

    // Do not write a default file, use pre-set value

    prevSavedIRL = irLocked;

    return true;
}

void saveIRLock(bool useCache)
{
    const char *funcName = "saveIRLock";
    char buf[6];
    File configFile;
    StaticJsonDocument<512> json;

    if(useCache && (prevSavedIRL == irLocked)) {
        #ifdef FC_DBG
        Serial.printf("%s: Prev. saved irl identical, not writing\n", funcName);
        #endif
        return;
    }

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    #ifdef FC_DBG
    Serial.printf("%s: Writing to %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    sprintf(buf, "%d", irLocked ? 1 : 0);
    json["lock"] = (char *)buf;

    #ifdef FC_DBG
    serializeJson(json, Serial);
    Serial.println(F(" "));
    #endif

    if(openCfgFileWrite(irlCfgName, configFile)) {
        serializeJson(json, configFile);
        configFile.close();
        prevSavedIRL = irLocked;
    } else {
        Serial.printf("%s: %s\n", funcName, failFileWrite);
    }
}

/*
 * Load custom IR config
 */

static bool loadIRkeysFromFile(File configFile, int index)
{
    StaticJsonDocument<1024> json;
    DeserializationError err = deserializeJson(json, configFile);
    uint32_t ir_keys[NUM_IR_KEYS];

    if(!err) {
        size_t jsonSize = json.memoryUsage();
        int j = 0;
        if(!index) Serial.printf("%s JSON parsed, size %d\n", configFile.name(), jsonSize);
        for(int i = 0; i < NUM_IR_KEYS; i++) {
            if(json[jsonNames[i]]) {
                j++;
                ir_keys[i] = (uint32_t)strtoul(json[(const char *)jsonNames[i]], NULL, 16);
                #ifdef FC_DBG
                Serial.printf("Adding IR %s - 0x%08x\n", jsonNames[i], ir_keys[i]);
                #endif
            } else {
                if(!index) Serial.printf("IR %s missing in %s\n", jsonNames[i], configFile.name());
            }
        }
        populateIRarray(ir_keys, index);
        if(!index) Serial.printf("%d IR keys added from %s\n", j, configFile.name());
    } else {
        if(!index) Serial.printf("Error parsing %s: %s\n", configFile.name(), err.c_str());
    }
    configFile.close();

    return true;
}

static bool loadIRKeys()
{
    File configFile;

    // Load user keys from SD
    if(haveSD) {
        if(SD.exists(irUCfgName)) {
            configFile = SD.open(irUCfgName, "r");
            if(configFile) {
                loadIRkeysFromFile(configFile, 0);
            } else {
                Serial.printf("%s not found on SD card\n", irUCfgName);
            }
        }
    }

    // Load learned keys from Flash/SD
    if(openCfgFileRead(irCfgName, configFile)) {
        loadIRkeysFromFile(configFile, 1);
    } else {
        #ifdef FC_DBG
        Serial.printf("%s does not exist\n", irCfgName);
        #endif
    }

    return true;
}

bool saveIRKeys()
{
    StaticJsonDocument<1024> json;
    File configFile;
    uint32_t ir_keys[NUM_IR_KEYS];
    char buf[12];

    if(!haveFS && !configOnSD)
        return true;

    copyIRarray(ir_keys, 1);

    for(int i = 0; i < NUM_IR_KEYS; i++) {
        sprintf(buf, "0x%08x", ir_keys[i]);
        json[(const char *)jsonNames[i]] = buf;
    }

    #ifdef FC_DBG
    serializeJson(json, Serial);
    Serial.println(F(" "));
    #endif

    if(openCfgFileWrite(irCfgName, configFile)) {
        serializeJson(json, configFile);
        configFile.close();
    } else {
        Serial.printf("saveIRKeys: %s\n", failFileWrite);
    }
    
    return true;
}

/* Copy volume/IR settings from/to SD if user
 * changed "save to SD"-option in CP
 */

void copySettings()
{
    if(!haveSD || !haveFS)
        return;

    configOnSD = !configOnSD;

    if(configOnSD || !FlashROMode) {
        #ifdef FC_DBG
        Serial.println(F("copySettings: Copying vol/speed/IR/etc settings to other medium"));
        #endif
        saveCurVolume(false);
        saveCurSpeed(false);
        saveBLLevel(false);
        saveIRLock(false);
        saveIRKeys();
    }

    configOnSD = !configOnSD;
}

/*
 * Load/save Music Folder Number
 */

bool loadMusFoldNum()
{
    bool writedefault = true;
    char temp[4];

    if(!haveSD)
        return false;

    if(SD.exists(musCfgName)) {

        File configFile = SD.open(musCfgName, "r");
        if(configFile) {
            StaticJsonDocument<512> json;
            if(!deserializeJson(json, configFile)) {
                if(!CopyCheckValidNumParm(json["folder"], temp, sizeof(temp), 0, 9, 0)) {
                    musFolderNum = atoi(temp);
                    writedefault = false;
                }
            } 
            configFile.close();
        }

    }

    if(writedefault) {
        musFolderNum = 0;
        saveMusFoldNum();
    }

    return true;
}

void saveMusFoldNum()
{
    const char *funcName = "saveMusFoldNum";
    StaticJsonDocument<512> json;
    char buf[4];

    if(!haveSD)
        return;

    sprintf(buf, "%1d", musFolderNum);
    json["folder"] = buf;
    
    File configFile = SD.open(musCfgName, FILE_WRITE);

    if(configFile) {
        serializeJson(json, configFile);
        configFile.close();
    } else {
        Serial.printf("%s: %s\n", funcName, failFileWrite);
    }
}

/*
 * Load/save/delete settings for static IP configuration
 */

bool loadIpSettings()
{
    bool invalid = false;
    bool haveConfig = false;

    if(!haveFS && !FlashROMode)
        return false;

    if( (!FlashROMode && SPIFFS.exists(ipCfgName)) ||
        (FlashROMode && SD.exists(ipCfgName)) ) {

        File configFile = FlashROMode ? SD.open(ipCfgName, "r") : SPIFFS.open(ipCfgName, "r");

        if(configFile) {

            StaticJsonDocument<512> json;
            DeserializationError error = deserializeJson(json, configFile);

            #ifdef FC_DBG
            serializeJson(json, Serial);
            Serial.println(F(" "));
            #endif

            if(!error) {

                invalid |= CopyIPParm(json["IpAddress"], ipsettings.ip, sizeof(ipsettings.ip));
                invalid |= CopyIPParm(json["Gateway"], ipsettings.gateway, sizeof(ipsettings.gateway));
                invalid |= CopyIPParm(json["Netmask"], ipsettings.netmask, sizeof(ipsettings.netmask));
                invalid |= CopyIPParm(json["DNS"], ipsettings.dns, sizeof(ipsettings.dns));
                
                haveConfig = !invalid;

            } else {

                invalid = true;

            }

            configFile.close();

        }

    }

    if(invalid) {

        // config file is invalid - delete it

        Serial.println(F("loadIpSettings: IP settings invalid; deleting file"));

        deleteIpSettings();

        memset(ipsettings.ip, 0, sizeof(ipsettings.ip));
        memset(ipsettings.gateway, 0, sizeof(ipsettings.gateway));
        memset(ipsettings.netmask, 0, sizeof(ipsettings.netmask));
        memset(ipsettings.dns, 0, sizeof(ipsettings.dns));

    }

    return haveConfig;
}

static bool CopyIPParm(const char *json, char *text, uint8_t psize)
{
    if(!json) return true;

    if(strlen(json) == 0) 
        return true;

    memset(text, 0, psize);
    strncpy(text, json, psize-1);
    return false;
}

void writeIpSettings()
{
    StaticJsonDocument<512> json;

    if(!haveFS && !FlashROMode)
        return;

    if(strlen(ipsettings.ip) == 0)
        return;

    #ifdef FC_DBG
    Serial.println(F("writeIpSettings: Writing file"));
    #endif
    
    json["IpAddress"] = ipsettings.ip;
    json["Gateway"] = ipsettings.gateway;
    json["Netmask"] = ipsettings.netmask;
    json["DNS"] = ipsettings.dns;

    File configFile = FlashROMode ? SD.open(ipCfgName, FILE_WRITE) : SPIFFS.open(ipCfgName, FILE_WRITE);

    #ifdef FC_DBG
    serializeJson(json, Serial);
    Serial.println(F(" "));
    #endif

    if(configFile) {
        serializeJson(json, configFile);
        configFile.close();
    } else {
        Serial.printf("writeIpSettings: %s\n", failFileWrite);
    }
}

void deleteIpSettings()
{
    if(!haveFS && !FlashROMode)
        return;

    #ifdef FC_DBG
    Serial.println(F("deleteIpSettings: Deleting ip config"));
    #endif

    if(FlashROMode) {
        SD.remove(ipCfgName);
    } else {
        SPIFFS.remove(ipCfgName);
    }
}


/*
 * Audio file installer
 *
 * Copies our default audio files from SD to flash FS.
 * The is restricted to the original default audio
 * files that came with the software. If you want to
 * customize your sounds, put them on a FAT32 formatted
 * SD card and leave this SD card in the slot.
 */

bool check_allow_CPA()
{
    return allowCPA;
}

#define SND_KEY_LEN 98742

static bool check_if_default_audio_present()
{
    File file;
    size_t ts;
    int i, idx = 0;
    size_t sizes[NUM_AUDIOFILES] = {
      9404, 7523, 5642, 6582, 6582,         // 0-4
      7836, 8463, 8463, 5015, 8777,         // 5-9
      5955,                                 // dot
      712515,                               // flux (loop) 
      57259,                                // startup
      46392, SND_KEY_LEN,                   // timetravel, travelstart
      36989,                                // fluxing
      43153,                                // renaming
      42212                                 // installing (not copied)
    };

    if(!haveSD)
        return false;

    // If identifier missing, quit now
    if(!(SD.exists(IDFN))) {
        #ifdef FC_DBG
        Serial.println("SD: ID file not present");
        #endif
        return false;
    }

    for(i = 0; i < NUM_AUDIOFILES; i++) {
        if(!SD.exists(audioFiles[i])) {
            #ifdef FC_DBG
            Serial.printf("missing: %s\n", audioFiles[i]);
            #endif
            return false;
        }
        if(!(file = SD.open(audioFiles[i])))
            return false;
        ts = file.size();
        file.close();
        #ifdef FC_DBG
        sizes[idx++] = ts;
        #else
        if(sizes[idx++] != ts)
            return false;
        #endif
    }

    #ifdef FC_DBG
    for(i = 0; i < (NUM_AUDIOFILES); i++) {
        Serial.printf("%d, ", sizes[i]);
    }
    Serial.println("");
    #endif

    return true;
}

/*
 * Install default audio files from SD to flash FS #############
 */

void doCopyAudioFiles()
{
    bool delIDfile = false;

    if(!copy_audio_files()) {
        // If copy fails, re-format flash FS
        formatFlashFS();            // Format
        rewriteSecondarySettings(); // Re-write ip/vol/speed/ir/etc settings
        #ifdef FC_DBG 
        Serial.println("Re-writing general settings");
        #endif
        write_settings();           // Re-write general settings
        if(!copy_audio_files()) {   // Retry copy
            showCopyError();
            mydelay(5000, false);
        } else {
            delIDfile = true;
        }
    } else {
        delIDfile = true;
    }

    if(delIDfile)
        delete_ID_file();

    mydelay(500, false);
    
    esp_restart();
}


bool copy_audio_files()
{
    int i, haveErr = 0;

    if(!allowCPA) {
        return false;
    }

    // Signal somehow
    //start_file_copy();

    for(i = 0; i < NUM_AUDIOFILES - 1; i++) {
        open_and_copy(audioFiles[i], haveErr);
    }

    if(haveErr) {
        //file_copy_error();
    } else {
        //file_copy_done();
    }

    return (haveErr == 0);
}

static void open_and_copy(const char *fn, int& haveErr)
{
    const char *funcName = "copy_audio_files";
    File sFile, dFile;

    if((sFile = SD.open(fn, FILE_READ))) {
        #ifdef FC_DBG
        Serial.printf("%s: Opened source file: %s\n", funcName, fn);
        #endif
        if((dFile = SPIFFS.open(fn, FILE_WRITE))) {
            #ifdef FC_DBG
            Serial.printf("%s: Opened destination file: %s\n", funcName, fn);
            #endif
            if(!filecopy(sFile, dFile)) {
                haveErr++;
            }
            dFile.close();
        } else {
            Serial.printf("%s: Error opening destination file: %s\n", funcName, fn);
            haveErr++;
        }
        sFile.close();
    } else {
        Serial.printf("%s: Error opening source file: %s\n", funcName, fn);
        haveErr++;
    }
}

static bool filecopy(File source, File dest)
{
    uint8_t buffer[1024];
    size_t bytesr, bytesw;

    while((bytesr = source.read(buffer, 1024))) {
        if((bytesw = dest.write(buffer, bytesr)) != bytesr) {
            Serial.println(F("filecopy: Error writing data"));
            return false;
        }
        //file_copy_progress();
    }

    return true;
}

bool audio_files_present()
{
    File file;
    size_t ts;
    
    if(FlashROMode)
        return true;

    if(!SPIFFS.exists(audioFiles[SND_KEY_IDX]))
        return false;
      
    if(!(file = SPIFFS.open(audioFiles[SND_KEY_IDX])))
        return false;
      
    ts = file.size();
    file.close();

    if(ts != SND_KEY_LEN)
        return false;

    return true;
}

void delete_ID_file()
{
    if(!haveSD)
        return;

    #ifdef FC_DBG
    Serial.printf("Deleting ID file %s\n", IDFN);
    #endif
        
    if(SD.exists(IDFN)) {
        SD.remove(IDFN);
    }
}

/*
 * Various helpers
 */

void formatFlashFS()
{
    #ifdef FC_DBG
    Serial.println(F("Formatting flash FS"));
    #endif
    SPIFFS.format();
}

// Re-write IP/speed/vol/etc settings
// Used during audio file installation when flash FS needs
// to be re-formatted.
void rewriteSecondarySettings()
{
    bool oldconfigOnSD = configOnSD;
    
    #ifdef FC_DBG
    Serial.println("Re-writing secondary settings");
    #endif
    
    writeIpSettings();

    // Create current settings on flash FS
    // regardless of SD-option
    configOnSD = false;
       
    saveCurVolume(false);
    saveCurSpeed(false);
    saveBLLevel(false);
    saveIRLock(false);
    saveIRKeys();
    
    configOnSD = oldconfigOnSD;
}
