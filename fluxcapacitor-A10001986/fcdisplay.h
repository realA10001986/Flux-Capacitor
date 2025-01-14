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

#ifndef _FCDISPLAY_H
#define _FCDISPLAY_H

/*
 * PWM LED class for Center and Box LEDs
 */

class PWMLED {

    public:

        PWMLED(uint8_t pwm_pin);
        void begin(uint8_t ledChannel, uint32_t freq, uint8_t resolution, uint8_t pwm_pin = 255);

        void setDC(uint32_t dutyCycle);
        uint32_t getDC();
        
    private:
        uint8_t   _pwm_pin;
        uint8_t   _chnl;
        uint32_t  _freq;
        uint8_t   _res;

        uint32_t _curDutyCycle;
};

// Special sequences
#define FCSEQ_STARTUP    1
#define FCSEQ_NOAUDIO    2
#define FCSEQ_WAIT       3
#define FCSEQ_BADINP     4
#define FCSEQ_ALARM      5
#define FCSEQ_LEARNSTART 6
#define FCSEQ_LEARNNEXT  7
#define FCSEQ_LEARNDONE  8
#define FCSEQ_ERRCOPY    9
#define FCSEQ_REMSTART   10
#define FCSEQ_REMEND     11
#define FCSEQ_NOMUSIC    12
#define FCSEQ_MAX        FCSEQ_NOMUSIC

/*
 * FC LEDs class
 */

class FCLEDs {

    public:

        FCLEDs(uint8_t timer_no, uint8_t shift_clk, uint8_t reg_clk, uint8_t ser_data, uint8_t mreset);
        void begin();
        
        void on();
        void off();

        void stop(bool stop);
        
        void setSpeed(uint16_t speed);
        uint16_t getSpeed();

        void setSequence(uint8_t seq);

        void SpecialSignal(uint8_t signum);
        bool SpecialDone();
        
    private:
        hw_timer_t *_FCLTimer_Cfg = NULL;
        uint8_t _timer_no;
        
};

#endif
