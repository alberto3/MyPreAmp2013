#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sfr_defs.h>
#include <EEPROM.h>
#include <ShiftLCD.h>
#include <IRReceiver.h>

//                           +-\/-+
//                     VCC  1|    |14  GND
//             (D 10)  PB0  2|    |13  AREF (D  0)
//             (D  9)  PB1  3|    |12  PA1  (D  1) 
//                     PB3  4|    |11  PA2  (D  2) 
//  PWM  INT0  (D  8)  PB2  5|    |10  PA3  (D  3) 
//  PWM        (D  7)  PA7  6|    |9   PA4  (D  4) 
//  PWM        (D  6)  PA6  7|    |8   PA5  (D  5)        PWM
//                           +----+

// IR DEFINES
#define IR_PIN 8

// BLINK LED
#define LED_PIN 7

// BUS
#define BUS_DATA_PIN 4
#define BUS_CLK_PIN 3

// LCD
#define LCD_LATCH_PIN 6

// Source & Volume
#define SOURCE_A_PIN 2
#define SOURCE_B_PIN 1
#define SOURCE_C_PIN 0
#define VOL_LATCH_PIN 5
#define MAX_VOLUME 192 // Gain(db) = 0
#define MAX_SOURCE 4
#define VOLUME_STEP 2 // should be greater than 1

uint8_t source = 0;
uint8_t volume = 160;
int8_t halfVolume = volume / 2;
bool isMuted = false;
bool isLightsOn = true;
ShiftLCD lcd(BUS_DATA_PIN, BUS_CLK_PIN, LCD_LATCH_PIN);
IRReceiver irReceiver(IR_PIN);
char* sourceArr[] = {
	"TV", "DVD", "PC", "AUX"
};

void setup(){
	delay(200);
	
	pinMode(LED_PIN, OUTPUT);
	pinMode(SOURCE_A_PIN, OUTPUT);
	pinMode(SOURCE_B_PIN, OUTPUT);
	pinMode(SOURCE_C_PIN, OUTPUT);
	pinMode(BUS_DATA_PIN, OUTPUT);
	pinMode(BUS_CLK_PIN, OUTPUT);
	pinMode(VOL_LATCH_PIN, OUTPUT);
	pinMode(LCD_LATCH_PIN, OUTPUT);
	
	attachInterrupt(0, SIGNAL_IR, FALLING);
	
	lcd.begin(16, 2);
	lcd.clear();
	lcd.noCursor();
	lcd.noAutoscroll();
	lcd.backlightOn();
	
	initScreen();
	showSource();
	setVolume(volume);
}

void loop(){

}

void SIGNAL_IR(){
	uint8_t oldSREG = SREG;
	cli();
	handleIRInterrupt();
	SREG = oldSREG;
}

void handleIRInterrupt(){
	unsigned long irCode;
	
	irCode = irReceiver.GetCode();
	
	if (irCode > 0)
	{
		switch(irCode){
			case 0x1000405: // Volume Up
				blink();
				if(volume < MAX_VOLUME)
				{
					isMuted = false;
					volume += VOLUME_STEP;
					halfVolume += (VOLUME_STEP / 2);
					setVolume(volume);
				}
				break;
				
			case 0x1008485: // Volume Down
				blink();
				if(volume > 0)
				{
					volume -= VOLUME_STEP;
					halfVolume -= (VOLUME_STEP / 2);
					setVolume(volume);
				}
				break;
				
			case 0x1004C4D: // Mute/Unmute
				blink();
				toggleMute();
				break;
				
			case 0x100A0A1: // Input selector
				blink();
				changeSource();
				break;
				
			case 0x1009C9D: // Lights On/Off
				blink();
				toggleLights();
				break;
		}
	}
}

void setVolume(uint8_t i_NewVolume){
	
	digitalWrite(VOL_LATCH_PIN, LOW);
	shiftOut(BUS_DATA_PIN, BUS_CLK_PIN, MSBFIRST, i_NewVolume);
	shiftOut(BUS_DATA_PIN, BUS_CLK_PIN, MSBFIRST, i_NewVolume);
	digitalWrite(VOL_LATCH_PIN, HIGH);
	
	showVolume();
}

void toggleMute(){
	isMuted = !isMuted;
	
	if(isMuted)
	{
		setVolume(0);
	}
	else
	{
		setVolume(volume);
	}
	showVolume();
}

void toggleLights(){
	isLightsOn = !isLightsOn;
	
	if(isLightsOn)
	{
		lcd.backlightOn();
	}
	else
	{
		lcd.backlightOff();
	}
}

void changeSource(){
	source++;
	
	if(source == MAX_SOURCE)
	{
		source = 0;
	}
	
	digitalWrite(SOURCE_A_PIN, source & 0x01);
	digitalWrite(SOURCE_B_PIN, source & 0x02);
	digitalWrite(SOURCE_C_PIN, source & 0x04);
	
	showSource();
}

void blink(){
	digitalWrite(LED_PIN, HIGH);
	delay(5);
	digitalWrite(LED_PIN, LOW);
}

void initScreen(){
	lcd.setCursor(0, 0);
	lcd.print("INPUT: ");
	lcd.setCursor(0, 1);
	lcd.print("VOLUME: ");
}

void showSource(){
	lcd.setCursor(7, 0);
	lcd.print(sourceArr[source]);
	lcd.print("   "); // spaces clean extra chars from previous source name
}

void showVolume(){
	lcd.setCursor(8, 1);
	if(isMuted)
	{
		lcd.print("*MUTE*");
	}
	else
	{
		lcd.print(getDecibel());
		lcd.print("dB      "); // spaces clean "*MUTE*"
	}
}

int8_t getDecibel(){
	return halfVolume - 96;
}
