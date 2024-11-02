/*
  File:     trs80-mod2-kb.ino
  Date:     2024.11.01
  Author:   Alain Boudreault - aka: ve2cuy, puyansude
  -------------------------------------------------------
  Description:  
  
  TODO:

*/

#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#include <PS2KeyAdvanced.h>

//#define TRACE
#define USE_BUSY_PIN
#define PS2_DATA_PIN              8 
#define PS2_IRQ_PIN               3     // On UNO, use pin 2 or 3
#define TRS80_KB_BUSY_PIN         10
#define TRS80_KB_DATA_PIN         11
#define TRS80_KB_CLOCK_PIN        12
#define TRS80_KB_END_PULSE_DELAY  165   // 165us produce a timing graph very close to a model II real keyboard

PS2KeyAdvanced keyboard;

// screen
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);  // High speed I2C

int compteur = 0;                 // Used with TRACE
char str[32];                     // Used with sprintf()
uint16_t c;                       // scan code from PS2 KB

// ------------------------------------------------------------------
void setup() {
// ------------------------------------------------------------------
  u8g2.begin();
  pinMode(TRS80_KB_BUSY_PIN, INPUT);
  pinMode(TRS80_KB_CLOCK_PIN, OUTPUT);
  pinMode(TRS80_KB_DATA_PIN, OUTPUT);
  digitalWrite(TRS80_KB_DATA_PIN, HIGH);

  u8g2.clearBuffer();                       // clear the display
  u8g2.setFont(u8g2_font_ncenB08_tr);       // choose a font
  u8g2.drawStr(0,10,"TRS-80M2-Keyboard");   // write something to the internal memory
  u8g2.sendBuffer();                        // transfer internal memory to the display
  keyboard.begin(PS2_DATA_PIN, PS2_IRQ_PIN);
  Serial.begin(115200);

 #ifdef TRACE
  Serial.println("\nTRS-80 Model II emulator");
#endif

  delay(1000);
  u8g2.clearBuffer();  
} // setup() 

// ------------------------------------------------------------------
void loop() {
// ------------------------------------------------------------------
  static byte statusBit = 0;
  static byte key = 0;

  if (keyboard.available())
    {

#ifdef TRACE
      //Serial.println(str);
#endif

    c = keyboard.read();
    if(c > 0) {
      statusBit =  c >> 8;
      key       =  c & 0xFF;
      sprintf(str, "%02d-%04X S:%02X K:%02X C:%c", compteur++, c, statusBit, key, key);
#ifdef TRACE
      Serial.println(str);
#endif
      u8g2.drawStr(0,40, str);    // write something to the internal memory
      u8g2.sendBuffer();          // transfer internal memory to the display

      key = translatePS2KeyToTRS80(key, statusBit);

      if ( key /*statusBit == 0 || statusBit == 1 */ /* FOR DEBUG || statusBit == 0xFF */) {
      sprintf(str, "Key sent to TRS* = %02X : %c\n", key, key );
#ifdef TRACE
        Serial.print(str);
#endif
        // Wait if CPU is busy before sending key sequence
#ifdef USE_BUSY_PIN        
        while(!digitalRead(TRS80_KB_BUSY_PIN));
#endif        
        u8g2.drawStr(0,55, str);
        u8g2.sendBuffer();
        keyToTRS(key);
      }  //   if statusBit
  
    }   // if keyboard.read > 0
  
   }  // if keyboard.available()

} // loop()

/// TODO: Optimise the code 
// ------------------------------------------------------------------
char translatePS2KeyToTRS80(char key, byte statusBit){
// ------------------------------------------------------------------
    static char shiftNumbers[] = {')','!','"','/','$','%','?','&','*','('};
    if (statusBit == 0 || statusBit == 1 || statusBit == 0xC0 /* FOR DEBUG || statusBit == 0xFF */) {
      key = key == 0x06 && statusBit == 0 ? 0x03 : key ;  // Break

      if (statusBit == 1) {
        key = key == 0x1E ? 0x0D : key; // Enter
        key = key == 0x1C ? 0x08 : key; // Back Space
        key = key == 0x1F ? 0x20 : key; // Space

        key = key == 0x17 ? 0x1E : key; // Arrow UP
        key = key == 0x18 ? 0x1F : key; // Arrow DOWN
        key = key == 0x15 ? 0x1C : key; // Arrow LEFT
        key = key == 0x16 ? 0x1D : key; // Arrow RIGHT
        key = key == 0x61 ? 0x01 : key; // F1
        key = key == 0x62 ? 0x02 : key; // F2
      } //  if statusBit == 1

      if ( statusBit == 0xC0 && (key >= '0' && key <='9') ) {
#ifdef TRACE
      Serial.println("SHIFT number key");
#endif              
        return shiftNumbers[key-0x30];
      }

      return key;
    } else
    {
      return 0;

    }
} // translatePS2KeyToTRS80

// ------------------------------------------------------------------
void keyToTRS(char key)
// ------------------------------------------------------------------
{
#ifdef TRACE
      Serial.print("In KeyToTRS with key: ");
      Serial.println(key);
#endif
      digitalWrite(TRS80_KB_CLOCK_PIN, LOW);
      // NOTE: arduino shiftout function does not produce a valide timing between DATA and CLOCK.
      //shiftOut(TRS80_DATA_PIN, TRS80_CLOCK_PIN, LSBFIRST, key);
      shiftOutV2(TRS80_KB_DATA_PIN, TRS80_KB_CLOCK_PIN, LSBFIRST, key, TRS80_KB_END_PULSE_DELAY);
      
      // END Pulse
      // delayMicroseconds(END_PULSE_DELAY/2);
      digitalWrite(TRS80_KB_DATA_PIN, LOW);
      delayMicroseconds(TRS80_KB_END_PULSE_DELAY/6);
      digitalWrite(TRS80_KB_DATA_PIN, HIGH);
} // keyToTRS


/**
    NOTE: On exit DATA pin will be HIGH and CLOCK will be LOW
*/
// ------------------------------------------------------------------
void shiftOutV2(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder,
                 uint8_t val, uint16_t speed)
// ------------------------------------------------------------------
{
        uint8_t i;

        for (i = 0; i < 8; i++)  {

                if (bitOrder == LSBFIRST)
                        digitalWrite(dataPin, !!(val & (1 << i)));
                else    
                       digitalWrite(dataPin, !!(val & (1 << (7 - i))));
               
               // Data is present on the BUS 1/2 cycle before clock signal
               // See Model II tech ref page 65 (keyboard timing)
               delayMicroseconds(speed/2);
               digitalWrite(clockPin, HIGH);
               delayMicroseconds(speed/2);
               digitalWrite(dataPin, HIGH); // Reset data half way in clock cycle.
               delayMicroseconds(speed/2);
               digitalWrite(clockPin, LOW); 
               delayMicroseconds(speed);
       } // for i
} // shiftOutV2

// -------->  END OF FILE
