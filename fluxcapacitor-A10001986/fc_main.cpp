/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Flux Capacitor
 * (C) 2023-2024 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Flux-Capacitor
 * https://fc.out-a-ti.me
 *
 * Main controller
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

#include "fc_global.h"

#include <Arduino.h>
#include <WiFi.h>
#include "fcdisplay.h"
#include "input.h"

#include "fc_main.h"
#include "fc_settings.h"
#include "fc_audio.h"
#include "fc_wifi.h"

unsigned long powerupMillis = 0;

// CenterLED PWM properties
#define CLED_FREQ     5000
#define CLED_CHANNEL  0
#define CLED_RES      8

// BoxLED PWM properties
#define BLED_FREQ     5000
#define BLED_CHANNEL  1
#define BLED_RES      8

// Speed pot
static bool useSKnob = false;
static unsigned long startSpdPot = 0;
#define SPD_SMOOTH_SIZE 4
static int rawSpd[SPD_SMOOTH_SIZE];
static int rawSpdIdx = 0;
static int anaReadCount = 0;
static long prev_avg, prev_raw, prev_raw2;
// Resolution for speed pot, 9-12 allowed
#define POT_RESOLUTION 9
#define POT_GRAN       45
static const uint16_t potSpeeds[POT_GRAN] = {
      3,   3,   3,   4,   5,   6,   7,   8,   9,  10,
     11,  12,  13,  14,  15,  16,  17,  18,  19,  20, 
     25,  30,  35,  40,  45,  50,  55,  60,  65,  70, 
     80,  90, 100, 110, 120, 140, 160, 180, 200, 250,
    300, 350, 400, 450, 500
};

// The IR-remote object
static IRRemote ir_remote(0, IRREMOTE_PIN);
static uint8_t IRFeedBackPin = IR_FB_PIN;

// The center LED object
static PWMLED centerLED(LED_PWM_PIN);

// The Box LED object
static PWMLED boxLED(BLED_PWM_PIN);
static bool PLforBL = false;
uint16_t minBLL = 0;
const uint8_t mbllArray[5] = { 0, 1, 3, 8, 12 };

// The FC LEDs object
FCLEDs fcLEDs(1, SHIFT_CLK_PIN, REG_CLK_PIN, SERDATA_PIN, MRESET_PIN);

// The tt button / TCD tt trigger
static FCButton TTKey = FCButton(TT_IN_PIN,
    false,    // Button/Trigger is active HIGH
    false,    // Disable internal pull-up resistor
    true      // Enable internal pull-down resistor
);

#define TT_DEBOUNCE    50    // tt button debounce time in ms
#define TT_PRESS_TIME 200    // tt button will register a short press
#define TT_HOLD_TIME 5000    // time in ms holding the tt button will count as a long press
static bool isTTKeyPressed = false;
static bool isTTKeyHeld = false;

bool networkTimeTravel = false;
bool networkTCDTT      = false;
bool networkReentry    = false;
bool networkAbort      = false;
bool networkAlarm      = false;
uint16_t networkLead   = ETTO_LEAD;

static bool useGPSS     = false;
static bool usingGPSS   = false;
static int16_t gpsSpeed       = -1;
static int16_t oldGpsSpeed    = -2;
static unsigned long lastGPSchange = 0;
static bool spdIsRotEnc = false;

static bool useNM = false;
static bool tcdNM = false;
bool        fluxNM = false;
static bool useFPO = false;
static bool tcdFPO = false;

static bool bttfnTT = true;

static bool skipttblanim = false;

#define FLUXM2_SECS  30
#define FLUXM3_SECS  60
int                  playFLUX = 1;
static bool          fluxTimer = false;
static unsigned long fluxTimerNow = 0;
static unsigned long fluxTimeout = FLUXM2_SECS * 1000;

uint8_t fluxPat = 0;

static bool          playTTsounds = true;

// Time travel status flags etc.
bool                 TTrunning = false;  // TT sequence is running
static bool          extTT = false;      // TT was triggered by TCD
static unsigned long TTstart = 0;
static unsigned long P0duration = ETTO_LEAD;
static bool          TTP0 = false;
static bool          TTP1 = false;
static bool          TTP2 = false;
static bool          TTP1snd = false;
static unsigned long TTFInt = 0;
static unsigned long TTfUpdNow = 0;
static int           TTSSpd = 0;
static unsigned long TTbUpdNow = 0;
static unsigned long TTcUpdNow = 0;
static bool cDone = false, bDone = false, fDone = false;
static int           bP1idx = 0;
static const int16_t bP1Seq[] = { 
        0, 255,
       30, 0,
      120, 255,
      140, 0,
      200, 255,
      230, 0,
      380, 255,
      420, 0,
      510, 255,
      560, 0,
      650, 255,
      700, 0,
     1500, 0,
       -1, 0
};

// Durations of tt phases for internal tt
#define P0_DUR          5000    // acceleration phase
#define P1_DUR          5000    // time tunnel phase
#define P2_DUR          3000    // re-entry phase (unused)
#define TT_SNDLAT        400    // DO NOT CHANGE (latency for sound/mp3)

bool         TCDconnected = false;
static bool  noETTOLead = false;

static bool          volchanged = false;
static unsigned long volchgnow = 0;
static bool          spdchanged = false;
static unsigned long spdchgnow = 0;
static bool          bllchanged = false;
static unsigned long bllchgnow = 0;
static bool          ipachanged = false;
static unsigned long ipachgnow = 0;
static bool          irlchanged = false;
static unsigned long irlchgnow = 0;

static unsigned long ssLastActivity = 0;
static unsigned long ssDelay = 0;
static unsigned long ssOrigDelay = 0;
static bool          ssActive = false;

static bool          nmOld = false;
static bool          fpoOld = false;
bool                 FPBUnitIsOn = true;

/*
 * Leave first two columns at 0 here, those will be filled
 * by a user-provided ir_keys.txt file on the SD card, and 
 * learned keys respectively.
 */
#define NUM_REM_TYPES 3
static uint32_t remote_codes[NUM_IR_KEYS][NUM_REM_TYPES] = {
//    U  L  Default     (U = user provided via ir_keys.txt; L = Learned)
    { 0, 0, 0x97483bfb },    // 0:  0
    { 0, 0, 0xe318261b },    // 1:  1
    { 0, 0, 0x00511dbb },    // 2:  2
    { 0, 0, 0xee886d7f },    // 3:  3
    { 0, 0, 0x52a3d41f },    // 4:  4
    { 0, 0, 0xd7e84b1b },    // 5:  5
    { 0, 0, 0x20fe4dbb },    // 6:  6
    { 0, 0, 0xf076c13b },    // 7:  7
    { 0, 0, 0xa3c8eddb },    // 8:  8
    { 0, 0, 0xe5cfbd7f },    // 9:  9
    { 0, 0, 0xc101e57b },    // 10: *
    { 0, 0, 0xf0c41643 },    // 11: #
    { 0, 0, 0x3d9ae3f7 },    // 12: arrow up
    { 0, 0, 0x1bc0157b },    // 13: arrow down
    { 0, 0, 0x8c22657b },    // 14: arrow left
    { 0, 0, 0x0449e79f },    // 15: arrow right
    { 0, 0, 0x488f3cbb }     // 16: OK/Enter
};

#define INPUTLEN_MAX 6
static char          inputBuffer[INPUTLEN_MAX + 2];
static int           inputIndex = 0;
static bool          inputRecord = false;
static unsigned long lastKeyPressed = 0;
static int           maxIRctrls = NUM_REM_TYPES;

#define IR_FEEDBACK_DUR 300
static bool          irFeedBack = false;
static unsigned long irFeedBackNow = 0;

bool                 irLocked = false;
static bool          noIR = false;      // for temporary disabling IR reception

bool                 IRLearning = false;
static uint32_t      backupIRcodes[NUM_IR_KEYS];
static int           IRLearnIndex = 0;
static unsigned long IRLearnNow;
static unsigned long IRFBLearnNow;
static bool          IRLearnBlink = false;

uint16_t lastIRspeed = FC_SPD_IDLE;

// BTTF network
#define BTTFN_VERSION              1
#define BTTF_PACKET_SIZE          48
#define BTTF_DEFAULT_LOCAL_PORT 1338
#define BTTFN_NOT_PREPARE  1
#define BTTFN_NOT_TT       2
#define BTTFN_NOT_REENTRY  3
#define BTTFN_NOT_ABORT_TT 4
#define BTTFN_NOT_ALARM    5
#define BTTFN_NOT_REFILL   6
#define BTTFN_NOT_FLUX_CMD 7
#define BTTFN_NOT_SID_CMD  8
#define BTTFN_NOT_PCG_CMD  9
#define BTTFN_NOT_WAKEUP   10
#define BTTFN_NOT_AUX_CMD  11
#define BTTFN_NOT_VSR_CMD  12
#define BTTFN_TYPE_ANY     0    // Any, unknown or no device
#define BTTFN_TYPE_FLUX    1    // Flux Capacitor
#define BTTFN_TYPE_SID     2    // SID
#define BTTFN_TYPE_PCG     3    // Plutonium gauge panel
#define BTTFN_TYPE_VSR     4    // VSR
#define BTTFN_TYPE_AUX     5    // Aux (user custom device)
static const uint8_t BTTFUDPHD[4] = { 'B', 'T', 'T', 'F' };
static bool          useBTTFN = false;
static WiFiUDP       bttfUDP;
static UDP*          fcUDP;
static byte          BTTFUDPBuf[BTTF_PACKET_SIZE];
static unsigned long BTTFNUpdateNow = 0;
static unsigned long BTFNTSAge = 0;
static unsigned long BTTFNTSRQAge = 0;
static bool          BTTFNPacketDue = false;
static bool          BTTFNWiFiUp = false;
static uint8_t       BTTFNfailCount = 0;
static uint32_t      BTTFUDPID = 0;
static unsigned long lastBTTFNpacket = 0;
static bool          BTTFNBootTO = false;
static bool          haveTCDIP = false;
static IPAddress     bttfnTcdIP;
#ifdef BTTFN_MC
static uint32_t      tcdHostNameHash = 0;
#endif  

