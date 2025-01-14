/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Flux Capacitor
 * (C) 2023-2025 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Flux-Capacitor
 * https://fc.out-a-ti.me
 *
 * FCRemote Class: Remote control handling
 * Inspired by Ken Shirriff's IRRemote library
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

#include <Arduino.h>

#include "input.h"

/*
 * IRRemote class
 */

#define TMR_TIME      0.00005    // 0.00005s = 50us
#define TMR_PRESCALE  80
#define TMR_TICKS     (uint64_t)(((double)TMR_TIME * 80000000.0) / (double)TMR_PRESCALE)
#define TME_TIMEUS    (TMR_TIME * 1000000)

#define GAP_DUR 5000  // Minimum gap between transmissions in us (microseconds)
#define GAP_TICKS     (GAP_DUR / TME_TIMEUS)

// IR receiver pin polarity
#define IR_LIGHT  0
#define IR_DARK   1

static void IRAM_ATTR IRTimer_ISR();

static uint8_t _ir_pin;

static volatile uint32_t _cnt = 0;
static volatile IRState  _irstate = IRSTATE_IDLE;
static volatile uint32_t _irlen = 0;
static volatile uint32_t _irbuf[IRBUFSIZE];

// ISR 
// Record duration of marks/spaces through a simple state machine
static void IRAM_ATTR IRTimer_ISR()
{
    uint8_t irpin = (uint8_t)digitalRead(_ir_pin);

    _cnt++;
    
    switch(_irstate) {
    case IRSTATE_IDLE:
        if(irpin == IR_LIGHT) {
            if(_cnt >= GAP_TICKS) {
                // Current gap longer than minimum gap size,
                // start recording.
                // (In case of a smaller gap, we assume being in 
                // the middle of a transmission whose start we 
                // missed. Do nothing then.)
                _irstate = IRSTATE_LIGHT;
                _irbuf[0] = _cnt;  // First is length of previous gap
                _irlen = 1;
            }
            _cnt = 0;
        }
        break;
    case IRSTATE_LIGHT:
        if(irpin == IR_DARK) {
            _irstate = IRSTATE_DARK;
            _irbuf[_irlen++] = _cnt;
            _cnt = 0;
            if(_irlen >= IRBUFSIZE) _irstate = IRSTATE_STOP;
        }
        break;
    case IRSTATE_DARK:
        if(irpin == IR_LIGHT) {
            _irstate = IRSTATE_LIGHT;
            _irbuf[_irlen++] = _cnt;
            _cnt = 0;
            if(_irlen >= IRBUFSIZE) _irstate = IRSTATE_STOP;
        } else if(_cnt > GAP_TICKS) {
            // Gap longer than usual space, transmission finished.
            _irstate = IRSTATE_STOP;
        }
        break;
    case IRSTATE_STOP:
        if(irpin == IR_LIGHT) _cnt = 0; // Reset cnt whenever we see something, even if we miss recording it
        break;
    }
}
 
// Store basic config data
IRRemote::IRRemote(uint8_t timer_no, uint8_t ir_pin)
{
    _timer_no = timer_no;
    _ir_pin = ir_pin;
}

void IRRemote::begin()
{
    pinMode(_ir_pin, INPUT);
    _irstate = IRSTATE_IDLE;
    _irlen = 0;

    // Install & enable interrupt
    _IRTimer = timerBegin(_timer_no, TMR_PRESCALE, true);
    timerAttachInterrupt(_IRTimer, &IRTimer_ISR, true);
    // While in theory, this should work, it disturbs firmware updates:
    //timerAttachInterruptFlag(_IRTimer, &IRTimer_ISR, true, ESP_INTR_FLAG_IRAM); //|ESP_INTR_FLAG_LEVEL2);
    timerAlarmWrite(_IRTimer, TMR_TICKS, true);
    timerAlarmEnable(_IRTimer);
}

// Decode IR signal
bool IRRemote::loop()
{
    // No new transmission, bail...
    if(_irstate != IRSTATE_STOP)
        return false;

    // Copy result to backup buffer
    _buflen = _irlen;
    for(uint8_t i = 0; i < _buflen; i++) {
        _buf[i] = _irbuf[i];
    }

    // Continue recording
    resume();
    
    // Calc hash on received "code"
    if(calcHash()) {
        unsigned long now = millis();
        if(_hvalue == _prevHash) {
            if(now - _prevTime < 300) {
                _prevTime = now;
                return false;
            }
        }
        _prevHash = _hvalue;
        _prevTime = now;
        return true;
    }

    return false;
}

void IRRemote::resume()
{
    _irstate = IRSTATE_IDLE;
}

uint32_t IRRemote::readHash()
{
    return _hvalue;
}


