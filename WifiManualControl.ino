/***********************************************************************************************************************
   @file
   @ingroup
   @brief
 **********************************************************************************************************************/

/***********************************************************************************************************************
   I N C L U D E S
 **********************************************************************************************************************/
#include <KeyPadMatrix.h>
#include "Loclib.h"
#include "WmcTft.h"
#include "Z21Slave.h"
#include "app_cfg.h"
#include "fsmlist.hpp"
#include "wmc_event.h"
#include <KeypadMatrix.h>
#include <ArduinoOTA.h>

/***********************************************************************************************************************
   E X P O R T E D   S Y M B O L   D E F I N I T I O N S (defines, typedefs)
 **********************************************************************************************************************/

/***********************************************************************************************************************
   D A T A   D E C L A R A T I O N S (exported, local)
 **********************************************************************************************************************/

KeypadMatrix keypadMatrix = KeypadMatrix(PIN_KEYBOARD_C0, PIN_KEYBOARD_C1, PIN_KEYBOARD_C2, PIN_KEYBOARD_C3);

uint32 millisOld = millis();

// variables for encoder isr
boolean encoderStatusA = false;
boolean encoderStatusB = false;

unsigned long WmcStartMsPulseSwitchPushButton;
unsigned long WmcUpdateTimer3Seconds;
unsigned long WmcUpdatePulseSwitch;
unsigned long WmcUpdateTimer5msec;
unsigned long WmcUpdateTimer50msec;
unsigned long WmcUpdateTimer100msec;
unsigned long WmcUpdateTimer500msec;

// buffer to hold incoming and outgoing packets
volatile int encoderPos = 0;  // a counter for the dial
int encoderPosOld = 1;   // change management

updateEvent3sec wmcUpdateEvent3Sec;
powerOffEvent wmcPowerOffEvent;
pushButtonsEvent wmcPushButtonEvent;
pulseSwitchEvent wmcPulseSwitchEvent;
updateEvent5msec wmcUpdateEvent5msec;
updateEvent50msec wmcUpdateEvent50msec;
updateEvent100msec wmcUpdateEvent100msec;
updateEvent500msec wmcUpdateEvent500msec;

/***********************************************************************************************************************
   L O C A L   F U N C T I O N S
 **********************************************************************************************************************/

/**
 * Pin change ISR for Encoder pin A.
 */
static void isrPinChangeEncoderA() {
    delayMicroseconds(ENC_DEBOUNCE_DELAY_US);                     // a little delay for debouncing
    if (digitalRead(PIN_ENCODER_A) != encoderStatusA ) {          // read pin for encoder A again
        encoderStatusA = !encoderStatusA;
        if (encoderStatusA && !encoderStatusB) encoderPos += 1;   // increments encoderPos if A leads B
    }
}

/**
 * Pin change ISR for Encoder pin B.
 */
static void isrPinChangeEncoderB() {
    delayMicroseconds (ENC_DEBOUNCE_DELAY_US);                    // a little delay for debouncing
    if (digitalRead(PIN_ENCODER_B) != encoderStatusB) {           // read pin for encoder B again
        encoderStatusB = !encoderStatusB;
        if (encoderStatusB && !encoderStatusA) encoderPos -= 1;   //  decrements encoderPos if B leads A
    }
}

/**
 * Process key press at keypad matrix
 */
void processKeypadMatrix(uint8 keyCode, KeypadMatrix::keyAction keyAction)
{
    if (keyAction == KeypadMatrix::keyAction::released) {
        if (keyCode == button_encoder) {
            wmcPulseSwitchEvent.Status = pushedShort;
            wmcPushButtonEvent.Button = button_none;
            send_event(wmcPulseSwitchEvent);

        } else {
            wmcPushButtonEvent.Button = (pushButtons)keyCode;
            wmcPulseSwitchEvent.Status = turn;
            send_event(wmcPushButtonEvent);
        }

    } else if (keyAction == KeypadMatrix::keyAction::pressedLong) {
        if (keyCode == button_power) {
            send_event(wmcPowerOffEvent);
        }
    }
}

/***********************************************************************************************************************
 */
static bool WmcUpdate5msec(void)
{
    bool Result = false;

    if (millis() - WmcUpdateTimer5msec > 5)
    {
        Result              = true;
        WmcUpdateTimer5msec = millis();
        send_event(wmcUpdateEvent5msec);
    }

    return (Result);
}

/***********************************************************************************************************************
 */