static int      iCmdIdx = 0;
static int      oCmdIdx = 0;
static uint32_t commandQueue[16] = { 0 };

// Forward declarations ------

static void startIRLearn();
static void endIRLearn(bool restore);
static void handleIRinput();
static void handleIRKey(int command);
static void handleRemoteCommand();
static bool execute(bool isIR);
static void startIRfeedback();
static void endIRfeedback();

static uint16_t getRawSpeed();
static void     setPotSpeed();

static void timeTravel(bool TCDtriggered, uint16_t P0Dur);
static int convertGPSSpeed(int16_t spd);

static void ttkeyScan();
static void TTKeyPressed();
static void TTKeyHeld();

static void ssStart();
static void ssEnd(bool doSound = true);
static void ssRestartTimer();

static bool contFlux();

static void waitAudioDone(bool withIR);

static void bttfn_setup();
static void BTTFNCheckPacket();
static bool BTTFNTriggerUpdate();
static void BTTFNSendPacket();
static bool BTTFNTriggerTT();

void main_boot()
{
    // Boot center LED here (is for some reason on after reset)
    #ifdef FC_DBG
    Serial.println(F("Booting Center LED"));
    #endif
    centerLED.begin(CLED_CHANNEL, CLED_FREQ, CLED_RES);

    // Boot FC leds here to have a way to show the user whats going on
    #ifdef FC_DBG
    Serial.println(F("Booting FC LEDs"));
    #endif
    fcLEDs.begin();
}

void main_setup()
{
    Serial.println(F("Flux Capacitor version " FC_VERSION " " FC_VERSION_EXTRA));

    // Load settings
    loadCurSpeed();
    loadBLLevel();
    loadIdlePat();
    loadIRLock();

    // Set up options to play/mute sounds
    playFLUX = atoi(settings.playFLUXsnd);
    playTTsounds = (atoi(settings.playTTsnds) > 0);

    // Other options
    ssDelay = ssOrigDelay = atoi(settings.ssTimer) * 60 * 1000;    
    useGPSS = (atoi(settings.useGPSS) > 0);
    useNM = (atoi(settings.useNM) > 0);
    useFPO = (atoi(settings.useFPO) > 0);
    bttfnTT = (atoi(settings.bttfnTT) > 0);
    
    skipttblanim = (atoi(settings.skipTTBLAnim) > 0);

    // Option to disable supplied default IR remote
    if((atoi(settings.disDIR) > 0)) 
        maxIRctrls--;

    // Initialize flux sound modes
    if(playFLUX >= 3) {
        playFLUX = 3;
        fluxTimeout = FLUXM3_SECS*1000;
    } else if(playFLUX == 2) 
        fluxTimeout = FLUXM2_SECS*1000;
    
    // [formerly started CP here]

    // Swap "box light" <> "GPIO14"
    PLforBL = (atoi(settings.usePLforBL) > 0);
    // As long as we "abuse" the GPIO14 for the IR feedback,
    // swap it for box light as well
    #if IR_FB_PIN == GPIO_14
    IRFeedBackPin = PLforBL ? BLED_PWM_PIN : GPIO_14;
    #endif

    // Determine if Time Circuits Display is connected
    // via wire, and is source of GPIO tt trigger
    TCDconnected = (atoi(settings.TCDpresent) > 0);
    noETTOLead = (atoi(settings.noETTOLead) > 0);

    // Init IR feedback LED
    pinMode(IRFeedBackPin, OUTPUT);
    digitalWrite(IRFeedBackPin, LOW);

    // Boot remaining display LEDs (but keep them dark)
    #ifdef FC_DBG
    Serial.println(F("Booting Box LED"));
    #endif
    boxLED.begin(BLED_CHANNEL, BLED_FREQ, BLED_RES, PLforBL ? GPIO_14 : 255);

    // Make sure center LED is off
    centerLED.setDC(0);

    // Set up TT button / TCD trigger
    TTKey.attachPress(TTKeyPressed);
    if(!TCDconnected) {
        // If we have a physical button, we need
        // reasonable values for debounce and press
        TTKey.setTiming(TT_DEBOUNCE, TT_PRESS_TIME, TT_HOLD_TIME);
        TTKey.attachLongPressStart(TTKeyHeld);
    } else {
        // If the TCD is connected, we can go more to the edge
        TTKey.setTiming(5, 50, 100000);
        // Long press ignored when TCD is connected
        // IRLearning only possible if "TCD connected by wire" unset.
    }

    // Power-up use of speed pot
    useSKnob = (atoi(settings.useSknob) > 0);
    
    // Set resolution for speed pot
    analogReadResolution(POT_RESOLUTION);
    analogSetWidth(POT_RESOLUTION);

    // Invoke audio file installer if SD content qualifies
    #ifdef FC_DBG
    Serial.println(F("Probing for audio data on SD"));
    #endif
    if(check_allow_CPA()) {
        showWaitSequence();
        if(prepareCopyAudioFiles()) {
            play_file("/_installing.mp3", PA_ALLOWSD, 1.0);
            waitAudioDone(false);
        }
        doCopyAudioFiles();
        // We never return here. The ESP is rebooted.
    }

    #ifdef FC_DBG
    Serial.println(F("Booting IR Receiver"));
    #endif
    ir_remote.begin();

    if(!haveAudioFiles) {
        #ifdef FC_DBG
        Serial.println(F("Current audio data not installed"));
        #endif
        fcLEDs.SpecialSignal(FCSEQ_NOAUDIO);
        while(!fcLEDs.SpecialDone()) {
            mydelay(100, false);
        }
    }

    fcLEDs.stop(true);
    fcLEDs.setSequence(fluxPat);

    // Set FCLeds to default/saved speed
    if(useSKnob) {
        setPotSpeed();
    } else {
        fcLEDs.setSpeed(lastIRspeed);
    }

    // Initialize BTTF network
    bttfn_setup();
    bttfn_loop();

    // If "Follow TCD fake power" is set,
    // stay silent and dark

    if(useBTTFN && useFPO && (WiFi.status() == WL_CONNECTED)) {

        FPBUnitIsOn = false;
        tcdFPO = fpoOld = true;
            
        fcLEDs.off();
        boxLED.setDC(0);
        centerLED.setDC(0);

        // Light up IR feedback for 500ms
        startIRfeedback();
        mydelay(500, false);
        endIRfeedback();

        Serial.println("Waiting for TCD fake power on");

    } else {

        // Otherwise boot:
        FPBUnitIsOn = true;
        
        fcLEDs.on();
    
        // Set minimum box light level
        boxLED.setDC(mbllArray[minBLL]);
    
        // Play startup
        play_file("/startup.mp3", PA_INTRMUS|PA_ALLOWSD, 1.0);
        if(playFLUX) {
            append_flux();
        }
        fcLEDs.SpecialSignal(FCSEQ_STARTUP);
        fcLEDs.stop(false);
        while(!fcLEDs.SpecialDone()) {
             mydelay(20, false);
        }

        ssRestartTimer();

    }
    
    #ifdef FC_DBG
    Serial.println(F("main_setup() done"));
    #endif

    // Delete previous IR input, start fresh
    ir_remote.resume();
}


