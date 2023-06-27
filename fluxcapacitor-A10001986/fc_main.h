/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Flux Capacitor
 * (C) 2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Flux-Capacitor-A10001986
 *
 * Main controller
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

#ifndef _FC_MAIN_H
#define _FC_MAIN_H

// Number of IR keys
#define NUM_IR_KEYS 17

// FC LEDs 
#define FC_SPD_MAX 1     // 10ms
#define FC_SPD_MIN 500   // 5000ms
#define FC_SPD_IDLE 20

extern uint16_t lastIRspeed;

extern bool TTrunning;
extern bool playFLUX;
extern bool IRLearning;

#ifdef FC_HAVEMQTT
extern bool mqttTimeTravel;
extern bool mqttTCDTT;
extern bool mqttReentry;
extern bool mqttAlarm;
#endif

void main_boot();
void main_setup();
void main_loop();

void showWaitSequence();
void endWaitSequence();

void populateIRarray(uint32_t *irkeys, int index);
void copyIRarray(uint32_t *irkeys, int index);

void mydelay(unsigned long mydel, bool withIR);

#ifdef HAVEBTTFN_TEST
void bttfn_loop();
#endif

#endif
