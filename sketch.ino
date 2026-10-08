#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

#define OLED_SCL 6
#define OLED_SDA 7

#define BUTTON_1 3
#define BUTTON_2 4
#define BUTTON_3 5

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

enum AppState { STATE_MENU, STATE_COUNTERS, STATE_STOPWATCH, STATE_REACTION };
AppState currentState = STATE_MENU;

enum IconType { ICON_UP, ICON_DOWN, ICON_SELECT, ICON_PLUS, ICON_PLAY, ICON_PAUSE, ICON_RESET, ICON_EXIT, ICON_READY, ICON_NONE };

int menuIndex = 0;

int count1 = 0;
int count2 = 0;

unsigned long swStartTime = 0;
unsigned long swElapsedTime = 0;
bool swRunning = false;

enum ReactionState { RX_IDLE, RX_WAITING, RX_GO, RX_RESULT, RX_EARLY };
ReactionState rxState = RX_IDLE;

unsigned long rxTargetTime = 0;
unsigned long rxStartTime = 0;
unsigned long rxScoreMs = 0;

bool lastBtn1 = HIGH;
bool lastBtn2 = HIGH;
bool lastBtn3 = HIGH;

void setup() {
  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  pinMode(BUTTON_3, INPUT_PULLUP);

  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
      for (;;);
    }
  }

  randomSeed(analogRead(0) + micros());

  display.setTextWrap(false);
  updateDisplay();
}

void loop() {
  bool btn1 = digitalRead(BUTTON_1);
  bool btn2 = digitalRead(BUTTON_2);
  bool btn3 = digitalRead(BUTTON_3);

  bool needRedraw = false;
  unsigned long now = millis();

  if (btn1 == LOW && lastBtn1 == HIGH) {
    if (currentState == STATE_MENU) {
      menuIndex = (menuIndex - 1 + 3) % 3;
      needRedraw = true;
    } else if (currentState == STATE_COUNTERS) {
      count1++;
      needRedraw = true;
    } else if (currentState == STATE_STOPWATCH) {
      if (swRunning) {
        swRunning = false;
        swElapsedTime = now - swStartTime;
      } else {
        swRunning = true;
        swStartTime = now - swElapsedTime;
      }
      needRedraw = true;
    } else if (currentState == STATE_REACTION) {
      if (rxState == RX_WAITING) {
        rxState = RX_EARLY;
        needRedraw = true;
      } else if (rxState == RX_GO) {
        rxScoreMs = now - rxStartTime;
        rxState = RX_RESULT;
        needRedraw = true;
      }
    }
  }

  if (btn2 == LOW && lastBtn2 == HIGH) {
    if (currentState == STATE_MENU) {
      menuIndex = (menuIndex + 1) % 3;
      needRedraw = true;
    } else if (currentState == STATE_COUNTERS) {
      count2++;
      needRedraw = true;
    } else if (currentState == STATE_STOPWATCH) {
      swRunning = false;
      swElapsedTime = 0;
      needRedraw = true;
    } else if (currentState == STATE_REACTION) {
      if (rxState == RX_IDLE || rxState == RX_RESULT || rxState == RX_EARLY) {
        rxState = RX_WAITING;
        rxTargetTime = now + random(2000, 5001);
        needRedraw = true;
      }
    }
  }

  if (btn3 == LOW && lastBtn3 == HIGH) {
    if (currentState == STATE_MENU) {
      if (menuIndex == 0) currentState = STATE_COUNTERS;
      else if (menuIndex == 1) currentState = STATE_STOPWATCH;
      else if (menuIndex == 2) {
        currentState = STATE_REACTION;
        rxState = RX_IDLE;
      }
    } else {
      currentState = STATE_MENU;
    }
    needRedraw = true;
  }

  lastBtn1 = btn1;
  lastBtn2 = btn2;
  lastBtn3 = btn3;

  if (currentState == STATE_REACTION && rxState == RX_WAITING) {
    if (now >= rxTargetTime) {
      rxState = RX_GO;
      rxStartTime = now;
      needRedraw = true;
    }
  }

  if (currentState == STATE_STOPWATCH && swRunning) {
    needRedraw = true;
  }

  if (needRedraw) {
    updateDisplay();
  }

  delay(10);
}