void main_loop()
{
    unsigned long now = millis();

    // Follow TCD fake power
    if(useFPO && (tcdFPO != fpoOld)) {
        if(tcdFPO) {
            // Power off:
            FPBUnitIsOn = false;
            
            if(TTrunning) {
                fcLEDs.setSpeed(TTSSpd);
            }
            TTrunning = false;
            noIR = false;
            
            mp_stop();
            stopAudio();
            fluxTimer = false;

            if(irFeedBack) {
                endIRfeedback();
                irFeedBack = false;
            }
            
            if(IRLearning) {
                endIRLearn(true); // Turns LEDs on
            }
            
            fcLEDs.off();
            boxLED.setDC(0);
            centerLED.setDC(0);

            flushDelayedSave();
            
        } else {
            // Power on: 
            FPBUnitIsOn = true;
            
            fcLEDs.on();
            boxLED.setDC(mbllArray[minBLL]);

            // Play startup
            play_file("/startup.mp3", PA_INTRMUS|PA_ALLOWSD, 1.0);
            if(playFLUX) {
                append_flux();
            }
            fcLEDs.SpecialSignal(FCSEQ_STARTUP);
            fcLEDs.stop(false);
            while(!fcLEDs.SpecialDone()) {
                 mydelay(20, false);
            }

            isTTKeyHeld = isTTKeyPressed = false;
            networkTimeTravel = false;

            ssRestartTimer();
            ssActive = false;

            ir_remote.loop();
 
        }
        fpoOld = tcdFPO;
    }
    
    // Discard (incomlete) input from IR after 30 seconds of inactivity
    if(now - lastKeyPressed >= 30*1000) {
        inputBuffer[0] = '\0';
        inputIndex = 0;
        inputRecord = false;
    }

    // IR feedback timeout
    if(irFeedBack && now - irFeedBackNow > IR_FEEDBACK_DUR) {
        endIRfeedback();
        irFeedBack = false;
    }

    if(IRLearning) {
        ssRestartTimer();
        if(now - IRFBLearnNow > 200) {
            IRLearnBlink = !IRLearnBlink;
            IRLearnBlink ? endIRfeedback() : startIRfeedback();
            IRFBLearnNow = now;
        }
        if(now - IRLearnNow > 10000) {
            endIRLearn(true);
            #ifdef FC_DBG
            Serial.println("main_loop: IR learning timed out");
            #endif
        }
    }

    // IR Remote loop
    if(FPBUnitIsOn) {
        if(!noIR && ir_remote.loop()) {
            handleIRinput();
        }
        handleRemoteCommand();
    }

    // Eval GPS/RotEnc speed
    // We track speed even when off, so we are immediately
    // up to speed when coming back.
    if(useGPSS) {

        if(gpsSpeed >= 0) {

            if(!TTrunning) {

                if(!usingGPSS || (now - lastGPSchange > 200)) {
                
                    uint16_t shouldBe = convertGPSSpeed(gpsSpeed);
                    uint16_t isNow = fcLEDs.getSpeed();
                    uint16_t toSet;
    
                    if(abs(shouldBe - isNow) > 3) {
                        toSet = (shouldBe + isNow) / 2;
                    } else {
                        toSet = shouldBe;
                    }
    
                    if(isNow != toSet) {
                        fcLEDs.setSpeed(toSet);
                    }

                    //Serial.printf("%d %d %d (%d)\n", shouldBe, isNow, toSet, gpsSpeed);

                    lastGPSchange = now;
                }

                usingGPSS = true;
            }

        } else if(usingGPSS) {
          
            usingGPSS = false;
            if(!useSKnob) {
                if(!TTrunning || (TTP2 && fDone)) {
                    fcLEDs.setSpeed(lastIRspeed);
                } else {
                    TTSSpd = lastIRspeed;
                }
            }
        }
    }
    
    // Poll speed pot
    // We track speed even when off, so we are immediately
    // up to speed when coming back.
    if(useSKnob && !usingGPSS) {
        setPotSpeed();
    }

    // TT button evaluation
    if(FPBUnitIsOn && !TTrunning) {
        ttkeyScan();
        if(isTTKeyHeld) {
            ssEnd();
            isTTKeyHeld = isTTKeyPressed = false;
            if(!IRLearning) {
                startIRLearn();
                #ifdef FC_DBG
                Serial.println("main_loop: IR learning started");
                #endif
            }
        } else if(isTTKeyPressed) {
            isTTKeyPressed = false;
            if(!TCDconnected && ssActive) {
                // First button press when ss is active only deactivates SS
                ssEnd();
            } else if(IRLearning) {
                endIRLearn(true);
                #ifdef FC_DBG
                Serial.println("main_loop: IR learning aborted");
                #endif
            } else {
                if(TCDconnected) {
                    ssEnd(false);  // let TT() take care of restarting sound
                }
                if(TCDconnected || !bttfnTT || !BTTFNTriggerTT()) {
                    timeTravel(TCDconnected, noETTOLead ? 0 : ETTO_LEAD);
                }
            }
        }
    
        // Check for BTTFN/MQTT-induced TT
        if(networkTimeTravel) {
            networkTimeTravel = false;
            ssEnd(false);  // let TT() take care of restarting sound
            timeTravel(networkTCDTT, networkLead);
        }
    }

    now = millis();

    // The time travel sequences
    
    if(TTrunning) {

        if(extTT) {

            // ***********************************************************************************
            // TT triggered by TCD (BTTFN, GPIO or MQTT) *****************************************
            // ***********************************************************************************

            if(TTP0) {   // Acceleration - runs for ETTO_LEAD ms by default

                if(!networkAbort && (now - TTstart < P0duration)) {

                    if(TTFInt && (now - TTfUpdNow >= TTFInt)) {
                        int t = fcLEDs.getSpeed();
                        if(t >= 100)      t -= 50;
                        else if(t >= 20)  t -= 10;
                        else if(t > 2)    t--;
                        fcLEDs.setSpeed(t);
                        TTfUpdNow = now;
                    }
                             
                } else {

                    if(fcLEDs.getSpeed() != 2) {
                        fcLEDs.setSpeed(2);
                    }

                    TTP0 = false;
                    TTP1 = true;
                    bP1idx = 0;
                    TTstart = now;
                    noIR = true;
                    if(playTTsounds && !networkAbort) {
                        play_file("/travelstart.mp3", PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL, 1.0);
                    }
                }
            }
            if(TTP1) {   // Peak/"time tunnel" - ends with pin going LOW or BTTFN/MQTT "REENTRY"

                if( (networkTCDTT && (!networkReentry && !networkAbort)) || (!networkTCDTT && digitalRead(TT_IN_PIN))) 
                {

                    int t;

                    if((t = centerLED.getDC()) < 255) {
                        t += 2;
                        if(t > 255) t = 255;
                        centerLED.setDC(t);
                    }

                    if(!skipttblanim) {
                        if(bP1Seq[bP1idx] >= 0) {
                            if(now - TTstart > bP1Seq[bP1idx]) {
                                boxLED.setDC(bP1Seq[bP1idx+1]);
                                bP1idx += 2;
                            }
                            TTbUpdNow = now;
                        } else if(now - TTstart < 4800) {
                            if(now - TTbUpdNow > 20) {
                                t = (esp_random() % 255) & 0b11000111;
                                boxLED.setDC(t);
                                TTbUpdNow = now;
                            }
                        } else if(now - TTstart < 5500) {
                            t = boxLED.getDC();
                            if(t) boxLED.setDC(0);
                        } else if((t = boxLED.getDC()) < 255) {
                            t += 1;
                            if(t > 255) t = 255;
                            boxLED.setDC(t);
                        }
                    } else {
                        if((t = boxLED.getDC()) < 255) {
                            t += 1;
                            if(t > 255) t = 255;
                            boxLED.setDC(t);
                        }
                    }

                    if(fcLEDs.getSpeed() != 2) {
                        fcLEDs.setSpeed(2);
                    }
                    
                } else {

                    TTP1 = false;
                    TTP2 = true;
                    cDone = bDone = fDone = false;
                    TTfUpdNow = TTcUpdNow = TTbUpdNow = now;
                    // For GPS and RotEnc, let normal loop take
                    // care of returning to "current" speed; here
                    // we only switch down one notch.
                    if(usingGPSS && gpsSpeed >= 0) {
                          TTSSpd = 3;
                    }
                    if(playTTsounds) {
                        if(!networkAbort) {
                            play_file("/timetravel.mp3", PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL, 1.0);
                        }
                        if(playFLUX) {
                            append_flux();
                        }
                    }
                }
            }
            if(TTP2) {   // Reentry - up to us

                int t;

                if(!cDone && now - TTcUpdNow > 3) {
                    if((t = centerLED.getDC()) > 0) {
                        t -= 1;
                        if(t < 0) t = 0;
                        centerLED.setDC(t);
                        TTcUpdNow = now;
                    } else {
                        cDone = true;
                        centerLED.setDC(0);
                    }
                }

                if(!bDone && now - TTbUpdNow > 2) {
                    if((t = boxLED.getDC()) > mbllArray[minBLL]) {
                        t -= 1;
                        if(t < 0) t = 0;
                        boxLED.setDC(t);
                        TTbUpdNow = now;
                    } else {
                        bDone = true;
                        boxLED.setDC(mbllArray[minBLL]);
                    }
                }

                if(!fDone && now - TTfUpdNow >= 250) {

                    if((t = fcLEDs.getSpeed()) < TTSSpd) {
                        if(t >= 50)      t += 50;
                        else if(t >= 10) t += 10;
                        else             t++;
                        fcLEDs.setSpeed(t);
                        TTfUpdNow = now;
                    } else {
                        fDone = true;
                        fcLEDs.setSpeed(TTSSpd);
                    }
                }

                if(cDone && bDone && fDone) {

                    // At very end:
                    TTP2 = false;
                    noIR = false;
                    TTrunning = false;
                    isTTKeyHeld = isTTKeyPressed = false;
                    ssRestartTimer();
                    ir_remote.loop();

                }
            }
          
        } else {

            // ****************************************************************************
            // TT triggered by button (if TCD not connected), MQTT or IR ******************
            // ****************************************************************************
          
            if(TTP0) {   // Acceleration - runs for P0_DUR ms

                if(now - TTstart < P0_DUR) {

                    if(TTFInt && (now - TTfUpdNow >= TTFInt)) {
                        int t = fcLEDs.getSpeed();
                        if(t >= 100)      t -= 50;
                        else if(t >= 20)  t -= 10;
                        else if(t > 2)    t--;
                        fcLEDs.setSpeed(t);
                        TTfUpdNow = now;
                    }
                             
                } else {

                    if(fcLEDs.getSpeed() != 2) {
                        fcLEDs.setSpeed(2);
                    }

                    TTP0 = false;
                    TTP1 = true;
                    noIR = true;
                    TTstart = now;
                    bP1idx = 0;
                    if(playTTsounds) {
                        play_file("/travelstart.mp3", PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL, 1.0);
                    }
                }
            }
            
            if(TTP1) {   // Peak/"time tunnel" - runs for P1_DUR ms

                if(now - TTstart < P1_DUR) {

                    int t;

                    if((t = centerLED.getDC()) < 255) {
                        t += 2;
                        if(t > 255) t = 255;
                        centerLED.setDC(t);
                    }

                    if(!skipttblanim) {
                        if(bP1Seq[bP1idx] >= 0) {
                            if(now - TTstart > bP1Seq[bP1idx]) {
                                boxLED.setDC(bP1Seq[bP1idx+1]);
                                bP1idx += 2;
                            }
                            TTbUpdNow = now;
                        } else if(now - TTstart < 4800) {
                            if(now - TTbUpdNow > 20) {
                                t = (esp_random() % 255) & 0b11000111;
                                boxLED.setDC(t);
                                TTbUpdNow = now;
                            }
                        } else if(now - TTstart < 5500) {
                            t = boxLED.getDC();
                            if(t) boxLED.setDC(0);
                        } else if((t = boxLED.getDC()) < 255) {
                            t += 1;
                            if(t > 255) t = 255;
                            boxLED.setDC(t);
                        }
                    } else {
                        if((t = boxLED.getDC()) < 255) {
                            t += 1;
                            if(t > 255) t = 255;
                            boxLED.setDC(t);
                        }
                    }

                    if(fcLEDs.getSpeed() != 2) {
                        fcLEDs.setSpeed(2);
                    }
                    
                } else {

                    boxLED.setDC(255);
                    TTP1 = false;
                    TTP2 = true;
                    cDone = bDone = fDone = false;
                    TTfUpdNow = TTcUpdNow = TTbUpdNow = now;
                    if(playTTsounds) {
                        play_file("/timetravel.mp3", PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL, 1.0);
                        if(playFLUX) {
                            append_flux();
                        }
                    }
                }
            }
            
            if(TTP2) {   // Reentry - up to us
                
                int t;

                if(!cDone && now - TTcUpdNow > 3) {
                    if((t = centerLED.getDC()) > 0) {
                        t -= 1;
                        if(t < 0) t = 0;
                        centerLED.setDC(t);
                        TTcUpdNow = now;
                    } else {
                        cDone = true;
                        centerLED.setDC(0);
                    }
                }

                if(!bDone && now - TTbUpdNow > 2) {
                    if((t = boxLED.getDC()) > mbllArray[minBLL]) {
                        t -= 1;
                        if(t < 0) t = 0;
                        boxLED.setDC(t);
                        TTbUpdNow = now;
                    } else {
                        bDone = true;
                        boxLED.setDC(mbllArray[minBLL]);
                    }
                }

                if(!fDone && now - TTfUpdNow >= 250) {
                    if((t = fcLEDs.getSpeed()) < TTSSpd) {
                        if(t >= 50)      t += 50;
                        else if(t >= 10) t += 10;
                        else             t++;
                        fcLEDs.setSpeed(t);
                        TTfUpdNow = now;
                    } else {
                        fDone = true;
                        fcLEDs.setSpeed(TTSSpd);
                    }
                }

                if(cDone && bDone && fDone) {

                    // At very end:
                    TTP2 = false;
                    noIR = false;
                    TTrunning = false;
                    isTTKeyHeld = isTTKeyPressed = false;
                    ssRestartTimer();
                    ir_remote.loop();

                }
            }
          
        }
    }

    // Follow TCD night mode
    if(useNM && (tcdNM != nmOld)) {
        if(tcdNM) {
            // NM on: Set Screen Saver timeout to 10 seconds
            ssDelay = 10 * 1000;
            fluxNM = true;
        } else {
            // NM off: End Screen Saver; reset timeout to old value
            ssEnd();  // Doesn't do anything if fake power is off
            ssDelay = ssOrigDelay;
            fluxNM = false;
        }
        nmOld = tcdNM;
    }

    // Wake up on GPS/RotEnc speed changes
    if(gpsSpeed != oldGpsSpeed) {
        if(FPBUnitIsOn && !TTrunning && !IRLearning && spdIsRotEnc && gpsSpeed >= 0) {
            wakeup();
        }
        oldGpsSpeed = gpsSpeed;
    }

    now = millis();

    // "Screen saver"
    if(FPBUnitIsOn) {
        if(!TTrunning && !ssActive && ssDelay && (now - ssLastActivity > ssDelay)) {
            ssStart();
        }
    }

    // Flux auto modes
    if(FPBUnitIsOn) {
        if(fluxTimer && (now - fluxTimerNow > fluxTimeout)) {
            if(playingFlux) {
                stopAudio();
            }
            fluxTimer = false;
        }
    }

    // If network is interrupted, return to stand-alone
    if(useBTTFN) {
        if( (lastBTTFNpacket && (now - lastBTTFNpacket > 30*1000)) ||
            (!BTTFNBootTO && !lastBTTFNpacket && (now - powerupMillis > 60*1000)) ) {
            tcdNM = false;
            tcdFPO = false;
            gpsSpeed = -1;
            lastBTTFNpacket = 0;
            BTTFNBootTO = true;
        }
    }

    if(!TTrunning) {
        // Save volume 10 seconds after last change
        if(volchanged && (now - volchgnow > 10000)) {
            volchanged = false;
            saveCurVolume();
        }
    
        // Save speed 10 seconds after last change
        if(spdchanged && (now - spdchgnow > 10000)) {
            spdchanged = false;
            saveCurSpeed();
        }
    
        // Save mbll 10 seconds after last change
        if(bllchanged && (now - bllchgnow > 10000)) {
            bllchanged = false;
            saveBLLevel();
        }
    
        // Save idle pattern 10 seconds after last change
        if(ipachanged && (now - ipachgnow > 10000)) {
            ipachanged = false;
            saveIdlePat();
        }
    
        // Save irlock 10 seconds after last change
        if(irlchanged && (now - irlchgnow > 10000)) {
            irlchanged = false;
            saveIRLock();
        }
    }

    if(networkAlarm && !TTrunning && !IRLearning) {
        networkAlarm = false;
        if(atoi(settings.playALsnd) > 0) {
            play_file("/alarm.mp3", PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL, 1.0);
            if(FPBUnitIsOn && !ssActive) {
                if(playFLUX == 1) {
                    append_flux();
                }
            }
        }
        fcLEDs.SpecialSignal(FCSEQ_ALARM);
    }
}