/* CalcHash: Calculate hash over an arbitrary IR code
 * 
 * Based on code published here:
 * http://arcfn.com/2010/01/using-arbitrary-remotes-with-arduino.html
 *
 */

uint32_t IRRemote::compare(uint32_t a, uint32_t b) 
{
    if(b < a * 80 / 100) return 0;
    if(a < b * 80 / 100) return 2;
    return 1;
}

/* Converts the raw code values into a 32-bit hash code.
 * Use FNV-1 (Fowler–Noll–Vo) hash algorithm.
 * https://en.wikipedia.org/wiki/Fowler-Noll-Vo_hash_function
 */

#define FNV_PRIME_32 16777619
#define FNV_BASIS_32 2166136261

bool IRRemote::calcHash()
{
    if(_buflen < 6)
        return false;
    
    uint32_t hash = FNV_BASIS_32;
    
    for(int i = 1; i + 2 < _buflen; i++) {
        hash = (hash * FNV_PRIME_32) ^ compare(_buf[i], _buf[i+2]);
    }
    
    _hvalue = hash;
    
    return true;
}

/*
 * FCButton class
 * 
 * If a Long-Press-function is registered, a "press" is only reported only after
 * the button is released. If no such function is registered, a press is
 * reported immediately (after PressTicks have elapsed), regardless of a button
 * release. The latter mode is used for when the TCD is connected to trigger
 * time travels.
 */

/* pin: The pin to be used
 * activeLow: Set to true when the input level is LOW when the button is pressed, Default is true.
 * pullupActive: Activate the internal pullup when available. Default is true.
 */
FCButton::FCButton(const int pin, const bool activeLow, const bool pullupActive, const bool pulldownActive)
{
    _pin = pin;

    _buttonPressed = activeLow ? LOW : HIGH;
  
    pinMode(pin, pullupActive ? INPUT_PULLUP : (pulldownActive ? INPUT_PULLDOWN : INPUT));
}

// Setup buttom timin:
// dticks: Number of millisec for a stable click to be assumed
// pticks: Number of millisec to pass for a short press 
// lticks: Number of millisec to pass for a long press
void FCButton::setTiming(const int debounceDur, const int pressDur, const int lPressDur)
{
    _debounceDur = debounceDur;
    _pressDur = pressDur;
    _longPressDur = lPressDur;
}

// Register function for short press event
void FCButton::attachPress(void (*newFunction)(void))
{
    _pressFunc = newFunction;
}

// Register function for long press start event
void FCButton::attachLongPressStart(void (*newFunction)(void))
{
    _longPressStartFunc = newFunction;
}

// Register function for long press stop event
void FCButton::attachLongPressStop(void (*newFunction)(void))
{
    _longPressStopFunc = newFunction;
}

// Check input of the pin and advance the state machine
void FCButton::scan(void)
{
    unsigned long now = millis();
    unsigned long waitTime = now - _startTime;
    bool active = (digitalRead(_pin) == _buttonPressed);
    
    switch(_state) {
    case TCBS_IDLE:
        if(active) {
            transitionTo(TCBS_PRESSED);
            _startTime = now;
        }
        break;

    case TCBS_PRESSED:
        if((!active) && (waitTime < _debounceDur)) {  // de-bounce
            transitionTo(_lastState);
        } else if(!active) {
            transitionTo(TCBS_RELEASED);
            _startTime = now;
        } else {
            if(!_longPressStartFunc) {
                if(waitTime > _pressDur) {
                    if(_pressFunc) _pressFunc();
                    _pressNotified = true;
                }      
            } else if(waitTime > _longPressDur) {
                if(_longPressStartFunc) _longPressStartFunc(); 
                transitionTo(TCBS_LONGPRESS);
            }
        }
        break;

    case TCBS_RELEASED:
        if((active) && (waitTime < _debounceDur)) {  // de-bounce
            transitionTo(_lastState);
        } else if((!active) && (waitTime > _pressDur)) {
            if(!_pressNotified && _pressFunc) _pressFunc();
            reset();
        }
        break;
  
    case TCBS_LONGPRESS:
        if(!active) {
            transitionTo(TCBS_LONGPRESSEND);
            _startTime = now;
        }
        break;

    case TCBS_LONGPRESSEND:
        if((active) && (waitTime < _debounceDur)) { // de-bounce
            transitionTo(_lastState);
        } else if(waitTime >= _debounceDur) {
            if(_longPressStopFunc) _longPressStopFunc();
            reset();
        }
        break;

    default:
        transitionTo(TCBS_IDLE);
        break;
    }
}

/*
 * Private
 */

void FCButton::reset(void)
{
    _state = TCBS_IDLE;
    _lastState = TCBS_IDLE;
    _startTime = 0;
    _pressNotified = false;
}

// Advance to new state
void FCButton::transitionTo(ButtonState nextState)
{
    _lastState = _state;
    _state = nextState;
}
 
