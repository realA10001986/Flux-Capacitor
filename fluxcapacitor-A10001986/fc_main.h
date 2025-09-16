/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Flux Capacitor
 * (C) 2023-2025 Thomas Winischhofer (A10001986)
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

#ifndef _FC_MAIN_H
#define _FC_MAIN_H

// Number of IR keys
#define NUM_IR_KEYS 17

// FC LEDs 
#define FC_SPD_MAX 3     // 30ms
#define FC_SPD_MIN 500   // 5000ms
#define FC_SPD_IDLE 20

extern unsigned long powerupMillis;

extern uint16_t minBLL;
extern uint16_t lastIRspeed;
extern bool     irLocked;

extern bool TCDconnected;

extern bool FPBUnitIsOn;
extern bool fluxNM;

extern uint8_t fluxPat;

extern bool TTrunning;
extern int  playFLUX;
extern bool IRLearning;

extern bool networkTimeTravel;
extern bool networkTCDTT;
extern bool networkReentry;
extern bool networkAbort;
extern bool networkAlarm;
extern uint16_t networkLead;

extern uint32_t myRemID;

void main_boot();
void main_setup();
void main_loop();

void flushDelayedSave();

bool decIRSpeed();
bool incIRSpeed();
bool resetIRSpeed();
void setFluxPattern(uint8_t i);

void showWaitSequence();
void endWaitSequence();
void showCopyError();

void allOff();
void prepareReboot();

void populateIRarray(uint32_t *irkeys, int index);
void copyIRarray(uint32_t *irkeys, int index);

void setFluxMode(int mode);
void startFluxTimer();

bool switchMusicFolder(uint8_t nmf);

void mydelay(unsigned long mydel, bool withIR);

void prepareTT();
void wakeup();

void bttfn_loop();

#endif