void flushDelayedSave()
{
    if(volchanged) {
        volchanged = false;
        saveCurVolume();
    }

    if(spdchanged) {
        spdchanged = false;
        saveCurSpeed();
    }

    if(bllchanged) {
        bllchanged = false;
        saveBLLevel();
    }

    if(ipachanged) {
        ipachanged = false;
        saveIdlePat();
    }

    if(irlchanged) {
        irlchanged = false;
        saveIRLock();
    }
}

/*
 * Time travel
 */

static void timeTravel(bool TCDtriggered, uint16_t P0Dur)
{
    int i = 0, tspd;
    
    if(TTrunning || IRLearning)
        return;

    flushDelayedSave();

    if(playTTsounds) {
        if(mp_stop() || !playingFlux) {
           play_flux();
        }
        fluxTimer = false;  // Disable timer for tt phase 0
    }
        
    TTrunning = true;
    TTstart = TTfUpdNow = millis();
    TTP0 = true;   // phase 0
    TTP1 = TTP2 = false;

    TTSSpd = tspd = fcLEDs.getSpeed();

    if(TTSSpd > 50) TTSSpd = TTSSpd / 10 * 10;

    if(TTSSpd != tspd) {
        fcLEDs.setSpeed(TTSSpd);
        tspd = TTSSpd;
    }

    // Calculate number of steps for acceleration
    while(tspd >= 100) {
        tspd -= 50;
        i++;
    }
    while(tspd >= 20) {
        tspd -= 10;
        i++;
    }
    while(tspd > 2) {
        tspd--;
        i++;
    }
    
    if(TCDtriggered) {    // TCD-triggered TT (GPIO, BTTFN, MQTT-pub) (synced with TCD)
        extTT = true;
        P0duration = P0Dur;
        #ifdef FC_DBG
        Serial.printf("P0 duration is %d\n", P0duration);
        #endif
        if(i > 0) {
            TTFInt = P0duration / i;
        } else {
            TTFInt = 0;
        }
    } else {              // button/IR/MQTT-cmd triggered TT (stand-alone)
        extTT = false;
        if(i > 0) {
            TTFInt = P0_DUR / i;
        } else {
            TTFInt = 0;
        }
    }
}

