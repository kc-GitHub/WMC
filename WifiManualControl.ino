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

/***********************************************************************************************************************
   E X P O R T E D   S Y M B O L   D E F I N I T I O N S (defines, typedefs)
 **********************************************************************************************************************/
#define PIN_ENCODER_A           4 // D2 on WEMOS D1 Mini
#define PIN_ENCODER_B           5 // D1 on WEMOS D1 Mini
#define ENC_DEBOUNCE_DELAY_US   500

/***********************************************************************************************************************
   D A T A   D E C L A R A T I O N S (exported, local)
 **********************************************************************************************************************/

#if ENABLE_SERIAL_DEBUG == 0
	KeyPadMatrix keyPadMatrix = KeyPadMatrix(PIN_KEYBOARD_C0, PIN_KEYBOARD_C1, PIN_KEYBOARD_C2, PIN_KEYBOARD_C3);
#else {
	// KeyPadMatrix: Because column 3 was connected to TX pin, we must deactivate keys on column 5 before activate Serial interface
	KeyPadMatrix keyPadMatrix = KeyPadMatrix(PIN_KEYBOARD_C0, PIN_KEYBOARD_C1, PIN_KEYBOARD_C2);
#endif

#define KEYBOARD_SCAN_TIME      60000 //12000 us


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
 */
}

/**
 * Pin change ISR for Encoder pin A.
 */
static void isrPinChangeEncoderA() {
	delayMicroseconds(ENC_DEBOUNCE_DELAY_US);					// a little delay for debouncing
	if (digitalRead(PIN_ENCODER_A) != encoderStatusA ) {		// read pin for encoder A again
		encoderStatusA = !encoderStatusA;
		if (encoderStatusA && !encoderStatusB) encoderPos += 1;	// increments encoderPos if A leads B
	}
}

/**
 * Pin change ISR for Encoder pin B.
 */
static void isrPinChangeEncoderB() {
	delayMicroseconds (ENC_DEBOUNCE_DELAY_US);					// a little delay for debouncing
	if (digitalRead(PIN_ENCODER_B) != encoderStatusB) {			// read pin for encoder B again
		encoderStatusB = !encoderStatusB;
		if (encoderStatusB && !encoderStatusA) encoderPos -= 1;	//  decrements encoderPos if B leads A
	}
}

/**
 * Process key press at keypad matrix
 */
void processKeyPadMatrix(uint8 keyCode) {
	if (keyCode == 0)   {
		// no key was pressed

	} else if (keyCode == KEYCODE_0)   {
        wmcPushButtonEvent.Button = button_0;
        send_event(wmcPushButtonEvent);

	} else if (keyCode == KEYCODE_1)   {
		wmcPushButtonEvent.Button = button_1;
        send_event(wmcPushButtonEvent);

	} else if (keyCode == KEYCODE_2)  {
		wmcPushButtonEvent.Button = button_2;
        send_event(wmcPushButtonEvent);

	} else if (keyCode == KEYCODE_3)  {
		wmcPushButtonEvent.Button = button_3;
        send_event(wmcPushButtonEvent);

	} else if (keyCode == KEYCODE_4)   {
		wmcPushButtonEvent.Button = button_4;
        send_event(wmcPushButtonEvent);

	} else if (keyCode == KEYCODE_5)   {
		wmcPushButtonEvent.Button = button_5;
        send_event(wmcPushButtonEvent);

	} else if (keyCode == KEYCODE_6)  {
	} else if (keyCode == KEYCODE_7)  {
	} else if (keyCode == KEYCODE_8)  {
	} else if (keyCode == KEYCODE_9)  {
	} else if (keyCode == KEYCODE_LEFT) {
	} else if (keyCode == KEYCODE_RIGHT) {
	} else if (keyCode == KEYCODE_POWER)  {
		wmcPushButtonEvent.Button = button_power;
        send_event(wmcPushButtonEvent);

	} else if (keyCode == KEYCODE_MENU) {
        wmcPulseSwitchEvent.Status = pushedlong;			// > 3000ms
        send_event(wmcPulseSwitchEvent);

	} else if (keyCode == KEYCODE_MODE) {
        wmcPulseSwitchEvent.Status = pushedNormal;			// < 1100ms
        send_event(wmcPulseSwitchEvent);

	} else if (keyCode == KEYCODE_ENCODER_BTN) {
        wmcPulseSwitchEvent.Status = pushedShort;			// < 300ms
        send_event(wmcPulseSwitchEvent);
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
}

/**
 ******************************************************************************
   @brief
   @param
   @return     None
 ******************************************************************************/
void loop()
{
	}

    /* Check for timed events. */
    WmcUpdate5msec();
    WmcUpdate50msec();
    WmcUpdate100msec();
    WmcUpdate500msec();
    WmcUpdate3Sec();

    // Do keypad scan
	keyPadMatrix.scan();

    // Process keypad key press
	if (keyPadMatrix.keyCodeHasChanged()) {
		uint8_t keyCode = keyPadMatrix.getKeycode();
		processKeyPadMatrix(keyCode);
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
