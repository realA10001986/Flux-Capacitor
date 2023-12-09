/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Flux Capacitor
 * (C) 2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Flux-Capacitor
 * https://fc.backtothefutu.re
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

#define ARDUINOJSON_USE_LONG_LONG 0
#define ARDUINOJSON_USE_DOUBLE 0
#define ARDUINOJSON_ENABLE_ARDUINO_STRING 0
#define ARDUINOJSON_ENABLE_ARDUINO_STREAM 0
#define ARDUINOJSON_ENABLE_STD_STREAM 0
#define ARDUINOJSON_ENABLE_STD_STRING 0
#define ARDUINOJSON_ENABLE_NAN 0
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
bool haveFS = false;

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

/* Cache for idle pattern */
static uint8_t prevSavedIM = 0;

/* Cache for IR lock */
static bool prevSavedIRL = 0;

/* Paranoia: No writes Flash-FS  */
bool FlashROMode = false;

#define NUM_AUDIOFILES 11+8
#define SND_KEY_IDX  11+3
static const char *audioFiles[NUM_AUDIOFILES] = {
      "/0.mp3", "/1.mp3", "/2.mp3", "/3.mp3", "/4.mp3", 
      "/5.mp3", "/6.mp3", "/7.mp3", "/8.mp3", "/9.mp3", 
      "/dot.mp3", 
      "/flux.mp3",
      "/startup.mp3",
      "/timetravel.mp3",
      "/travelstart.mp3",
      "/alarm.mp3",
      "/fluxing.mp3",
      "/renaming.mp3",
      "/installing.mp3"
};
static const char *IDFN = "/FC_def_snd.txt";

static const char *cfgName    = "/fcconfig.json";   // Main config (flash)
static const char *ipCfgName  = "/fcipcfg.json";    // IP config (flash)
static const char *volCfgName = "/fcvolcfg.json";   // Volume config (flash/SD)
static const char *spdCfgName = "/fcspdcfg.json";   // Speed config (flash/SD)
static const char *bllCfgName = "/fcbllcfg.json";   // Minimum box light level config (flash/SD)
static const char *irCfgName  = "/fcirkeys.json";   // IR keys (system-created) (flash/SD)
static const char *irlCfgName = "/fcirlcfg.json";   // IR lock (flash/SD)
static const char *musCfgName = "/fcmcfg.json";     // Music config (SD)
static const char *irUCfgName = "/fcirkeys.txt";    // IR keys (user-created) (SD)
static const char *ipaCfgName = "/fcipat.json";     // Idle pattern (SD only)

static const char *jsonNames[NUM_IR_KEYS] = {
        "key0", "key1", "key2", "key3", "key4", 
        "key5", "key6", "key7", "key8", "key9", 
        "keySTAR", "keyHASH",
        "keyUP", "keyDOWN",
        "keyLEFT", "keyRIGHT",
        "keyOK" 
    };

static const char *fsNoAvail = "Filesystem not available";
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

static void formatFlashFS();

static bool CopyIPParm(const char *json, char *text, uint8_t psize);