static int convertGPSSpeed(int16_t spd)
{
    // GPS speeds 0-87 translate into fc LED speeds IDLE - 3; 88+ => 3 (2 reserved for tt)
    return (spd >= 88) ? 3 : ((87 - spd) * (FC_SPD_IDLE-3) / 87) + 3;
}

/*
 * IR remote input handling
 */

static void startIRfeedback()
{
    digitalWrite(IRFeedBackPin, HIGH);
}

static void endIRfeedback()
{
    digitalWrite(IRFeedBackPin, LOW);
}

static void backupIR()
{
    for(int i = 0; i < NUM_IR_KEYS; i++) {
        backupIRcodes[i] = remote_codes[i][1];
    }
}

static void restoreIRbackup()
{
    for(int i = 0; i < NUM_IR_KEYS; i++) {
        remote_codes[i][1] = backupIRcodes[i];
    }
}

static void startIRLearn()
{
    fcLEDs.stop(true);
    fcLEDs.off();
    mydelay(15, false);      // wait for ISR to catch up
    fcLEDs.SpecialSignal(FCSEQ_LEARNSTART);
    while(!fcLEDs.SpecialDone()) {
        mydelay(50, true);
    }
    IRLearning = true;
    IRLearnIndex = 0;
    IRLearnNow = IRFBLearnNow = millis();
    IRLearnBlink = false;
    backupIR();
    ir_remote.loop();     // Ignore IR received in the meantime
}

static void endIRLearn(bool restore)
{
    fcLEDs.stop(false);
    fcLEDs.on();
    IRLearning = false;
    endIRfeedback();
    if(restore) {
        restoreIRbackup();
    }
    ir_remote.loop();     // Ignore IR received in the meantime
}

static void handleIRinput()
{
    uint32_t myHash = ir_remote.readHash();
    uint16_t i, j;
    bool done = false;
    
    Serial.printf("handleIRinput: Received IR code 0x%lx\n", myHash);

    if(IRLearning) {
        endIRfeedback();
        remote_codes[IRLearnIndex++][1] = myHash;
        if(IRLearnIndex == NUM_IR_KEYS) {
            fcLEDs.SpecialSignal(FCSEQ_LEARNDONE);
            IRLearning = false;
            saveIRKeys();
            #ifdef FC_DBG
            Serial.println("handleIRinput: All IR keys learned, and saved");
            #endif
        } else {
            fcLEDs.SpecialSignal(FCSEQ_LEARNNEXT);
            #ifdef FC_DBG
            Serial.println("handleIRinput: IR key learned");
            #endif
        }
        while(!fcLEDs.SpecialDone()) {
            mydelay(50, true);
        }
        if(IRLearning) {
            IRLearnNow = millis();
        } else {
            endIRLearn(false);
        }
        return;
    }

    for(i = 0; i < NUM_IR_KEYS; i++) {
        for(j = 0; j < maxIRctrls; j++) {
            if(remote_codes[i][j] == myHash) {
                #ifdef FC_DBG
                Serial.printf("handleIRinput: key %d\n", i);
                #endif
                handleIRKey(i);
                done = true;
                break;
            }
        }
        if(done) break;
    }
}

static void clearInpBuf()
{
    inputIndex = 0;
    inputBuffer[0] = '\0';
    inputRecord = false;
}

static void recordKey(int key)
{
    if(inputIndex < INPUTLEN_MAX) {
        inputBuffer[inputIndex++] = key + '0';
        inputBuffer[inputIndex] = '\0';
    }
}

static uint8_t read2digs(uint8_t idx)
{
    return ((inputBuffer[idx] - '0') * 10) + (inputBuffer[idx+1] - '0');
}

static void doKey1()
{
}
static void doKey2()
{
    if((!TTrunning || !playTTsounds) && haveMusic) {
        mp_prev(mpActive);
    }
}
static void doKey3()
{
    if(!TTrunning) {
        play_file("/key3.mp3", PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL);
        if(contFlux()) {
            append_flux();
        }
    }
}
static void doKey4()
{
}
static void doKey5()
{
    if(haveMusic) {
        if(mpActive) {
            mp_stop();
            if(contFlux()) {
                play_flux();
            }
        } else {
            if(!TTrunning || !playTTsounds) mp_play();
        }
    }
}
static void doKey6()
{
    if(!TTrunning) {
        play_file("/key6.mp3", PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL);
        if(contFlux()) {
            append_flux();
        }
    }
}
static void doKey7()
{
}
static void doKey8()
{
    if((!TTrunning || !playTTsounds) && haveMusic) {
        mp_next(mpActive);
    }
}
static void doKey9()
{
}

static void handleIRKey(int key)
{
    int16_t tempi;
    bool doBadInp = false;
    unsigned long now = millis();

    if(ssActive) {
        if(!irLocked || key == 11) {
            ssEnd();
            return;
        }
    }

    if(!irLocked) {
        ssRestartTimer();
      
        startIRfeedback();
        irFeedBack = true;
        irFeedBackNow = now;
    }
    lastKeyPressed = now;
        
    // If we are in "recording" mode, just record and bail
    if(inputRecord && key >= 0 && key <= 9) {
        recordKey(key);
        return;
    }
    
    switch(key) {
    case 0:                           // 0: time travel
        if(irLocked) return;
        if(!bttfnTT || !BTTFNTriggerTT()) {
            timeTravel(false, ETTO_LEAD);
        }
        break;
    case 1:                           // 1:
        if(irLocked) return;
        doKey1();
        break;
    case 2:                           // 2: MusicPlayer: prev
        if(irLocked) return;
        doKey2();
        break;
    case 3:                           // 3: Play key3.mp3
        if(irLocked) return;
        doKey3();
        break;
    case 4:                           // 4:
        if(irLocked) return;
        doKey4();
        break;
    case 5:                           // 5: MusicPlayer: start/stop
        if(irLocked) return;
        doKey5();
        break;
    case 6:                           // 6: Play key6.mp3
        if(irLocked) return;
        doKey6();
        break;
    case 7:                           // 7:
        if(irLocked) return;
        doKey7();
        break;
    case 8:                           // 8: MusicPlayer: next
        if(irLocked) return;
        doKey8();
        break;
    case 9:                           // 9:
        if(irLocked) return;
        doKey9();
        break;
    case 10:                          // * - start code input
        clearInpBuf();
        inputRecord = true;
        break;
    case 11:                          // # - abort code input
        clearInpBuf();
        break;
    case 12:                          // arrow up: inc vol
        if(irLocked) return;
        if(curSoftVol != 255) {
            inc_vol();        
            volchanged = true;
            volchgnow = millis();
        }
        break;
    case 13:                          // arrow down: dec vol
        if(irLocked) return;
        if(curSoftVol != 255) {
            dec_vol();
            volchanged = true;
            volchgnow = millis();
        }
        break;
    case 14:                          // arrow left: dec LED speed
        if(irLocked) return;
        if(!useSKnob && !TTrunning) {
            tempi = lastIRspeed;
            if(tempi >= 100) tempi = tempi / 10 * 10;
            if(tempi >= 130)     tempi += 20;
            else if(tempi >= 90) tempi += 10;
            else if(tempi >= 15) tempi += 5;
            else if(tempi >= 1)  tempi++;
            if(tempi > FC_SPD_MIN) tempi = FC_SPD_MIN;
            if(!usingGPSS) {
                fcLEDs.setSpeed(tempi);
            }
            lastIRspeed = tempi;
            spdchanged = true;
            spdchgnow = millis();
        }
        break;
    case 15:                          // arrow right: inc LED speed
        if(irLocked) return;
        if(!useSKnob && !TTrunning) {
            tempi = lastIRspeed;
            if(tempi >= 100) tempi = tempi / 10 * 10;
            if(tempi >= 150)      tempi -= 20;
            else if(tempi >= 100) tempi -= 10;
            else if(tempi >= 20)  tempi -= 5;
            else if(tempi > 1)    tempi--;
            if(tempi < FC_SPD_MAX) tempi = FC_SPD_MAX;
            if(!usingGPSS) {
                fcLEDs.setSpeed(tempi);
            }
            lastIRspeed = tempi;
            spdchanged = true;
            spdchgnow = millis();
        }
        break;
    case 16:                          // ENTER: Execute code command
        doBadInp = execute(true);
        break;
    default:
        if(!irLocked) {
            doBadInp = true;
        }
    }

    if(!TTrunning && doBadInp) {
        fcLEDs.SpecialSignal(FCSEQ_BADINP);
    }
}

