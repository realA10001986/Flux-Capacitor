/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Flux Capacitor
 * (C) 2023-2025 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Flux-Capacitor
 * https://fc.out-a-ti.me
 *
 * FCDisplay: Handles the FC LEDs
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

#include "fcdisplay.h"

/*
 * PWM LED class for Center and Box LEDs
 */

// Store basic config data
PWMLED::PWMLED(uint8_t pwm_pin)
{
    _pwm_pin = pwm_pin;
}

void PWMLED::begin(uint8_t ledChannel, uint32_t freq, uint8_t resolution, uint8_t pwm_pin)
{
    _chnl = ledChannel;
    _freq = freq;
    _res = resolution;
    if(pwm_pin != 255) {
        _pwm_pin = pwm_pin;
    }
    
    // Config PWM properties
    ledcSetup(_chnl, _freq, _res);

    // Attach channel to GPIO
    ledcAttachPin(_pwm_pin, _chnl);

    // For 3.x (chnl unused)
    //ledcAttach(_pwm_pin, _freq, _res);

    // Set DC to 0
    setDC(0);
}

void PWMLED::setDC(uint32_t dutyCycle)
{
    _curDutyCycle = dutyCycle;
    ledcWrite(_chnl, dutyCycle);
    //ledcWrite(_pwm_pin, dutyCycle); // For 3.x
}

uint32_t PWMLED::getDC()
{
    return _curDutyCycle;
}

/*
 * FC LEDs class
 */

#define TMR_TIME      0.01    // 0.01s = 10ms
#define TMR_PRESCALE  80
#define TMR_TICKS     (uint64_t)(((double)TMR_TIME * 80000000.0) / (double)TMR_PRESCALE)
#define TME_TIMEUS    (TMR_TIME * 1000000)

