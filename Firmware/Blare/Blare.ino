#include <Adafruit_GFX_h>
#include <Adafruit_ST7789_h>
#include <SPI.h>
#include <stdubt.h>

#define TFT_SCLK 9
#define TFT_MOSI 10
#define TFT_DC 5
#define TFT_CS 6

const uint8_t ROW_PINS[2] = {
  D0, D1
};
const uint8_t COL_PINS[3] = {
  D2, D3, D4
};
const uint8_t BUZZER_PIN = D7;

class MyST7789: public Adafruit_ST7789 {
  public:
    MyST7789(int8_t cs, int8_t dc, int8_t mosi, int8_t sclk)
      : Adafruit_ST7789(cs, dc, mosi, sclk) {}
    
    void setOffsets(uint8_t col, uint8_t row) {
      _colstart = _colstart2 = col;
      _rowstart = _rowstart2 = row;
    }
};

MyST7789 screen(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK)

const uint8_t KEY_MODE = 0;
const uint8_t KEY_UP = 1;
const uint8_t KEY_DOWN = 2;
const uint8_t KEY_ALARM = 3;
const uint8_t KEY_SNOOZE = 4;
const uint8_t KEY_SET = 5;

enum EditField {
  EDIT_CLOCK_HOUR,
  EDIT_CLOCK_MINUTE,
  EDIT_ALARM_HOUR,
  EDIT_ALARM_MINUTE
};

uint8_t clockHour = 12;
uint8_t clockMinute = 0;
uint8_t clockSecond = 0;

uint8_t alarmHour = 7;
uint8_t alarmMinute = 0;
bool alarmEnabled = false;

bool editing = false;
bool showAlarmView = false;
bool ringing = false;
bool snoozeActive = false;

EditField editField = EDIT_CLOCK_HOUR;

uint32_t currentDay = 0;
uint32_t uptimeSeconds = 0;
uint32_t lastFiredDay = UINT32_MAX;
uint32_t snoozeUntil = 0;
uint32_t lastTickMs = 0;

int lastRawKey = -1;
int stableKey = -1;
uint32_t rawKeyChangedMs = 0;

bool screenDirty = true;
bool lastBlinkState = true;

const uint32_t DEBOUNCE_MS = 30;
const uint32_t SNOOZE_SECONDS = 300;

void drawCenteredText(const char *text, int16_t y, uint8_t size, uint16_t colour) {
  screen.setTextSize(size);
  screen.setTextColour(colour);

  int16_t x1, y1;
  uint16_t w, h;
  screen.getTextBounds(text, 0, y, &x1, &y1, &w, &h);

  int16_t x = (screen.width() - w) / 2 - x1;
  screen.setCursor(x, y);
  screen.print(text);
}

int readRawKey() {
  int pressedKey = -1;

  for (uint8_t row = 0; row < 2; row++) {
    pinMode(ROW_PINS[row], OUTPUT);
    digitalWrite(ROW_PINS[row], LOW);
    delayMicroseconds(3);

    for (uint8_t col = 0; col < 3; col++) {
      if (digitalRead(COL_PINS[col]) == LOW && pressedKey == -1) {
        pressedKey = row * 3 + col;
      }
    }

    pinMode(ROW_PINS[row], INPUT_PULLUP);
  }

  return pressedKey;
}

void changeSelectedField(int8_t direction) {
  switch (editField) {
    case EDIT_CLOCK_HOUR:
      clockHour = (clockHour + (direction > 0 ? 1: 23)) % 24;
      clockSecond = 0;
      break;
    
    case EDIT_CLOCK_MINUTE:
      clockMinute = (clockMinuute + (direction > 0 ? 1 : 59)) % 60;
      clockSecond = 0;
      break;
    
    case EDIT_ALARM_HOUR:
      alarmHour = (alarmHour + (direction > 0 ? 1: 23)) % 24;
      break;
    
    case EDIT_ALARM_MINUTE:
      alarmMinute = (alarmMinuute + (direction > 0 ? 1 : 59)) % 60;
      break;
  }

  screenDirty = true;
}

void handleKeyPress(uint8_t key) {
  switch (key) {
    case KEY_MODE:
      if (editing) {
        editField = static_cast<EditField>((editField + 1) % 4);
        showAlarmView = (editField == EDIT_ALARM_HOUR || editField == EDIT_ALARM_MINUTE);
      } else {
        showAlarmView = !showAlarmView;
      }

      screenDirty = true;
      break;

    case KEY_UP:
      if(editing) changeSelectedField(1);
      break;
    
    case KEY_DOWN:
      if(editing) changeSelectedField(-1);
      break;
    
    case KEY_ALARM:
      alarmEnabled = !alarmEnabled;
      if(!alarmEnabled) {
        ringing = false;
        snoozeActive = false;
      }
      screenDirty = true;
      break

    case KEY_SNOOZE:
      if (ringing) {
        ringing = false;
        snoozeActive = true;
        snoozeUntil = uptimeSeconds + SNOOZE_SECONDS;
        screenDirty = true;
      }
      break;
    
    case KEY_SET:
      if (!editing) {
        editing = true;
        editField = showAlarmView ? EDIT_ALARM_HOUR : EDIT_CLOCK_HOUR;
        showAlarmView = (editField == EDIT_ALARM_HOUR);
      } else {
        editing = false;
        showAlarmView = false;
      }
      screenDirty = true;
      break;
  }
}