static DeserializationError readJSONCfgFile(JsonDocument& json, File& configFile, const char *funcName);
static bool writeJSONCfgFile(const JsonDocument& json, const char *fn, bool useSD, const char *funcName);
static bool writeFileToSD(const char *fn, uint8_t *buf, int len);
static bool writeFileToFS(const char *fn, uint8_t *buf, int len);

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
    bool SDres = false;

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
        int tBytes = SPIFFS.totalBytes(); int uBytes = SPIFFS.usedBytes();
        Serial-printf("FlashFS: %d total, %d used\n", tBytes, uBytes);
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

    if(!(SDres = SD.begin(SD_CS_PIN, SPI, sdfreq))) {
        #ifdef FC_DBG
        Serial.printf("Retrying at 25Mhz... ");
        #endif
        SDres = SD.begin(SD_CS_PIN, SPI, 25000000);
    }

    if(SDres) {

        #ifdef FC_DBG
        Serial.println(F("ok"));
        #endif

        uint8_t cardType = SD.cardType();
       
        #ifdef FC_DBG
        const char *sdTypes[5] = { "No card", "MMC", "SD", "SDHC", "unknown (SD not usable)" };
        Serial.printf("SD card type: %s\n", sdTypes[cardType > 4 ? 4 : cardType]);
        #endif

        haveSD = ((cardType != CARD_NONE) && (cardType != CARD_UNKNOWN));

    } else {
      
        Serial.println(F("no SD card found"));
        
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

    // Determine if secondary settings are to be stored on SD
    configOnSD = (haveSD && ((settings.CfgOnSD[0] != '0') || FlashROMode));

    // Load user-config's and learned IR keys
    loadIRKeys();

    // Check if SD contains our default sound files
    if(haveFS && haveSD && !FlashROMode) {
        allowCPA = check_if_default_audio_present();
    }
}

void unmount_fs()
{
    if(haveFS) {
        SPIFFS.end();
        #ifdef TC_DBG
        Serial.println(F("Unmounted Flash FS"));
        #endif
        haveFS = false;
    }
    if(haveSD) {
        SD.end();
        #ifdef TC_DBG
        Serial.println(F("Unmounted SD card"));
        #endif
        haveSD = false;
    }
}

