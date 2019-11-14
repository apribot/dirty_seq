#include <MIDI.h>

#include <Wire.h>

#include <LiquidCrystal_I2C.h>



#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif

MIDI_CREATE_DEFAULT_INSTANCE();


uint8_t box0[] = {
  0x1F,
  0x11,
  0x11,
  0x11,
  0x11,
  0x11,
  0x11,
  0x1F
};

uint8_t box1[8] = {
  0x1F,
  0x1F,
  0x1F,
  0x1F,
  0x1F,
  0x1F,
  0x1F,
  0x1F
};

char notes[12] = {
  'A',
  'A',
  'B',
  'C',
  'C',
  'D',
  'D',
  'E',
  'F',
  'F',
  'G',
  'G'  
};

char sharps[12] {
  ' ',
  '#',
  ' ',
  ' ',
  '#',
  ' ',
  '#',
  ' ',
  ' ',
  '#',
  ' ',
  '#'
};

uint8_t bpm = 120;
uint8_t steps = 16;

uint8_t patternNote[16] = {
  60,
  64,
  67,
  71,
  72,
  71,
  67,
  64,
  60,
  64,
  67,
  71,
  72,
  71,
  67,
  64,
    
};

uint8_t patternVelocity[16] = {
  200,
  210,
  220,
  230,
  240,
  230,
  220,
  210,
  200,
  210,
  220,
  230,
  240,
  230,
  220,
  210
};


LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

uint8_t pos = 0;
uint8_t curNote;
uint8_t lastNote = 0;
uint8_t curVelocity;

uint8_t midiToNote(uint8_t val) {
  return (val - 21) % 12;
}

uint8_t midiToOctave(uint8_t val) {
  return (val - 21) / 12;
}

int bpmToDelay(uint8_t val) {
  return 60000 / (int) val;
}


void setup() {
  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  
  lcd.createChar(0, box0);
  lcd.createChar(1, box1);

  lcd.home();
  
  for(int i=0; i<steps; i++) {
    lcd.printByte(0);
  }


}

void loop() {

  curNote = patternNote[pos];
  curVelocity = patternVelocity[pos];

  MIDI.sendNoteOff(lastNote, 0, 1);
  MIDI.sendNoteOn(curNote, curVelocity, 1);

  if(pos == 0) {
    lcd.setCursor(steps-1,0);
    lcd.printByte(0);
    lcd.setCursor(pos,0);
    lcd.printByte(1);
  } else {
    lcd.setCursor(pos-1, 0);
    lcd.printByte(0);
    lcd.printByte(1);
  }

  lcd.setCursor(0,1);
  lcd.print(midiToOctave(curNote));
  lcd.print(notes[midiToNote(curNote)]);
  lcd.print(sharps[midiToNote(curNote)]);

  lcd.print(" v");
  lcd.print(curVelocity);
  pos++;
  pos = pos % steps;

  lastNote = curNote;
  delay(bpmToDelay(bpm));

}
