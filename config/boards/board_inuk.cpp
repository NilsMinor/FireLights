
#include <FruityHal.h>
#include <Boardconfig.h>
#include <InukTypes.h>

// inuk base board - https://github.com/NilsMinor/inuk-base

void setCustomPinset_Inuk(CustomPins* pinConfig){
		InukExtPins* pins = (InukExtPins*)pinConfig;
        pins->lio1      = 11;   // pwm output
		pins->lio2      = 12;   // pwm output
        pins->lio3      = 13;   // pwm output
        pins->lio4      = 14;   // pwm output
        pins->lio5      = 15;   // pwm output

        pins->vbatPin   = 2;    // AIN0
        pins->vsolarPin = 3;    // AIN1
        pins->pirPin    = 16;   // high active
}

void setBoard_inuk(BoardConfiguration *c)
{
#if BOARD_TYPE == 10000
    if (c->boardType == 10000)
    {
        c->led1Pin = 26;
        c->led2Pin = 27;
        //c->led3Pin = 19;
        c->ledActiveHigh = false;
        c->button1Pin = 25;
        c->buttonsActiveHigh = false;

        c->uartRXPin = 8;
        c->uartTXPin = 6;
        // dsiable flow control by setting RTS pin to -1
        c->uartCTSPin = -1;
        c->uartRTSPin = -1;
        c->uartBaudRate = (u32)FruityHal::UartBaudrate::BAUDRATE_115200;

        c->dBmRX = -96;
        c->calibratedTX = -60;
        c->lfClockSource = (u8)FruityHal::ClockSource::CLOCK_SOURCE_XTAL;
        c->lfClockAccuracy = (u8)FruityHal::ClockAccuracy::CLOCK_ACCURACY_20_PPM;
        c->dcDcEnabled = false;

        // disable fruitymesh adc
        c->batteryAdcInputPin = -1;

    }
#endif // BOARD_TYPE == 4
}