void drawIcon(int x, int y, IconType icon) {
  switch (icon) {
    case ICON_UP:
      display.fillTriangle(x + 3, y, x, y + 5, x + 6, y + 5, SSD1306_WHITE);
      break;
    case ICON_DOWN:
      display.fillTriangle(x, y, x + 6, y, x + 3, y + 5, SSD1306_WHITE);
      break;
    case ICON_SELECT:
      display.fillTriangle(x, y, x, y + 6, x + 5, y + 3, SSD1306_WHITE);
      break;
    case ICON_PLUS:
      display.drawFastHLine(x, y + 3, 7, SSD1306_WHITE);
      display.drawFastVLine(x + 3, y, 7, SSD1306_WHITE);
      break;
    case ICON_PLAY:
      display.fillTriangle(x + 1, y, x + 1, y + 6, x + 6, y + 3, SSD1306_WHITE);
      break;
    case ICON_PAUSE:
      display.fillRect(x + 1, y, 2, 6, SSD1306_WHITE);
      display.fillRect(x + 5, y, 2, 6, SSD1306_WHITE);
      break;
    case ICON_RESET:
      display.drawCircle(x + 3, y + 3, 3, SSD1306_WHITE);
      display.drawPixel(x + 3, y + 3, SSD1306_WHITE);
      break;
    case ICON_EXIT:
      display.fillTriangle(x, y + 3, x + 4, y, x + 4, y + 6, SSD1306_WHITE);
      display.drawFastHLine(x + 3, y + 3, 4, SSD1306_WHITE);
      break;
    case ICON_READY:
      display.drawRect(x, y + 1, 6, 5, SSD1306_WHITE);
      display.drawPixel(x + 3, y + 3, SSD1306_WHITE);
      break;
    case ICON_NONE:
      break;
  }
}

void drawBottomFooter(IconType icon1, IconType icon2, IconType icon3) {
  if (icon1 != ICON_NONE) drawIcon(18, 56, icon1);
  if (icon2 != ICON_NONE) drawIcon(60, 56, icon2);
  if (icon3 != ICON_NONE) drawIcon(102, 56, icon3);
}

void renderMenu() {
  display.setTextSize(1);
  display.setCursor(14, 8);
  if (menuIndex == 0) display.print("> "); else display.print("  ");
  display.print("1. Counters");

  display.setCursor(14, 22);
  if (menuIndex == 1) display.print("> "); else display.print("  ");
  display.print("2. Stopwatch");

  display.setCursor(14, 36);
  if (menuIndex == 2) display.print("> "); else display.print("  ");
  display.print("3. Reaction Test");

  drawBottomFooter(ICON_UP, ICON_DOWN, ICON_SELECT);
}

void renderCountersApp() {
  display.setTextSize(1);
  display.setCursor(32, 4);
  display.print("2x COUNTERS");

  display.setCursor(12, 26);
  display.print("C1: "); display.print(count1);

  display.setCursor(72, 26);
  display.print("C2: "); display.print(count2);

  drawBottomFooter(ICON_PLUS, ICON_PLUS, ICON_EXIT);
}

void renderStopwatchApp() {
  display.setTextSize(1);
  display.setCursor(38, 4);
  display.print("STOPWATCH");

  unsigned long totalMs = swRunning ? (millis() - swStartTime) : swElapsedTime;
  unsigned int mins = (totalMs / 60000) % 60;
  unsigned int secs = (totalMs / 1000) % 60;
  unsigned int hundredths = (totalMs / 10) % 100;

  display.setTextSize(2);
  display.setCursor(14, 24);

  if (mins < 10) display.print("0");
  display.print(mins);
  display.print(":");

  if (secs < 10) display.print("0");
  display.print(secs);
  display.print(".");

  if (hundredths < 10) display.print("0");
  display.print(hundredths);

  drawBottomFooter(swRunning ? ICON_PAUSE : ICON_PLAY, ICON_RESET, ICON_EXIT);
}

void renderReactionApp() {
  if (rxState == RX_IDLE) {
    display.setTextSize(1);
    display.setCursor(24, 4);
    display.print("REACTION TEST");

    display.setCursor(22, 26);
    display.print("Press ");
    drawIcon(58, 26, ICON_READY);
    display.setCursor(68, 26);
    display.print(" to Start");

    drawBottomFooter(ICON_NONE, ICON_READY, ICON_EXIT);
  } 
  else if (rxState == RX_WAITING) {
    display.setTextSize(2);
    display.setCursor(24, 24);
    display.print("WAIT...");
  } 
  else if (rxState == RX_GO) {
    display.fillRect(0, 0, 128, 64, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(28, 24);
    display.print("PRESS!");
  } 
  else if (rxState == RX_RESULT) {
    display.setTextSize(1);
    display.setCursor(28, 4);
    display.print("YOUR TIME:");

    display.setTextSize(2);
    display.setCursor(24, 24);
    display.print(rxScoreMs);
    display.print(" ms");

    drawBottomFooter(ICON_NONE, ICON_READY, ICON_EXIT);
  } 
  else if (rxState == RX_EARLY) {
    display.setTextSize(2);
    display.setCursor(4, 24);
    display.print("TOO EARLY!");

    drawBottomFooter(ICON_NONE, ICON_READY, ICON_EXIT);
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (currentState == STATE_MENU) {
    renderMenu();
  } else if (currentState == STATE_COUNTERS) {
    renderCountersApp();
  } else if (currentState == STATE_STOPWATCH) {
    renderStopwatchApp();
  } else if (currentState == STATE_REACTION) {
    renderReactionApp();
  }

  display.display();
}