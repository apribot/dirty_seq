#include <EasyButton.h>
#include <MIDI.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include "stuff.h"

#define NOTE_POT 6 
#define VEL_POT 3
#define POS_POT 2
#define LEN_POT 1
#define BPM_POT 0
#define OCT_UP_BTN 10
#define OCT_DN_BTN 11
#define PLAY_BTN 12

#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif

uint8_t bpm = 120;
uint8_t steps = 16;
uint8_t pos = 0;
uint8_t curNote;
uint8_t lastNote = 0;
uint8_t curVelocity;

uint8_t selectedNoteLast;
uint8_t selectedNote;
uint8_t selectedVelocityLast;
uint8_t selectedVelocity;
uint8_t selectedPosition = 0;
uint8_t selectedOctave;
bool noteIsPlaying = false;
unsigned long previousMillis = 0;
bool isPlay = false;
bool settingsChanged = false;
bool isOctUp = false;
bool isOctDn = false;

MIDI_CREATE_DEFAULT_INSTANCE();
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
EasyButton play_button(PLAY_BTN);
EasyButton oct_dn_button(OCT_DN_BTN);
EasyButton oct_up_button(OCT_UP_BTN);

uint8_t midiToNote(uint8_t val) {
  return (val - 24) % 12;
}

uint8_t midiToOctave(uint8_t val) {
  return (val - 24) / 12;
}

unsigned long bpmToDelay(uint8_t val) {
  //60000 for for whole notes, we want quarter notes
  return 15000 / (unsigned long) val;
}

void loadSettings() {
  int addr;
  // load pattern notes

  // idk if this is a good idea
  // but we put a header in here 
  // to see if the eeprom has data yet
  if(EEPROM.read(0) == 4 && EEPROM.read(1) == 20) {
    
    for(addr=2; addr<18; addr++) {  
      patternNote[addr-2] = EEPROM.read(addr);
    }
    for(addr=18; addr<34; addr++) {
      patternVelocity[addr-18] = EEPROM.read(addr);
    }
    bpm = EEPROM.read(34);
  }
}

void saveSettings() {
  int addr;
  // header
  EEPROM.write(0, 4);
  EEPROM.write(1, 20);    
  
  for(addr=2; addr<18; addr++) {  
    EEPROM.write(addr, patternNote[addr-2]);
  }
  
  for(addr=18; addr<34; addr++) {
    EEPROM.write(addr, patternVelocity[addr-18]);
  }
  // this don't work yet uwu
  EEPROM.write(34, bpm);
}


void playInit() {
  lcd.home();
  
  for(int i=0; i<steps; i++) {
    lcd.printByte(0);
  }

  selectedNoteLast = selectedNote;
  selectedVelocityLast = selectedVelocity;
}

void playLoop() {
  curNote = patternNote[pos];
  curVelocity = patternVelocity[pos];

  if(curVelocity != 0) {
    MIDI.sendNoteOn(curNote, curVelocity, 1);
    noteIsPlaying = true;
  }
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
  lcd.print("dirt seq  ");
  lcd.print(bpm);
  lcd.print("bpm");
  pos++;
  pos = pos % steps;

  lastNote = curNote;
}

void doPlayPause() {
  saveSettings();
  if(isPlay) {
    // kill ringing note
    MIDI.sendNoteOff(lastNote, 0, 1);
    isPlay = false;
    // this code is trash and i'm sorry
    isOctUp = false;
    isOctDn = false;
    playInit();
  } else {
    playInit();
    pos = 0;
    isPlay = true;
  }
}

void doOctDn() {
  isOctDn = true;
}

void doOctUp() {
  isOctUp = true;  
}

void readPots() {
  // maybe break this up per loops for performance
  bpm = map(analogRead(BPM_POT), 0, 1023, 40, 200);
  selectedNote = map(analogRead(NOTE_POT), 0, 1023, 0, 12);
  selectedVelocity = map(analogRead(VEL_POT), 0, 1023, 0, 127);  
  selectedPosition = map(analogRead(POS_POT), 0, 1023, 0, 15);

}

void setup() {
  loadSettings();
  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  
  lcd.createChar(0, box0);
  lcd.createChar(1, box1);

  MIDI.begin(MIDI_CHANNEL_OMNI);
  play_button.begin();
  oct_dn_button.begin();
  oct_up_button.begin();
  play_button.onPressed(doPlayPause);
  oct_dn_button.onPressed(doOctDn);
  oct_up_button.onPressed(doOctUp);
  readPots();
  playInit();
}

void loop() {
  play_button.read();
  oct_dn_button.read();
  oct_up_button.read();
  unsigned long currentMillis = millis();

  readPots();
  
  if(isPlay) {
    if(currentMillis - previousMillis > bpmToDelay(bpm)) {
      previousMillis = currentMillis;
      playLoop();
    } else if(
      noteIsPlaying
      && (currentMillis - previousMillis > bpmToDelay(bpm) - 25 )
    ) {
      // hack for older synths that can't handle gate off being too close to gate on
      // picking 25ms at random, hope it works
      if(lastNote != 0) {
        MIDI.sendNoteOff(lastNote, 0, 1);
        noteIsPlaying = false;
      }
    }
  } else {
    // program mode
    if(isOctDn) {
      isOctDn = false;
      if(patternNote[pos] > 36) {
        patternNote[pos] -= 12;
        settingsChanged = true; 
        selectedOctave = midiToOctave(patternNote[pos]);
      }
    }
    if(isOctUp) {
      isOctUp = false;
      if(patternNote[pos] < 95) {
        patternNote[pos] += 12;
        settingsChanged = true;
        selectedOctave = midiToOctave(patternNote[pos]);
      }
    }

    if(selectedPosition != pos) {
      lcd.setCursor(pos,0);
      lcd.printByte(0);
      pos = selectedPosition;
      
      lcd.setCursor(pos,0);
      lcd.printByte(1);
      settingsChanged = true;
      selectedOctave = midiToOctave(patternNote[pos]);
    }
    if(selectedVelocityLast != selectedVelocity) {
      patternVelocity[pos] = selectedVelocity;
      selectedVelocityLast = selectedVelocity;
      settingsChanged = true;
    }

    if(selectedNoteLast != selectedNote) {
      patternNote[pos] = selectedNote + (selectedOctave * 12) + 24;
      selectedNoteLast = selectedNote;
      settingsChanged = true;
    }
    if(settingsChanged) {
      settingsChanged = false;
      
      curNote = patternNote[pos];
      curVelocity = patternVelocity[pos];
      
      lcd.setCursor(0,1);
      lcd.print(midiToOctave(curNote));
      lcd.print(notes[midiToNote(curNote)]);
      lcd.print(sharps[midiToNote(curNote)]);

      if(curVelocity == 0) {
        lcd.print("  off");
      } else {
        lcd.print(" v");
        lcd.print(curVelocity);
      }  
      lcd.print(" ");
      lcd.print(bpm);
      lcd.print("bpm");

    }
  
  }
}