static volatile uint8_t  _shift_clk;
static volatile uint8_t  _reg_clk;
static volatile uint8_t  _serdata;
static volatile uint8_t  _mreset;
static volatile uint32_t _ticks = 0;
static volatile bool     _critical = false;
static volatile uint16_t _tick_interval = 100;
static volatile bool     _fcledsoff = true;
static volatile bool     _fcledsareoff = false;
static volatile bool     _fcstopped = false;
#define SEQEND 0x80
static volatile uint8_t  _seqType = 0;
static volatile uint8_t  _index = 0;
static const DRAM_ATTR byte _array[] = {
        0b100000,
        0b010000,
        0b001000,
        0b000100,
        0b000010,
        0b000001,
        0b000000,   // TW: Added 9-5-2025, to match original circuit board's design for 7 lamps
        SEQEND
};
static const DRAM_ATTR byte _array1[] = {   //  KITT
        0b100000,
        0b010000,
        0b001000,
        0b000100,
        0b000010,
        0b000001,
        0b000010,
        0b000100,
        0b001000,
        0b010000,
        SEQEND
};
static const DRAM_ATTR byte _array2[] = {   //  spinner
        0b100000,
        0b110000,
        0b111000,
        0b111100,
        0b111110,
        0b111111,
        0b011111,
        0b001111,
        0b000111,
        0b000011,
        0b000001,
        SEQEND
};
static const DRAM_ATTR byte _array3[] = {   //  <>
        0b001100,
        0b010010,
        0b100001,
        0b010010,
        SEQEND
};
static const DRAM_ATTR byte _array4[] = {   //  <> full
        0b000000,
        0b001100,
        0b011110,
        0b111111,
        0b011110,
        0b001100,
        SEQEND
};
static const DRAM_ATTR byte _array5[] = {   //  <> exploding
        0b001100,
        0b011110,
        0b111111,
        0b110011,
        0b100001,
        SEQEND
};
static const DRAM_ATTR byte _array6[] = {   //  inverse normal
        0b000001,
        0b000010,
        0b000100,
        0b001000,
        0b010000,
        0b100000,
        SEQEND
};
static const DRAM_ATTR byte _array7[] = {   //  jumpman
        0b000001,
        0b100000,
        0b000010,
        0b010000,
        0b000100,
        0b001000,
        0b000100,
        0b010000,
        0b000010,
        0b100000,
        SEQEND
};
static const DRAM_ATTR byte _array8[] = {   //  dual runner
        0b100100,
        0b010010,
        0b001001,
        SEQEND
};
static const DRAM_ATTR byte _array9[] = {   // double runner
        0b110000,
        0b011000,
        0b001100,
        0b000110,
        0b000011,
        0b100001,
        SEQEND
};
static const byte* chaseArrs[10];
#define SS_ONESHOT 0xfffe   // Always needs to have "all off" as last step
#define SS_LOOP    0
#define SS_END     0xffff
static volatile bool     _specialsig = false;
static volatile bool     _wasSpecial = false;
static volatile bool     _specialOS = false;
static volatile uint8_t  _specialsignum = 0;
static volatile uint8_t  _specialidx = 0;
static volatile int16_t  _specialticks = 0;
static const DRAM_ATTR uint16_t _specialArray[FCSEQ_MAX][32] = {
        {                                               // 1: startup
          #define SPD 20
          SS_ONESHOT,
          0b100000, SPD, 0b110000, SPD, 0b111000, SPD,
          0b111100, SPD, 0b111110, SPD, 0b111111, SPD*2,
          0b111110, SPD, 0b111100, SPD, 0b111000, SPD,
          0b110000, SPD, 0b100000, SPD, SS_END
          // No "all off" at end, never run when FC chase is off
          #undef SPD
        },
        {                                               // 2: error: no audio files installed (128)
          SS_ONESHOT,
          0b000000, 100, 
          0b000001, 100, 0b000000, 100,
          0b000001, 100, 0b000000, 100, SS_END
        },
        {                                               // 3: wait: installing audio files / formatting FS / fw update
          SS_LOOP,
          0b100000, 50, 0b000001, 50, SS_END
        },
        {
          SS_ONESHOT,                                   // 4: error: Bad IR input (1)
          0b000000, 100,
          0b100000, 25, 0b000000, 25,
          0b100000, 25, 0b000000, 25, SS_END
        },
        {
          SS_ONESHOT,                                   // 5: Alarm (BTTFN/MQTT)
          0b000111,  50, 
          0b111000,  50,
          0b000111,  50, 
          0b111000,  50,
          0b000111,  50, 
          0b111000,  50,
          0b000111,  50, 
          0b111000,  50,
          0b000000,   1,
          SS_END
        },
        {
          SS_ONESHOT,                                   // 6: IR learning start
          0b000000,  20,
          0b111111, 100, 0b000000, 100,
          0b111111, 100, 0b000000,   1, SS_END
        },
        {
          SS_ONESHOT,                                   // 7: IR learning ok, next
          0b000000,  10,
          0b001100,  50, 0b000000,  50,
          0b001100,  50, 0b000000,   1, SS_END
        },
        {
          SS_ONESHOT,                                   // 8: IR learning finished
          0b000000,  10,
          0b111111,  50, 0b000000,  50,
          0b111111,  50, 0b000000,  50, SS_END
        },
        {
          SS_LOOP,                                      // 9: Error when copying audio files
          0b110000,  20, 0b000011,  20, SS_END
        },
        {
          SS_ONESHOT,                                   // 10: RemMode started
          0b000000,  10,
          0b101010,  50, 0b000000,  50,
          0b101010,  50, 0b000000,  50, SS_END
        },
        {
          SS_ONESHOT,                                   // 11: RemMode quit
          0b000000,  10,
          0b010101,  50, 0b000000,  50, SS_END
        },
        {
          SS_ONESHOT,                                   // 12: No music in current music folder
          0b000000,  10,
          0b000101,  50, 0b000000,  50,
          0b000101,  50, 0b000000,  50, SS_END
        }
};        

// ISR-helper: Update shift register
static void IRAM_ATTR updateShiftRegister(byte val)
{
    digitalWrite(_reg_clk, LOW);
    for(uint8_t i = 128; i != 0; i >>= 1) {
        digitalWrite(_serdata, !!(val & i));
        digitalWrite(_shift_clk, HIGH);
        digitalWrite(_shift_clk, LOW);
    }
    digitalWrite(_reg_clk, HIGH);
}