static void handleRemoteCommand()
{
    uint32_t command = commandQueue[oCmdIdx];

    if(!command)
        return;

    commandQueue[oCmdIdx] = 0;
    oCmdIdx++;
    oCmdIdx &= 0x0f;

    if(ssActive) {
        ssEnd();
    }
    ssRestartTimer();
    lastKeyPressed = millis();

    // Some translation
    if(command < 10) {
      
        // <10 are translated to direct-key-actions.
        switch(command) {
        case 1:
            doKey1();
            break;
        case 2:
            doKey2();
            break;
        case 3:
            doKey3();
            break;
        case 4:
            doKey4();
            break;
        case 5:
            doKey5();
            break;
        case 6:
            doKey6();
            break;
        case 7:
            doKey7();
            break;
        case 8:
            doKey8();
            break;
        case 9:
            doKey9();
            break;
        }

        return;
        
    } else if(command < 100) {
      
        sprintf(inputBuffer, "%02d", command);
        
    } else if(command < 1000) {
      
        sprintf(inputBuffer, "%03d", command);
        
    } else if(command < 100000) {

        sprintf(inputBuffer, "%05d", command);
        
    } else {
      
        sprintf(inputBuffer, "%06d", command);

    }

    execute(false);
}

static bool execute(bool isIR)
{
    bool doBadInp = false;
    bool isIRLocked = isIR ? irLocked : false;
    uint16_t temp;
    unsigned long now = millis();

    switch(strlen(inputBuffer)) {
    case 1:
        /*
        if(!isIRLocked) {
            fluxPat = inputBuffer[0] - '0';       // *0-*9 Set idle pattern (deprecated)
            fcLEDs.setSequence(fluxPat);
            ipachanged = true;
            ipachgnow = now;
        }
        */
        break;
    case 2:
        temp = atoi(inputBuffer);
        if(temp >= 10 && temp <= 19) {            // *10-*19 Set idle pattern
            if(!isIRLocked) {
                fluxPat = atoi(inputBuffer) - 10;
                fcLEDs.setSequence(fluxPat);
                ipachanged = true;
                ipachgnow = now;
            }
        } else {
            switch(temp) {
            /*
            case 0:                               // *00 Disable looped FLUX sound playback
            case 1:                               // *01 Enable looped FLUX sound playback
            case 2:                               // *02 Enable looped FLUX sound playback for 30 seconds
            case 3:                               // *03 Enable looped FLUX sound playback for 60 seconds
                if(!TTrunning && !isIRLocked) {
                    setFluxMode(temp);
                }
                break;
            */
            case 20:                              // *20 Disable looped FLUX sound playback
            case 21:                              // *21 Enable looped FLUX sound playback
            case 22:                              // *22 Enable looped FLUX sound playback for 30 seconds
            case 23:                              // *23 Enable looped FLUX sound playback for 60 seconds
                if(!TTrunning && !isIRLocked) {
                    setFluxMode(temp - 20);
                }
                break;
            case 30:                              // *30 - *34: Set minimum box light level
            case 31:
            case 32:
            case 33:
            case 34:
                if(!TTrunning && !isIRLocked) {
                    minBLL = temp - 30;
                    boxLED.setDC(mbllArray[minBLL]);
                    bllchanged = true;
                    bllchgnow = now;
                }
                break;
            case 40:                              // *40 set default speed
                if(!TTrunning && !isIRLocked) {
                    if(!useSKnob) {
                        if(!usingGPSS) {
                            fcLEDs.setSpeed(FC_SPD_IDLE);
                        }
                        lastIRspeed = FC_SPD_IDLE;
                        spdchanged = true;
                        spdchgnow = now;
                    }
                }
                break;
            case 70:                              // *70 lock/unlock ir
                irLocked = !irLocked;
                irlchanged = true;
                irlchgnow = now;
                if(!TTrunning && !irLocked) {
                    startIRfeedback();
                    irFeedBack = true;
                    irFeedBackNow = now;
                }
                break;
            case 71:
                // Taken by SID IR lock sequence
                break;
            case 81:                              // *81 Toggle knob use for LED speed
                if(!TTrunning && !isIRLocked) {
                    useSKnob = !useSKnob;
                    if(!useSKnob && !usingGPSS) {
                        fcLEDs.setSpeed(lastIRspeed);
                    }
                }
                break;
            case 85:
                if(!TTrunning && !isIRLocked) {
                    play_file("/fluxing.mp3", PA_INTRMUS, 1.0);
                    if(contFlux()) {
                        append_flux();
                    }
                }
                break;
            case 90:                              // *90 say IP address
                if(!TTrunning && !isIRLocked) {
                    uint8_t a, b, c, d;
                    bool wasActiveM = false, wasActiveF = false;
                    char ipbuf[16];
                    char numfname[8] = "/x.mp3";
                    if(haveMusic && mpActive) {
                        mp_stop();
                        wasActiveM = true;
                    } else if(playingFlux) {
                        wasActiveF = true;
                    }
                    stopAudio();
                    flushDelayedSave();
                    wifi_getIP(a, b, c, d);
                    sprintf(ipbuf, "%d.%d.%d.%d", a, b, c, d);
                    numfname[1] = ipbuf[0];
                    play_file(numfname, PA_INTRMUS|PA_ALLOWSD);
                    for(int i = 1; i < strlen(ipbuf); i++) {
                        if(ipbuf[i] == '.') {
                            append_file("/dot.mp3", PA_INTRMUS|PA_ALLOWSD);
                        } else {
                            numfname[1] = ipbuf[i];
                            append_file(numfname, PA_INTRMUS|PA_ALLOWSD);
                        }
                        while(append_pending()) {
                            mydelay(10, false);
                        }
                    }
                    waitAudioDone(false);
                    if(wasActiveF && contFlux()) play_flux();
                    else if(wasActiveM)          mp_play();
                    ir_remote.loop(); // Flush IR afterwards
                }
                break;
            default:                              // *50 - *59 Set music folder number
                if(!TTrunning && !isIRLocked) {
                    if(inputBuffer[0] == '5' && haveSD) {
                        switchMusicFolder((uint8_t)inputBuffer[1] - '0');
                    } else {
                        doBadInp = true;
                    }
                }
            }
        }
        break;
    case 3:
        if(!isIRLocked) {
            temp = atoi(inputBuffer);
            if(temp >= 300 && temp <= 399) {
                temp -= 300;                              // 300-319/399: Set fixed volume level / enable knob
                if(temp == 99) {
                    curSoftVol = 255;
                    volchanged = true;
                    volchgnow = millis();
                } else if(temp <= 19) {
                    curSoftVol = temp;
                    volchanged = true;
                    volchgnow = millis();
                } else {
                    doBadInp = true;
                }
            } else {
                if(!TTrunning) {
                    switch(temp) {
                    /*
                    case 0:                               // *000 Disable looped FLUX sound playback
                    case 1:                               // *001 Enable looped FLUX sound playback
                    case 2:                               // *002 Enable looped FLUX sound playback for 30 seconds
                    case 3:                               // *003 Enable looped FLUX sound playback for 60 seconds
                        setFluxMode(temp);
                        break;
                    */
                    case 222:                             // *222/*555 Disable/enable shuffle
                    case 555:
                        if(haveMusic) {
                            mp_makeShuffle((temp == 555));
                        }
                        break;
                    case 888:                             // *888 go to song #0
                        if(haveMusic) {
                            mp_gotonum(0, mpActive);
                        }
                        break;
                    default:
                        doBadInp = true;
                    }
                }
            }
        }
        break;
    case 5:
        if(!isIRLocked) {
            if(!strcmp(inputBuffer, "64738")) {
                allOff();
                endIRfeedback();
                mp_stop();
                stopAudio();
                flushDelayedSave();
                unmount_fs();
                delay(500);
                esp_restart();
            }
            doBadInp = true;
        }
        break;
    case 6:
        if(!isIRLocked) {
            if(!TTrunning) {                          // *888xxx go to song #xxx
                if(haveMusic && !strncmp(inputBuffer, "888", 3)) {
                    uint16_t num = ((inputBuffer[3] - '0') * 100) + read2digs(4);
                    num = mp_gotonum(num, mpActive);
                } else if(!strcmp(inputBuffer, "123456")) {
                    flushDelayedSave();
                    deleteIpSettings();               // *123456OK deletes IP settings
                    if(settings.appw[0]) {
                        settings.appw[0] = 0;         // and clears AP mode WiFi password
                        write_settings();
                    }
                } else if(!strcmp(inputBuffer, "654321")) {
                    deleteIRKeys();                   // *654321OK deletes learned IR remote
                    for(int i = 0; i < NUM_IR_KEYS; i++) {
                        remote_codes[i][1] = 0;
                    }
                } else {
                    doBadInp = true;
                }
            }
        }
        break;
    default:
        if(!isIRLocked) {
            doBadInp = true;
        }
    }
    clearInpBuf();

    return doBadInp;
}

/*
 * Speed pot
 */

