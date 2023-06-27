/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Flux Capacitor
 * (C) 2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Flux-Capacitor-A10001986
 *
 * FCRemote Class: Remote control handling
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

#ifndef _FCINPUT_H
#define _FCINPUT_H


/*
 * IRRemote class
 */

#define IRBUFSIZE 100

typedef enum {
    IRSTATE_IDLE,
    IRSTATE_LIGHT,
    IRSTATE_DARK,
    IRSTATE_STOP  
} IRState;

class IRRemote {

    public:
        IRRemote(uint8_t timerno, uint8_t ir_pin);
        void begin();

        bool loop();
        uint32_t readHash();
        void resume();
        
    private:
        uint32_t compare(unsigned int oldval, unsigned int newval);
        bool     calcHash();

        uint8_t _timer_no = 0;
        hw_timer_t *_IRTimer = NULL;

        uint32_t _buflen;
        uint32_t _buf[IRBUFSIZE];
        uint32_t _hvalue;

        unsigned long _prevTime;
        uint32_t      _prevHash;
};


/*
 * FCButton class
 */

typedef enum {
    TCBS_IDLE,
    TCBS_PRESSED,
    TCBS_RELEASED,
    TCBS_LONGPRESS,
    TCBS_LONGPRESSEND
} ButtonState;

class FCButton {
  
    public:
        FCButton(const int pin, const boolean activeLow = true, const bool pullupActive = true);
      
        void setDebounceTicks(const int ticks);
        void setPressTicks(const int ticks);
        void setLongPressTicks(const int ticks);
      
        void attachPress(void (*newFunction)(void));
        void attachLongPressStart(void (*newFunction)(void));
        void attachLongPressStop(void (*newFunction)(void));

        void scan(void);

    private:

        void reset(void);
        void transitionTo(ButtonState nextState);

        void (*_pressFunc)(void) = NULL;
        void (*_longPressStartFunc)(void) = NULL;
        void (*_longPressStopFunc)(void) = NULL;

        int _pin;
        
        unsigned int _debounceTicks = 50;
        unsigned int _pressTicks = 400;
        unsigned int _longPressTicks = 800;
      
        int _buttonPressed;
      
        ButtonState _state     = TCBS_IDLE;
        ButtonState _lastState = TCBS_IDLE;
      
        unsigned long _startTime;

        bool _pressNotified = false;
};

#endif