// ISR: Play sequences
static void IRAM_ATTR FCLEDTimer_ISR()
{
     if(_critical)
        return;
     
     if(_specialsig) {
      
        // Special sequence for signalling
        if(_specialticks == 0) {
            _wasSpecial = true;
            if(_specialArray[_specialsignum][_specialidx] == SS_END) {
                 if(_specialOS) {
                    _specialsig = false; 
                    _ticks = 0;
                    _index = 0;
                 } else {
                    _specialidx = 1; 
                 }
            }
            if(_specialsig) {
                updateShiftRegister(_specialArray[_specialsignum][_specialidx]);
            }
        }
        if(_specialsig) {
            _specialticks++;
            if(_specialticks >= _specialArray[_specialsignum][_specialidx + 1]) {
                _specialticks = 0;
                _specialidx += 2;
            }
        }
        
    } else {

        const byte *arr;

        if(_fcledsoff) {
            if(_fcledsareoff && !_wasSpecial) return;
            updateShiftRegister(0);
            _fcledsareoff = true;
            _wasSpecial = false;
            return;
        }
         
        if(_fcledsareoff) {
            _ticks = 0;
            _index = 0;
            _fcledsareoff = false;
        }

        if(_fcstopped)
            return;

        arr = chaseArrs[_seqType];
      
        // Normal sequences
        if(_ticks == 0) {
            updateShiftRegister(*(arr + _index));
        }
        _ticks++;
        if(_ticks >= _tick_interval) {
            _ticks = 0;
            _index++;
            if(*(arr + _index) == SEQEND) _index = 0;
        }
    }
}

FCLEDs::FCLEDs(uint8_t timer_no, uint8_t shift_clk, uint8_t reg_clk, uint8_t ser_data, uint8_t mreset)
{
    _timer_no = timer_no;
    _shift_clk = shift_clk;
    _reg_clk = reg_clk;
    _serdata = ser_data;
    _mreset = mreset;
}

void FCLEDs::begin()
{   
    pinMode(_reg_clk, OUTPUT);
    pinMode(_shift_clk, OUTPUT);  
    pinMode(_serdata, OUTPUT);
    pinMode(_mreset, OUTPUT);
    
    digitalWrite(_mreset, HIGH);

    // Set to "idle" speed
    setSpeed(20);

    // Switch off
    off();

    chaseArrs[0] = _array;
    chaseArrs[1] = _array1;
    chaseArrs[2] = _array2;
    chaseArrs[3] = _array3;
    chaseArrs[4] = _array4;
    chaseArrs[5] = _array5;
    chaseArrs[6] = _array6;
    chaseArrs[7] = _array7;
    chaseArrs[8] = _array8;
    chaseArrs[9] = _array9;
    
    // Install & enable timer interrupt
    _FCLTimer_Cfg = timerBegin(_timer_no, TMR_PRESCALE, true);
    timerAttachInterrupt(_FCLTimer_Cfg, &FCLEDTimer_ISR, true);
    // While in theory, this should work, it disturbs firmware updates:
    //timerAttachInterruptFlag(_FCLTimer_Cfg, &FCLEDTimer_ISR, true, ESP_INTR_FLAG_IRAM);
    timerAlarmWrite(_FCLTimer_Cfg, TMR_TICKS, true);
    timerAlarmEnable(_FCLTimer_Cfg);
}

void FCLEDs::on()
{
    _fcledsoff = false;
}

void FCLEDs::off()
{
    _fcledsoff = true;
}

void FCLEDs::stop(bool dostop)
{
    _fcstopped = !!dostop;
}

void FCLEDs::setSpeed(uint16_t speed)
{
    if(speed < 1) speed = 1;
    _critical = true;
    _tick_interval = speed;
    _critical = false;
    #ifdef FC_DBG
    Serial.printf("fcdisplay: Setting speed %d\n", speed);
    #endif
}

uint16_t FCLEDs::getSpeed()
{
    return _tick_interval;
}

void FCLEDs::setSequence(uint8_t seq)
{
    if(seq > 9) seq = 0;
    _critical = true;
    _seqType = seq;
    _ticks = 0;
    _index = 0;
    _critical = false;
}

// Special sequences

void FCLEDs::SpecialSignal(uint8_t signum) 
{
    _critical = true;
    _specialsig = false;
    _fcledsareoff = false;
    if(signum) {
        _specialsignum = signum - 1;
        _specialOS = (_specialArray[_specialsignum][0] == SS_ONESHOT);
        _specialidx = 1;
        _specialticks = 0;
        _specialsig = true;
    }
    _critical = false;
}

bool FCLEDs::SpecialDone()
{
    return !_specialsig;
}