static bool WmcUpdate50msec(void)
{
    bool Result = false;

    if (millis() - WmcUpdateTimer50msec > 50)
    {
        Result               = true;
        WmcUpdateTimer50msec = millis();
        send_event(wmcUpdateEvent50msec);
    }

    return (Result);
}

/***********************************************************************************************************************
 */
static bool WmcUpdate100msec(void)
{
    bool Result = false;

    if (millis() - WmcUpdateTimer100msec >= 100)
    {
        Result                = true;
        WmcUpdateTimer100msec = millis();
        send_event(wmcUpdateEvent100msec);
    }

    return (Result);
}

/***********************************************************************************************************************
 */
static bool WmcUpdate500msec(void)
{
    bool Result = false;

    if (millis() - WmcUpdateTimer500msec >= 500)
    {
        Result                = true;
        WmcUpdateTimer500msec = millis();
        send_event(wmcUpdateEvent500msec);
    }

    return (Result);
}

/***********************************************************************************************************************
 */
static bool WmcUpdate3Sec(void)
{
    bool Result = false;
    if (millis() - WmcUpdateTimer3Seconds > 3000)
    {
        Result                 = true;
        WmcUpdateTimer3Seconds = millis();
        send_event(wmcUpdateEvent3Sec);
    }

    return (Result);
}

/***********************************************************************************************************************
   E X P O R T E D   F U N C T I O N S
 **********************************************************************************************************************/

/**
 ******************************************************************************
   @brief
   @param
   @return     None
 ******************************************************************************/
void setup()
{
    // set long key press timeout
    keypadMatrix.setLongKeyPressTimeout(2000);

    #if PIN_LCD_BACKLIGHT > 0
        // setup Backlight pins
        #if PIN_LCD_BACKLIGHT == 15
            // pin 15 at ESP8266 is configured as HW_SPI CS-Pin we reconfigure this pin here
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15);
        #endif
        pinMode(PIN_LCD_BACKLIGHT, OUTPUT);
        digitalWrite(PIN_LCD_BACKLIGHT, LOW);     // Disable backlight here to prevent LCD flickering at power on
    #endif

    #if ENABLE_SERIAL_DEBUG == 1
        Serial.begin(76800);
    #endif

    // initialize the rotary encoder
    pinMode(PIN_ENCODER_A, INPUT_PULLUP);
    pinMode(PIN_ENCODER_B, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_A), isrPinChangeEncoderA, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_B), isrPinChangeEncoderB, CHANGE);

    /* Init timers. */
    WmcUpdateTimer3Seconds          = millis();
    WmcUpdatePulseSwitch            = millis();
    WmcUpdateTimer50msec            = millis();
    WmcUpdateTimer100msec           = millis();
    WmcUpdateTimer500msec           = millis();
    WmcUpdateTimer3Seconds          = millis();
    WmcStartMsPulseSwitchPushButton = millis();

    /* Kick the state machine. */
    fsm_list::start();

    pinMode(PIN_POWER_ENABLE, OUTPUT);
    digitalWrite(PIN_POWER_ENABLE, LOW);

    // switch TFT display backlight on
    #if PIN_LCD_BACKLIGHT > 0
        analogWrite(PIN_LCD_BACKLIGHT, 1024);
    #endif
}

/**
 ******************************************************************************
   @brief
   @param
   @return     None
 ******************************************************************************/
void loop() {

    ArduinoOTA.handle();

    /* Check for timed events. */
    WmcUpdate5msec();
    WmcUpdate50msec();
    WmcUpdate100msec();
    WmcUpdate500msec();
    WmcUpdate3Sec();

    #if ENABLE_SERIAL_DEBUG == 0
        // Do keypad scan
        keypadMatrix.scan();
    #else
        if (millis() - millisOld > 10) {
            Serial.end();
            delay(10);
            keypadMatrix.scan();
            Serial.begin(76800);
            millisOld = millis();
        }
    #endif

    // Process keypad key press
    if (millis() - millisOld > 100) {
        if (keypadMatrix.hasEvent()) {
            processKeypadMatrix(keypadMatrix.getKeycode(), keypadMatrix.getAction());
        }
        millisOld = millis();
    }

    // process encoder position
    int8_t encoderDelta = 0;
    if (encoderPos != encoderPosOld) {
        encoderDelta = encoderPos - encoderPosOld;
        encoderPosOld = encoderPos;
    }

    if (encoderDelta != 0) {
        wmcPulseSwitchEvent.Delta = encoderDelta;
        send_event(wmcPulseSwitchEvent);
    }
}