static bool read_settings(File configFile)
{
    const char *funcName = "read_settings";
    bool wd = false;
    size_t jsonSize = 0;
    //StaticJsonDocument<JSON_SIZE> json;
    DynamicJsonDocument json(JSON_SIZE);
    
    DeserializationError error = readJSONCfgFile(json, configFile, funcName);

    jsonSize = json.memoryUsage();
    if(jsonSize > JSON_SIZE) {
        Serial.printf("%s: ERROR: Config file too large (%d vs %d), memory corrupted, awaiting doom.\n", funcName, jsonSize, JSON_SIZE);
    }
    
    #ifdef FC_DBG
    if(jsonSize > JSON_SIZE - 256) {
          Serial.printf("%s: WARNING: JSON_SIZE needs to be adapted **************\n", funcName);
    }
    Serial.printf("%s: Size of document: %d (JSON_SIZE %d)\n", funcName, jsonSize, JSON_SIZE);
    #endif

    if(!error) {

        wd |= CopyCheckValidNumParm(json["playFLUXsnd"], settings.playFLUXsnd, sizeof(settings.playFLUXsnd), 0, 3, DEF_PLAY_FLUX_SND);
        wd |= CopyCheckValidNumParm(json["ssTimer"], settings.ssTimer, sizeof(settings.ssTimer), 0, 999, DEF_SS_TIMER);

        wd |= CopyCheckValidNumParm(json["usePLforBL"], settings.usePLforBL, sizeof(settings.usePLforBL), 0, 1, DEF_BLEDSWAP);
        wd |= CopyCheckValidNumParm(json["useVknob"], settings.useVknob, sizeof(settings.useVknob), 0, 1, DEF_VKNOB);
        wd |= CopyCheckValidNumParm(json["useSknob"], settings.useSknob, sizeof(settings.useSknob), 0, 1, DEF_SKNOB);
        wd |= CopyCheckValidNumParm(json["disDIR"], settings.disDIR, sizeof(settings.disDIR), 0, 1, DEF_DISDIR);

        if(json["hostName"]) {
            memset(settings.hostName, 0, sizeof(settings.hostName));
            strncpy(settings.hostName, json["hostName"], sizeof(settings.hostName) - 1);
        } else wd = true;
        if(json["systemID"]) {
            memset(settings.systemID, 0, sizeof(settings.systemID));
            strncpy(settings.systemID, json["systemID"], sizeof(settings.systemID) - 1);
        } else wd = true;

        if(json["appw"]) {
            memset(settings.appw, 0, sizeof(settings.appw));
            strncpy(settings.appw, json["appw"], sizeof(settings.appw) - 1);
        } else wd = true;
        
        wd |= CopyCheckValidNumParm(json["wifiConRetries"], settings.wifiConRetries, sizeof(settings.wifiConRetries), 1, 10, DEF_WIFI_RETRY);
        wd |= CopyCheckValidNumParm(json["wifiConTimeout"], settings.wifiConTimeout, sizeof(settings.wifiConTimeout), 7, 25, DEF_WIFI_TIMEOUT);

        wd |= CopyCheckValidNumParm(json["TCDpresent"], settings.TCDpresent, sizeof(settings.TCDpresent), 0, 1, DEF_TCD_PRES);
        wd |= CopyCheckValidNumParm(json["noETTOLead"], settings.noETTOLead, sizeof(settings.noETTOLead), 0, 1, DEF_NO_ETTO_LEAD);

        if(json["tcdIP"]) {
            memset(settings.tcdIP, 0, sizeof(settings.tcdIP));
            strncpy(settings.tcdIP, json["tcdIP"], sizeof(settings.tcdIP) - 1);
        } else wd = true;
        wd |= CopyCheckValidNumParm(json["useGPSS"], settings.useGPSS, sizeof(settings.useGPSS), 0, 1, DEF_USE_GPSS);
        wd |= CopyCheckValidNumParm(json["useNM"], settings.useNM, sizeof(settings.useNM), 0, 1, DEF_USE_NM);
        wd |= CopyCheckValidNumParm(json["useFPO"], settings.useFPO, sizeof(settings.useFPO), 0, 1, DEF_USE_FPO);
        wd |= CopyCheckValidNumParm(json["bttfnTT"], settings.bttfnTT, sizeof(settings.bttfnTT), 0, 1, DEF_BTTFN_TT);

        wd |= CopyCheckValidNumParm(json["playTTsnds"], settings.playTTsnds, sizeof(settings.playTTsnds), 0, 1, DEF_PLAY_TT_SND);
        wd |= CopyCheckValidNumParm(json["skipTTBLAnim"], settings.skipTTBLAnim, sizeof(settings.skipTTBLAnim), 0, 1, DEF_STTBL_ANIM);
        wd |= CopyCheckValidNumParm(json["playALsnd"], settings.playALsnd, sizeof(settings.playALsnd), 0, 1, DEF_PLAY_ALM_SND);

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

    json["playFLUXsnd"] = (const char *)settings.playFLUXsnd;
    json["ssTimer"] = (const char *)settings.ssTimer;

    json["usePLforBL"] = (const char *)settings.usePLforBL;
    json["useVknob"] = (const char *)settings.useVknob;
    json["useSknob"] = (const char *)settings.useSknob;
    json["disDIR"] = (const char *)settings.disDIR;

    json["hostName"] = (const char *)settings.hostName;
    json["systemID"] = (const char *)settings.systemID;
    json["appw"] = (const char *)settings.appw;
    json["wifiConRetries"] = (const char *)settings.wifiConRetries;
    json["wifiConTimeout"] = (const char *)settings.wifiConTimeout;

    json["TCDpresent"] = (const char *)settings.TCDpresent;
    json["noETTOLead"] = (const char *)settings.noETTOLead;
    
    json["tcdIP"] = (const char *)settings.tcdIP;
    json["useGPSS"] = (const char *)settings.useGPSS;
    json["useNM"] = (const char *)settings.useNM;
    json["useFPO"] = (const char *)settings.useFPO;
    json["bttfnTT"] = (const char *)settings.bttfnTT;

    json["playTTsnds"] = (const char *)settings.playTTsnds;
    json["skipTTBLAnim"] = (const char *)settings.skipTTBLAnim;
    json["playALsnd"] = (const char *)settings.playALsnd;

    #ifdef FC_HAVEMQTT
    json["useMQTT"] = (const char *)settings.useMQTT;
    json["mqttServer"] = (const char *)settings.mqttServer;
    json["mqttUser"] = (const char *)settings.mqttUser;
    #endif

    json["shuffle"] = (const char *)settings.shuffle;
    
    json["CfgOnSD"] = (const char *)settings.CfgOnSD;
    //json["sdFreq"] = (const char *)settings.sdFreq;

    writeJSONCfgFile(json, cfgName, FlashROMode, funcName);
}

bool checkConfigExists()
{
    return FlashROMode ? SD.exists(cfgName) : (haveFS && SPIFFS.exists(cfgName));
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

    i = atoi(text);

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

static bool openCfgFileRead(const char *fn, File& f, bool SDonly = false)
{
    bool haveConfigFile = false;
    
    if(configOnSD || SDonly) {
        if(SD.exists(fn)) {
            haveConfigFile = (f = SD.open(fn, "r"));
        }
    } 
    if(!haveConfigFile && !SDonly && haveFS) {
        if(SPIFFS.exists(fn)) {
            haveConfigFile = (f = SPIFFS.open(fn, "r"));
        }
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
        #ifdef FC_DBG
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        #endif
        return false;
    }

    #ifdef FC_DBG
    Serial.printf("%s: Loading from %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    if(openCfgFileRead(volCfgName, configFile)) {
        StaticJsonDocument<512> json;
        //if(!deserializeJson(json, configFile)) {
        if(!readJSONCfgFile(json, configFile, funcName)) {
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

    sprintf(buf, "%d", curSoftVol);
    json["volume"] = (const char *)buf;

    if(writeJSONCfgFile(json, volCfgName, configOnSD, funcName)) {
        prevSavedVol = curSoftVol;
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
        #ifdef FC_DBG
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        #endif
        return false;
    }

    #ifdef FC_DBG
    Serial.printf("%s: Loading from %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    if(openCfgFileRead(spdCfgName, configFile)) {
        StaticJsonDocument<512> json;
        //if(!deserializeJson(json, configFile)) {
        if(!readJSONCfgFile(json, configFile, funcName)) {
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

    sprintf(buf, "%d", lastIRspeed);
    json["speed"] = (const char *)buf;

    if(writeJSONCfgFile(json, spdCfgName, configOnSD, funcName)) {
        prevSavedSpd = lastIRspeed;
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
        #ifdef FC_DBG
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        #endif
        return false;
    }

    #ifdef FC_DBG
    Serial.printf("%s: Loading from %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    if(openCfgFileRead(bllCfgName, configFile)) {
        StaticJsonDocument<512> json;
        //if(!deserializeJson(json, configFile)) {
        if(!readJSONCfgFile(json, configFile, funcName)) {
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

    sprintf(buf, "%d", minBLL);
    json["mbll"] = (const char *)buf;

    if(writeJSONCfgFile(json, bllCfgName, configOnSD, funcName)) {
        prevSavedBLL = minBLL;
    }
}

/*
 *  Load/save the idle pattern (SD only)
 */

bool loadIdlePat()
{
    const char *funcName = "loadIdlePat";
    char temp[6];
    File configFile;

    if(!haveSD) {
        #ifdef FC_DBG
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        #endif
        return false;
    }

    if(openCfgFileRead(ipaCfgName, configFile, true)) {
        StaticJsonDocument<512> json;
        //if(!deserializeJson(json, configFile)) {
        if(!readJSONCfgFile(json, configFile, funcName)) {
            if(!CopyCheckValidNumParm(json["pattern"], temp, sizeof(temp), 0, 9, 0)) {
                fluxPat = (uint8_t)atoi(temp);
            }
        } 
        configFile.close();
    }

    // Do not write a default file, use pre-set value

    prevSavedIM = fluxPat;

    return true;
}

void saveIdlePat(bool useCache)
{
    const char *funcName = "saveIdlePat";
    char buf[6];
    StaticJsonDocument<512> json;

    if(useCache && (prevSavedIM == fluxPat)) {
        #ifdef FC_DBG
        Serial.printf("%s: Prev. saved IM identical, not writing\n", funcName);
        #endif
        return;
    }

    if(!haveSD) {
        #ifdef FC_DBG
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        #endif
        return;
    }

    sprintf(buf, "%d", fluxPat);
    json["pattern"] = (const char *)buf;

    if(writeJSONCfgFile(json, ipaCfgName, true, funcName)) {
        prevSavedIM = fluxPat;
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
        #ifdef FC_DBG
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        #endif
        return false;
    }

    #ifdef FC_DBG
    Serial.printf("%s: Loading from %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    if(openCfgFileRead(irlCfgName, configFile)) {
        StaticJsonDocument<512> json;
        //if(!deserializeJson(json, configFile)) {
        if(!readJSONCfgFile(json, configFile, funcName)) {
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
    StaticJsonDocument<512> json;

    if(useCache && (prevSavedIRL == irLocked)) {
        #ifdef FC_DBG
        Serial.printf("%s: Prev. saved irLock identical, not writing\n", funcName);
        #endif
        return;
    }

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    sprintf(buf, "%d", irLocked ? 1 : 0);
    json["lock"] = (const char *)buf;

    if(writeJSONCfgFile(json, irlCfgName, configOnSD, funcName)) {
        prevSavedIRL = irLocked;
    }
}

/*
 * Load custom IR config
 */

static bool loadIRkeysFromFile(File configFile, int index)
{
    uint32_t ir_keys[NUM_IR_KEYS];
    StaticJsonDocument<1024> json;

    //DeserializationError err = deserializeJson(json, configFile);
    DeserializationError err = readJSONCfgFile(json, configFile, "loadIRkeysFromFile");
    
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
                #ifdef FC_DBG
                Serial.printf("%s not found on SD card\n", irUCfgName);
                #endif
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
    uint32_t ir_keys[NUM_IR_KEYS];
    char buf[12];

    if(!haveFS && !configOnSD)
        return true;

    copyIRarray(ir_keys, 1);

    for(int i = 0; i < NUM_IR_KEYS; i++) {
        sprintf(buf, "0x%08x", ir_keys[i]);
        json[(const char *)jsonNames[i]] = buf;    // no const cast, needs to be copied
    }

    writeJSONCfgFile(json, irCfgName, configOnSD, "saveIRKeys");
    
    return true;
}

void deleteIRKeys()
{
    if(configOnSD) {
        SD.remove(irCfgName);
    } else if(haveFS) {
        SPIFFS.remove(irCfgName);
    }
}

/* Copy secondary settings from/to SD if user
 * changed "save to SD"-option in CP
 */

void copySettings()
{
    if(!haveSD || !haveFS)
        return;

    configOnSD = !configOnSD;

    if(configOnSD || !FlashROMode) {
        #ifdef FC_DBG
        Serial.println(F("copySettings: Copying secondary settings to other medium"));
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
            //if(!deserializeJson(json, configFile)) {
            if(!readJSONCfgFile(json, configFile, "loadMusFoldNum")) {
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
    json["folder"] = (const char *)buf;

    writeJSONCfgFile(json, musCfgName, true, funcName);
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
            //DeserializationError error = deserializeJson(json, configFile);
            DeserializationError error = readJSONCfgFile(json, configFile, "loadIpSettings");

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
    
    json["IpAddress"] = (const char *)ipsettings.ip;
    json["Gateway"]   = (const char *)ipsettings.gateway;
    json["Netmask"]   = (const char *)ipsettings.netmask;
    json["DNS"]       = (const char *)ipsettings.dns;

    writeJSONCfgFile(json, ipCfgName, FlashROMode, "writeIpSettings");
}

void deleteIpSettings()
{
    #ifdef FC_DBG
    Serial.println(F("deleteIpSettings: Deleting ip config"));
    #endif

    if(FlashROMode) {
        SD.remove(ipCfgName);
    } else if(haveFS) {
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
      //9404, 7523, 5642, 6582, 6582,         // 0-4
      //7836, 8463, 8463, 5015, 8777,         // 5-9
      //5955,                                 // dot
      7313, 8045, 7313, 7679, 7679,         // 0-4
      8045, 8045, 8045, 7679, 8045,         // 5-9
      7679,                                 // dot
      712515,                               // flux (loop) 
      57259,                                // startup
      46392, SND_KEY_LEN,                   // timetravel, travelstart
      65230,                                // alarm
      60342, // 36989,                      // fluxing
      40593, // 43153,                      // renaming
      32548  // 42212                       // installing (not copied)
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
        rewriteSecondarySettings(); // Re-write secondary settings
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

    unmount_fs();
    delay(500);
    
    esp_restart();
}


bool copy_audio_files()
{
    int i, haveErr = 0;

    if(!allowCPA) {
        return false;
    }

    for(i = 0; i < NUM_AUDIOFILES - 1; i++) {
        open_and_copy(audioFiles[i], haveErr);
        if(haveErr) break;
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
    }

    return true;
}

bool audio_files_present()
{
    File file;
    size_t ts;
    
    if(FlashROMode || !haveFS)
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

static void formatFlashFS()
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

static DeserializationError readJSONCfgFile(JsonDocument& json, File& configFile, const char *funcName)
{
    char *buf = NULL;
    size_t bufSize = configFile.size();
    DeserializationError ret;

    if(!(buf = (char *)malloc(bufSize + 1))) {
        Serial.printf("%s: Buffer allocation failed (%d)\n", funcName, bufSize);
        return DeserializationError::NoMemory;
    }

    memset(buf, 0, bufSize + 1);

    configFile.read((uint8_t *)buf, bufSize);

    #ifdef FC_DBG
    Serial.println((const char *)buf);
    #endif
    
    ret = deserializeJson(json, buf);

    free(buf);

    return ret;
}

static bool writeJSONCfgFile(const JsonDocument& json, const char *fn, bool useSD, const char *funcName)
{
    char *buf;
    size_t bufSize = measureJson(json);
    bool success = false;

    if(!(buf = (char *)malloc(bufSize + 1))) {
        Serial.printf("%s: Buffer allocation failed (%d)\n", funcName, bufSize);
        return false;
    }

    memset(buf, 0, bufSize + 1);
    serializeJson(json, buf, bufSize);

    #ifdef FC_DBG
    Serial.printf("Writing %s to %s\n", fn, useSD ? "SD" : "FS");
    Serial.println((const char *)buf);
    #endif

    if(useSD) {
        success = writeFileToSD(fn, (uint8_t *)buf, (int)bufSize);
    } else {
        success = writeFileToFS(fn, (uint8_t *)buf, (int)bufSize);
    }

    free(buf);

    if(!success) {
        Serial.printf("%s: %s\n", funcName, failFileWrite);
    }

    return success;
}

static bool writeFileToSD(const char *fn, uint8_t *buf, int len)
{
    size_t bytesw;
    
    if(!haveSD)
        return false;

    File myFile = SD.open(fn, FILE_WRITE);
    if(myFile) {
        bytesw = myFile.write(buf, len);
        myFile.close();
        return (bytesw == len);
    } else
        return false;
}

static bool writeFileToFS(const char *fn, uint8_t *buf, int len)
{
    size_t bytesw;
    
    if(!haveFS)
        return false;

    File myFile = SPIFFS.open(fn, FILE_WRITE);
    if(myFile) {
        bytesw = myFile.write(buf, len);
        myFile.close();
        return (bytesw == len);
    } else
        return false;
}