static uint16_t getRawSpeed()
{
    long avg = 0, avg1 = 0, avg2 = 0;
    long raw;

    raw = analogRead(SPEED_PIN);

    if(raw > (1<<POT_RESOLUTION)-1)
        raw = (1<<POT_RESOLUTION)-1;

    //Serial.printf("raw %d\n", raw);

    if(anaReadCount > 1) {
      
        rawSpd[rawSpdIdx] = raw;

        if(anaReadCount < SPD_SMOOTH_SIZE) {
        
            avg = 0;
            for(int i = rawSpdIdx; i > rawSpdIdx - anaReadCount; i--) {
                avg += rawSpd[i & (SPD_SMOOTH_SIZE-1)];
            }
            avg /= anaReadCount;
            anaReadCount++;

        } else {

            for(int i = rawSpdIdx; i > rawSpdIdx - anaReadCount; i--) {
                if(i & 1) { 
                    avg1 += rawSpd[i & (SPD_SMOOTH_SIZE-1)];
                } else {
                    avg2 += rawSpd[i & (SPD_SMOOTH_SIZE-1)];
                }
            }
            avg1 = round((float)avg1 / (float)(SPD_SMOOTH_SIZE/2));
            avg2 = round((float)avg2 / (float)(SPD_SMOOTH_SIZE/2));
            avg = (abs(avg1-prev_avg) < abs(avg2-prev_avg)) ? avg1 : avg2;
            
            prev_avg = avg;
        }
        
    } else {
      
        anaReadCount++;
        rawSpd[rawSpdIdx] = avg = prev_avg = prev_raw = prev_raw2 = raw;
        
    }

    rawSpdIdx++;
    rawSpdIdx &= (SPD_SMOOTH_SIZE-1);

    prev_raw2 = prev_raw;
    prev_raw = raw;

    return (uint16_t)avg;
} 

static void setPotSpeed()
{
    unsigned long now = millis();
    
    if(TTrunning || IRLearning)
        return;

    if(!startSpdPot || (now - startSpdPot > 200)) {

        uint16_t spd = getRawSpeed() / (((1 << POT_RESOLUTION) - 1) / POT_GRAN);
        if(spd > POT_GRAN - 1) spd = POT_GRAN - 1;
        spd = potSpeeds[spd];
        if(fcLEDs.getSpeed() != spd) {
            fcLEDs.setSpeed(spd);
        }
        
        startSpdPot = now;
    }
}

/*
 * Switch music folder
 */
 
void switchMusicFolder(uint8_t nmf)
{
    bool wasActive = false;
    bool waitShown = false;

    if(nmf > 9) return;
    
    if(nmf != musFolderNum) {
        musFolderNum = nmf;
        // Initializing the MP can take a while;
        // need to stop all audio before calling
        // mp_init()
        if(haveMusic && mpActive) {
            mp_stop();
        } else if(playingFlux) {
            wasActive = true;
        }
        stopAudio();
        if(mp_checkForFolder(musFolderNum) == -1) {
            flushDelayedSave();
            showWaitSequence();
            waitShown = true;
            play_file("/renaming.mp3", PA_INTRMUS|PA_ALLOWSD);
            waitAudioDone(false);
        }
        saveMusFoldNum();
        mp_init(false);
        if(waitShown) {
            fcLEDs.SpecialSignal(0);
        }
        if(wasActive && contFlux()) play_flux();
        ir_remote.loop(); // Flush IR afterwards
    }
}

/*
 * Helpers
 */

void showWaitSequence()
{
    fcLEDs.SpecialSignal(FCSEQ_WAIT);
}

void endWaitSequence()
{
    fcLEDs.SpecialSignal(0);
}

void showCopyError()
{
    fcLEDs.SpecialSignal(FCSEQ_ERRCOPY);
}

void allOff()
{
    fcLEDs.off();
    centerLED.setDC(0);
    boxLED.setDC(0);
}

void populateIRarray(uint32_t *irkeys, int index)
{
    for(int i = 0; i < NUM_IR_KEYS; i++) {
        remote_codes[i][index] = irkeys[i]; 
    }
}

void copyIRarray(uint32_t *irkeys, int index)
{
    for(int i = 0; i < NUM_IR_KEYS; i++) {
        irkeys[i] = remote_codes[i][index];
    }
}

static void ttkeyScan()
{
    TTKey.scan();  // scan the tt button
}

static void TTKeyPressed()
{
    isTTKeyPressed = true;
}

static void TTKeyHeld()
{
    isTTKeyHeld = true;
}

// "Screen saver"

static void ssStart()
{
    if(ssActive)
        return;

    if(playingFlux) {
        stopAudio();
    }
    fluxTimer = false;

    fcLEDs.off();
    boxLED.setDC(0);

    ssActive = true;
}

static void ssRestartTimer()
{
    ssLastActivity = millis();
}

static void ssEnd(bool doSound)
{
    if(!FPBUnitIsOn)
        return;
        
    ssRestartTimer();
    
    if(!ssActive)
        return;

    fcLEDs.on();
    boxLED.setDC(mbllArray[minBLL]);

    if(doSound) {
        if(!mpActive) {
            if(playFLUX == 1) {
                play_flux();
            }
        }
    }

    ssActive = false;
}

void prepareTT()
{
    ssEnd(false);

    // Start flux sound (if so configured; we check
    // playTTsounds and not playFlux, since we expect
    // a timetravel, part of which is the flux sound), 
    // but honor the configured timer; we don't know
    // when (or if) the actual tt comes; the TCD will
    // count up the speed in the meantime, but
    // even the minimum of 30 seconds should always
    // cover the gap.
    if(playTTsounds) {
        if(mp_stop() || !playingFlux) {
           play_flux();
        }
        startFluxTimer();
    }
}

// Wakeup: Sent by TCD upon entering dest date,
// return from tt, triggering delayed tt via ETT
// For audio-visually synchronized behavior
// Also called when GPS/RotEnc speed is changed
void wakeup()
{
    // End screen saver
    ssEnd(false);

    // Start flux sound (if so configured)
    // unless mp is active
    if(!mpActive && playFLUX) {
        if(!playingFlux) {
           play_flux();
        }
        startFluxTimer();
    }
}

// Flux sound mode
// Might be called while ss is active (via MQTT)
// Is never called if fake-powered-off
void setFluxMode(int mode)
{
    switch(mode) {
    case 0:
        if(playingFlux) {
            stopAudio();
        }
        playFLUX = 0;
        fluxTimer = false;
        break;
    case 1:
        if(!mpActive && !ssActive) {
            append_flux();
        }
        playFLUX = 1;
        fluxTimer = false;
        break;
    case 2:
    case 3:
        if(playingFlux) {
            fluxTimerNow = millis();
            fluxTimer = true;
        }
        playFLUX = mode;
        fluxTimeout = (mode == 2) ? FLUXM2_SECS*1000 : FLUXM3_SECS*1000;
        break;
    }
}

void startFluxTimer()
{
    if(playFLUX >= 2) {
        fluxTimer = true;
        fluxTimerNow = millis();
    }
}

static bool contFlux()
{
    switch(playFLUX) {
    case 0:
        return false;
    case 1:
        return true;
    case 2:
    case 3:
        return fluxTimer;
    }

    return false;
}

static void waitAudioDone(bool withIR)
{
    int timeout = 400;

    while(!checkAudioDone() && timeout--) {
        mydelay(10, withIR);
    }
}

/*
 *  Do this whenever we are caught in a while() loop
 *  This allows other stuff to proceed
 *  withIR is needed for shorter delays; it allows
 *  filtering out repeated keys. If IR is flushed
 *  after the delay, withIR is not needed.
 */
static void myloop(bool withIR)
{
    wifi_loop();
    audio_loop();
    if(withIR) ir_remote.loop();
}

/*
 * MyDelay:
 * Calls myloop() periodically
 */
void mydelay(unsigned long mydel, bool withIR)
{
    unsigned long startNow = millis();
    myloop(withIR);
    while(millis() - startNow < mydel) {
        delay(10);
        myloop(withIR);
    }
}


/*
 * Basic Telematics Transmission Framework (BTTFN)
 */

static void addCmdQueue(uint32_t command)
{
    if(!command) return;

    commandQueue[iCmdIdx] = command;
    iCmdIdx++;
    iCmdIdx &= 0x0f;
}

static void bttfn_setup()
{
    useBTTFN = false;

    // string empty? Disable BTTFN.
    if(!settings.tcdIP[0])
        return;

    haveTCDIP = isIp(settings.tcdIP);
    
    if(!haveTCDIP) {
        #ifdef BTTFN_MC
        tcdHostNameHash = 0;
        unsigned char *s = (unsigned char *)settings.tcdIP;
        for ( ; *s; ++s) tcdHostNameHash = 37 * tcdHostNameHash + tolower(*s);
        #else
        return;
        #endif
    } else {
        bttfnTcdIP.fromString(settings.tcdIP);
    }
    
    fcUDP = &bttfUDP;
    fcUDP->begin(BTTF_DEFAULT_LOCAL_PORT);
    BTTFNfailCount = 0;
    useBTTFN = true;
}

void bttfn_loop()
{
    if(!useBTTFN)
        return;
        
    BTTFNCheckPacket();
    
    if(!BTTFNPacketDue) {
        // If WiFi status changed, trigger immediately
        if(!BTTFNWiFiUp && (WiFi.status() == WL_CONNECTED)) {
            BTTFNUpdateNow = 0;
        }
        if((!BTTFNUpdateNow) || (millis() - BTTFNUpdateNow > 1100)) {
            BTTFNTriggerUpdate();
        }
    }
}

