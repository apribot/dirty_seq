#include <EasyButton.h>
#include <MIDI.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#define NOTE_POT 4 
#define VEL_POT 3
#define POS_POT 2
#define LEN_POT 1
#define BPM_POT 0
#define OCT_UP_BTN 10
#define OCT_DN_BTN 11
#define PLAY_BTN 12

/* 
 * 
 * [+1]
 * 
 * [-1][note][velo]   [play]    [bpm][length]
 */



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
  64    
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
EasyButton play_button(PLAY_BTN);

uint8_t pos = 0;
uint8_t curNote;
uint8_t lastNote = 0;
uint8_t curVelocity;
uint8_t selectedNote;
uint8_t selectedVelocity;



unsigned long previousMillis = 0;
bool isPlay = false;

uint8_t midiToNote(uint8_t val) {
  return (val - 21) % 12;
}

uint8_t midiToOctave(uint8_t val) {
  return (val - 21) / 12;
}

unsigned long bpmToDelay(uint8_t val) {
  //60000 for for whole notes, we want quarter notes
  return 15000 / (unsigned long) val;
}

void playInit() {
  lcd.home();
  
  for(int i=0; i<steps; i++) {
    lcd.printByte(0);
  }
}

void playLoop() {
  curNote = patternNote[pos];
  curVelocity = patternVelocity[pos];

  if(lastNote != 0) {
    MIDI.sendNoteOff(lastNote, 0, 1);
  }
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

  lcd.print(" ");
  lcd.print(bpm);
  lcd.print("bpm");
  pos++;
  pos = pos % steps;

  lastNote = curNote;

}

void doPlayPause() {
  if(isPlay) {
    // kill ringing note
    MIDI.sendNoteOff(lastNote, 0, 1);
    isPlay = false;
    playInit();
  } else {
    pos = 0;
    isPlay = true;
  }
}


void setup() {
  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  
  lcd.createChar(0, box0);
  lcd.createChar(1, box1);

  MIDI.begin(MIDI_CHANNEL_OMNI);
  play_button.begin();
  play_button.onPressed(doPlayPause);
  playInit();

}

void readPots() {

  // maybe break this up per loops for performance
  bpm = map(analogRead(BPM_POT), 0, 1023, 40, 200);
  selectedNote = map(analogRead(NOTE_POT), 0, 1023, 0, 12);
  selectedVelocity = map(analogRead(VEL_POT), 0, 1023, 0, 255);

  
}

void loop() {
  play_button.read();
  unsigned long currentMillis = millis();

  readPots();
  
  if(isPlay) {
    if(currentMillis - previousMillis > bpmToDelay(bpm)) {
      previousMillis = currentMillis;
      playLoop();
    }
  }
}