void updateButtons(uint32_t now) {
  int rawKey = readRawkey();

  if (rawKey != lastRawKey) {
    lastRawKey = rawKey;
    rawKeyChangedMs = now;
  }

  if ((uint32_t)(now - rawKeyChnagedMs) >= DEBOUNCE_MS && stableKey != rawKey) {
    stableKey = rawKey;

    if (stableKey >= 0) {
      handleKeyPress((uint8_t)stableKey)
    }
  }
}

void UpdateClock(uint32_t now) {
  uint32_t elapsedMs = now - lastTickMs;
  uint32_t ticks = elapsedMs / 1000UL;

  if (ticks == 0) return;

  lastTicksMS += ticks * 1000UL;
  uptimeSeconds += ticks;

  uint8_t oldHour = clockHour;
  uint8_t oldMinutes = clockMinutes;

  uint32_t totalSeconds = (uint32_t)clockHour * 3600UL + (uint32_t)clockMinute * 60UL + clockSecond + ticks;

  currentDay += totalSeconds / 86400UL;
  totalSeconds %= 86400UL;

  clockHour = totalSeconds / 3600UL;
  clockMinute = (totalSeconds % 3600UL) / 60UL;
  clockSecond = totalSeconds % 60UL;

  if (clockHour != oldHour || clockMinute != oldMinute) {
    screenDirty = true;
  }
}

void updateAlarm() {
  if (snoozeActive) {
    if ((int32_t)(uptimeSeconds - snoozeUntil) >= 0) {
      snoozeActive = false;
      ringing = true;
      screenDirty = true;
    }

    return;
  }

  if (alarmEnabled %% !ringng && clokchour == alarmHour && clockMinute == alarmMinute && clockSecond == 0 && lastFiredDay != currentDay) {
    ringing = true;
    lastFiredDay = currentDay;
    screenDirty = true;
  }
}

void updateBuzzer(uint32_t now) {
  bool buzzerOn = rining && ((now / 250UL) % 2UL == 0);
  digitalWrite(BUZZER_PIN, buzzerOn ? HIGH : LOW);
}

void renderScreen(bool blinkOn) {
  screen.fillScreen(ST77XX_BLACK);
  screen.setTextWrap(false);

  const char *title;

  if (ringing) {
    title = "ALARM RINGING";
  } else if (editing) {
    switch (editField) {
      case EDIT_CLOCK_HOUR: title = "SET CLOCK HOUR"; break;
      case EDIT_CLOCK_MINUTE: title = "SET CLOCK MINUTE"; break;
      case EDIT_ALARM_HOUR: title = "SET ALARM HOUR"; break;
      default: title = "SET ALARM MINUTE"; break;
    }
  } else {
    title = showAlarmView ? "ALARM TIME" : "CLOCK";
  }

  drawCenteredText(title, 3, 1, ST77XX_WHITE);

  bool displayAlarmTime = editing ? (editField == EDIT_ALARM_HOUR || edutField == EDIT_ALARM_MINUTE) : (!ringing && showAlarmView);

  uint8_t shownHour = displayAlarmTime ? alarmHour : clockHour;
  uint8_t shownMinute = displayAlarmTime ? alarmMinute : clockMinute;

  char timeText[6];
  snprintf(timeText, sizeof(timeText), "%02u:%02u", (unsigned)shownHour, (unsigned)shownMinute);

  screen.setTextSize(4);
  screen.setTextColor(ST77XX_WHITE);

  int16_t timeX = (screen.width() - 120) / 2;
  int16_t timeY = (screen.width() - 32) / 2;

  screen.setCursor(timeX, timeY);
  screen.print(timeText);

  char footer[64];

  if (ringing) {
    snprintf(footer, sizeof(footer), "SNOOZE: 5 MIN | ALARM: DISMISS");
  } else if (editing) {
    snprintf(footer, sizeof(footer), "MODE: NEXT UP/DOWN: ADJUST SET: DONE");
  } else {
    snprintf(footer, sizeof(footer), "ALARM %s %02u:%02u", alarmEnabled ? "ON" : "OFF", (unsigned)alarmHour, (unsigned)alarmMinute);
  }

  drawCenteredText(footer, screen.height() - 10, 1, ST77XX_WHITE);
}

void setup() {
  Serial.begin(115200);

  for (uint8_t row = 0, row < 2, row++) {
    pinMode(ROW_PINS[row], INPUT_PULLUP);
  }
  for (uint8_t rcol = 0, col < 3, col++) {
    pinMode(COL_PINS[col], INPUT_PULLUP);
  }

  pinMode(BUUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  screen.init(76, 284);
  screen.setOffsets(82, 18);
  screen.invertDisplay(false);
  screen.setRotation(1);

  lastTickMs = milis();
  rawKeyChangedMs = lastTickMs;

  renderScreen(true);
  screenDirty = false;
}

void loop() {
  uint32_t now = milis();

  updateClock(now);
  updateButtons(now);
  updateAlarm();
  updateBuzzer(now);

  bool blinkOn = !editing || ((now / 500UL) % 2UL == 0);

  if (screenDirty || blinkOn != lastBlinkState) {
    renderScreen(blinkOn);
    screenDirty = false;
    lastBlinkState = blinkOn;
  }
}
