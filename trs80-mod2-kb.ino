#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#include <PS2KeyAdvanced.h>

//#define TRACE

const int PS2DataPin = 8; 
const int PS2IRQpin =  3; // Sur UNO, seulement pin 2 et 3

PS2KeyAdvanced keyboard;

// screen
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);  // High speed I2C

unsigned int END_PULSE_DELAY = 165; // 165us produce a timing graph very close to a model II real keyboard

int compteur = 0;
int busyPin   = 10;
int dataPin   = 11;
int clockPin  = 12;
bool keySent = false;

char str[32];
int x = 0;
uint16_t c;

static byte TRS80M2Keyboard[] = {1,2,3,4,5,6,7,8} ; 

void setup() {
  u8g2.begin();
  pinMode(busyPin, INPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  digitalWrite(dataPin, HIGH);

  u8g2.clearBuffer();                   // clear the internal memory
  u8g2.setFont(u8g2_font_ncenB08_tr);   // choose a suitable font
  u8g2.drawStr(0,10,"TRS-80M2-Keyboard");    // write something to the internal memory
  u8g2.sendBuffer();                    // transfer internal memory to the display
  keyboard.begin(PS2DataPin, PS2IRQpin);
  Serial.begin(115200);

 #ifdef TRACE
  Serial.println("TRS-80 Model II emulator");
#endif

  delay(1000);
  u8g2.clearBuffer();  
}

void loop() {
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

      key = translatePS2KeyToTRS80(key);



      if (statusBit == 0 || statusBit == 1 /* FOR DEBUG || statusBit == 0xFF */) {
      sprintf(str, "Key sent to TRS* = %02X", key );
#ifdef TRACE
        Serial.print(str);
#endif
        // Wait if CPU is busy before sending key sequence
        while(!digitalRead(busyPin));
        u8g2.drawStr(0,55, str);
        u8g2.sendBuffer();
        keyToTRS(key);
      }  //   if statusBit
  
    }   // if keyboard.read > 0
  
   }  // if keyboard.available()

} // loop()

/// TODO:
char translatePS2KeyToTRS80(char key){
  key = key == 6 ? 3 : key ;
  key = key == 0x1E ? 0x0D : key;
  key = key == 0x1C ? 0x08 : key;
  key = key == 0x1F ? 0x20 : key;
  return key;
} // translateKey

void keyToTRS(char key)
{
#ifdef TRACE
      Serial.print("In KeyToTRS with key: ");
      Serial.println(key);
#endif
      digitalWrite(clockPin, LOW);
      // NOTE: arduino shiftout function does not produce a valide timing between DATA and CLOCK.
      //shiftOut(dataPin, clockPin, LSBFIRST, key);
      shiftOutV2(dataPin, clockPin, LSBFIRST, key, END_PULSE_DELAY);
      
      // END Pulse
      // delayMicroseconds(END_PULSE_DELAY/2);
      digitalWrite(dataPin, LOW);
      delayMicroseconds(END_PULSE_DELAY/6);
      digitalWrite(dataPin, HIGH);
} // keyToTRS


/**
    NOTE: On exit DATA pin will be HIGH
*/
void shiftOutV2(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val, uint16_t speed)
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