// Check for pending packet and parse it
static void BTTFNCheckPacket()
{
    unsigned long mymillis = millis();
    
    int psize = fcUDP->parsePacket();
    if(!psize) {
        if(BTTFNPacketDue) {
            if((mymillis - BTTFNTSRQAge) > 700) {
                // Packet timed out
                BTTFNPacketDue = false;
                // Immediately trigger new request for
                // the first 10 timeouts, after that
                // the new request is only triggered
                // in greater intervals via bttfn_loop().
                if(haveTCDIP && BTTFNfailCount < 10) {
                    BTTFNfailCount++;
                    BTTFNUpdateNow = 0;
                }
            }
        }
        return;
    }
    
    fcUDP->read(BTTFUDPBuf, BTTF_PACKET_SIZE);

    // Basic validity check
    if(memcmp(BTTFUDPBuf, BTTFUDPHD, 4))
        return;

    uint8_t a = 0;
    for(int i = 4; i < BTTF_PACKET_SIZE - 1; i++) {
        a += BTTFUDPBuf[i] ^ 0x55;
    }
    if(BTTFUDPBuf[BTTF_PACKET_SIZE - 1] != a)
        return;

    if(BTTFUDPBuf[4] == (BTTFN_VERSION | 0x40)) {

        // A notification from the TCD

        switch(BTTFUDPBuf[5]) {
        case BTTFN_NOT_PREPARE:
            // Prepare for TT. Comes at some undefined point,
            // an undefined time before the actual tt, and
            // may not come at all.
            // We disable our Screen Saver and start the flux
            // sound (if to be played)
            // We don't ignore this if TCD is connected by wire,
            // because this signal does not come via wire.
            if(!TTrunning && !IRLearning) {
                prepareTT();
            }
            break;
        case BTTFN_NOT_TT:
            // Trigger Time Travel (if not running already)
            // Ignore command if TCD is connected by wire
            if(!TCDconnected && !TTrunning && !IRLearning) {
                networkTimeTravel = true;
                networkTCDTT = true;
                networkReentry = false;
                networkAbort = false;
                networkLead = BTTFUDPBuf[6] | (BTTFUDPBuf[7] << 8);
            }
            break;
        case BTTFN_NOT_REENTRY:
            // Start re-entry (if TT currently running)
            // Ignore command if TCD is connected by wire
            if(!TCDconnected && TTrunning && networkTCDTT) {
                networkReentry = true;
            }
            break;
        case BTTFN_NOT_ABORT_TT:
            // Abort TT (if TT currently running)
            // Ignore command if TCD is connected by wire
            if(!TCDconnected && TTrunning && networkTCDTT) {
                networkAbort = true;
            }
            break;
        case BTTFN_NOT_ALARM:
            networkAlarm = true;
            // Eval this at our convenience
            break;
        case BTTFN_NOT_FLUX_CMD:
            addCmdQueue( BTTFUDPBuf[6] | (BTTFUDPBuf[7] << 8) |
                        (BTTFUDPBuf[8] | (BTTFUDPBuf[9] << 8)) << 16);
            break;
        case BTTFN_NOT_WAKEUP:
            if(!TTrunning && !IRLearning) {
                wakeup();
            }
            break;
        }
      
    } else {

        // (Possibly) a response packet
    
        if(*((uint32_t *)(BTTFUDPBuf + 6)) != BTTFUDPID)
            return;
    
        // Response marker missing or wrong version, bail
        if(BTTFUDPBuf[4] != (BTTFN_VERSION | 0x80))
            return;

        BTTFNfailCount = 0;
    
        // If it's our expected packet, no other is due for now
        BTTFNPacketDue = false;

        #ifdef BTTFN_MC
        if(BTTFUDPBuf[5] & 0x80) {
            if(!haveTCDIP) {
                bttfnTcdIP = fcUDP->remoteIP();
                haveTCDIP = true;
                #ifdef FC_DBG
                Serial.printf("Discovered TCD IP %d.%d.%d.%d\n", bttfnTcdIP[0], bttfnTcdIP[1], bttfnTcdIP[2], bttfnTcdIP[3]);
                #endif
            } else {
                #ifdef FC_DBG
                Serial.println("Internal error - received unexpected DISCOVER response");
                #endif
            }
        }
        #endif

        if(BTTFUDPBuf[5] & 0x02) {
            gpsSpeed = (int16_t)(BTTFUDPBuf[18] | (BTTFUDPBuf[19] << 8));
            if(gpsSpeed > 88) gpsSpeed = 88;
            spdIsRotEnc = (BTTFUDPBuf[26] & 0x80) ? true : false;
        }
        if(BTTFUDPBuf[5] & 0x10) {
            tcdNM  = (BTTFUDPBuf[26] & 0x01) ? true : false;
            tcdFPO = (BTTFUDPBuf[26] & 0x02) ? true : false;   // 1 means fake power off
        } else {
            tcdNM = false;
            tcdFPO = false;
        }

        lastBTTFNpacket = mymillis;

        // Eval SID IP from TCD
        //if(BTTFUDPBuf[5] & 0x20) {
        //    Serial.printf("SID IP from TCD %d.%d.%d.%d\n", 
        //        BTTFUDPBuf[27], BTTFUDPBuf[28], BTTFUDPBuf[29], BTTFUDPBuf[30]);
        //}
    }
}

// Send a new data request
static bool BTTFNTriggerUpdate()
{
    BTTFNPacketDue = false;

    BTTFNUpdateNow = millis();

    if(WiFi.status() != WL_CONNECTED) {
        BTTFNWiFiUp = false;
        return false;
    }

    BTTFNWiFiUp = true;

    // Send new packet
    BTTFNSendPacket();
    BTTFNTSRQAge = millis();
    
    BTTFNPacketDue = true;
    
    return true;
}

static void BTTFNSendPacket()
{
    memset(BTTFUDPBuf, 0, BTTF_PACKET_SIZE);

    // ID
    memcpy(BTTFUDPBuf, BTTFUDPHD, 4);

    // Serial
    *((uint32_t *)(BTTFUDPBuf + 6)) = BTTFUDPID = (uint32_t)millis();

    // Tell the TCD about our hostname (0-term., 13 bytes total)
    strncpy((char *)BTTFUDPBuf + 10, settings.hostName, 12);
    BTTFUDPBuf[10+12] = 0;

    BTTFUDPBuf[10+13] = BTTFN_TYPE_FLUX;

    BTTFUDPBuf[4] = BTTFN_VERSION;  // Version
    BTTFUDPBuf[5] = 0x12;           // Request status and GPS speed

    //BTTFUDPBuf[5] |= 0x20;             // Query SID IP from TCD
    //BTTFUDPBuf[24] = BTTFN_TYPE_SID;

    #ifdef BTTFN_MC
    if(!haveTCDIP) {
        BTTFUDPBuf[5] |= 0x80;
        memcpy(BTTFUDPBuf + 31, (void *)&tcdHostNameHash, 4);
    }
    #endif

    uint8_t a = 0;
    for(int i = 4; i < BTTF_PACKET_SIZE - 1; i++) {
        a += BTTFUDPBuf[i] ^ 0x55;
    }
    BTTFUDPBuf[BTTF_PACKET_SIZE - 1] = a;

    #ifdef BTTFN_MC
    if(haveTCDIP) {
    #endif
        fcUDP->beginPacket(bttfnTcdIP, BTTF_DEFAULT_LOCAL_PORT);
    #ifdef BTTFN_MC    
    } else {
        #ifdef FC_DBG
        Serial.printf("Sending multicast (hostname hash %x)\n", tcdHostNameHash);
        #endif
        fcUDP->beginPacket("224.0.0.224", BTTF_DEFAULT_LOCAL_PORT + 1);
    }
    #endif
    fcUDP->write(BTTFUDPBuf, BTTF_PACKET_SIZE);
    fcUDP->endPacket();
}

static bool BTTFNTriggerTT()
{
    if(!useBTTFN)
        return false;

    #ifdef BTTFN_MC
    if(!haveTCDIP)
        return false;
    #endif

    if(WiFi.status() != WL_CONNECTED)
        return false;

    if(!lastBTTFNpacket)
        return false;

    if(TTrunning || IRLearning)
        return false;

    memset(BTTFUDPBuf, 0, BTTF_PACKET_SIZE);

    // ID
    memcpy(BTTFUDPBuf, BTTFUDPHD, 4);

    // Tell the TCD about our hostname (0-term., 13 bytes total)
    strncpy((char *)BTTFUDPBuf + 10, settings.hostName, 12);
    BTTFUDPBuf[10+12] = 0;

    BTTFUDPBuf[10+13] = BTTFN_TYPE_FLUX;

    BTTFUDPBuf[4] = BTTFN_VERSION;  // Version
    BTTFUDPBuf[5] = 0x80;           // Trigger BTTFN-wide TT

    uint8_t a = 0;
    for(int i = 4; i < BTTF_PACKET_SIZE - 1; i++) {
        a += BTTFUDPBuf[i] ^ 0x55;
    }
    BTTFUDPBuf[BTTF_PACKET_SIZE - 1] = a;
        
    fcUDP->beginPacket(bttfnTcdIP, BTTF_DEFAULT_LOCAL_PORT);
    fcUDP->write(BTTFUDPBuf, BTTF_PACKET_SIZE);
    fcUDP->endPacket();

    #ifdef FC_DBG
    Serial.println("Triggered BTTFN-wide TT");
    #endif

    return true;
}
