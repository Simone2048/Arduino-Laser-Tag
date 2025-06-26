#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>


// --- Pin Definitions ---
const int _laser = 2;
const int _irSensor = 3;
const int _irLed = 4;
const int _trigger = 5;
const int _statusLed = 13;

// --- Display Configuration ---
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Game Mode Selection ---
enum GameMode {
  SHOT,    // Infinite bullets, 1 life.
  RELOAD,  // 3 lives, 2 bullets per clip.
  HEALED   // 5 lives, 6 bullets, can heal over time.
};
const GameMode currentMode = HEALED;

// --- Game State Enum ---
enum PlayerState {
  ACTIVE,
  RELOADING,
  HEALING,
  DEAD
};
PlayerState currentState = ACTIVE;

// --- Game Settings (will be configured in setup) ---
int maxHealth;
int maxBullets;

// --- Game Variables ---
int health;
int bullets;

// --- Timers for non-blocking events ---
unsigned long eventStartTime; 
unsigned long lastBlinkTime;
unsigned long immunityStartTime;

// --- Durations (in milliseconds) ---
const unsigned long RELOAD_DURATION = 3000;
const unsigned long DEATH_BLINK_INTERVAL = 150;
const unsigned long RELOAD_BLINK_INTERVAL = 400;
const unsigned long HEAL_BLINK_INTERVAL = 600;
const unsigned long HIT_IMMUNITY_DURATION = 500;
const unsigned long HEAL_DURATION = 15000;

void setup() {
  pinMode(_laser, OUTPUT);
  pinMode(_irSensor, INPUT); 
  pinMode(_irLed, OUTPUT); 
  pinMode(_trigger, INPUT_PULLUP);
  pinMode(_statusLed, OUTPUT);

  Serial.begin(9600);
  Serial.println("\n--- Laser Tag Initializing ---");

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  
  switch (currentMode) {
    case SHOT:
      maxHealth = 1;
      maxBullets = 999;
      break;
    case RELOAD:
      maxHealth = 3;
      maxBullets = 2;
      break;
    case HEALED:
      maxHealth = 5;
      maxBullets = 6;
      break;
  }
  
  // Initialize player stats
  health = maxHealth;
  bullets = maxBullets;
  
  digitalWrite(_laser, HIGH);

  // Show initial status on the display
  updateDisplay(); 
}

void loop() {
  switch (currentState) {
    case ACTIVE:
      handleActiveState();
      break;
    case RELOADING:
      handleReloadingState();
      break;
    case HEALING:
      handleHealingState();
      break;
    case DEAD:
      handleDeadState();
      break;
  }
}


// Centrl function to draw everything on the OLED screen
void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Health: ");
  display.print(health);
  display.print("/");
  display.print(maxHealth);


  display.setCursor(90, 56);
  display.print("Ammo: ");
  if (currentMode == SHOT) {
    display.print("INF");
  } else {
    display.print(bullets);
    display.print("/");
    display.print(maxBullets);
  }
  String statusText = "";
  display.setTextSize(2);

  switch (currentState) {
    case ACTIVE:
      statusText = "ACTIVE";
      break;
    case RELOADING:
      statusText = "RELOAD";
      { // Use braces to create a local scope for variables
        unsigned long elapsedTime = millis() - eventStartTime;
        float progress = (float)elapsedTime / (float)RELOAD_DURATION;
        int barWidth = constrain(progress * SCREEN_WIDTH, 0, SCREEN_WIDTH);
        display.fillRect(0, 32, barWidth, 8, SSD1306_WHITE); // Progress bar
      }
      break;
    case HEALING:
      statusText = "HEALING";
      {
        unsigned long elapsedTime = millis() - eventStartTime;
        float progress = (float)elapsedTime / (float)HEAL_DURATION;
        int barWidth = constrain(progress * SCREEN_WIDTH, 0, SCREEN_WIDTH);
        display.drawRect(0, 32, SCREEN_WIDTH, 8, SSD1306_WHITE); // Outline
        display.fillRect(0, 32, barWidth, 8, SSD1306_WHITE);     // Filled bar
      }
      break;
    case DEAD:
      statusText = "- DEAD -";
      break;
  }
  
  // Center the status text
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(statusText, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 16);
  display.print(statusText);

  display.display();
}


void handleActiveState() {
  if (digitalRead(_irSensor) == HIGH) {
    getHit();
    return;
  }
  
  // CORRECTED: A pressed button with INPUT_PULLUP reads LOW
  if (digitalRead(_trigger) == LOW && bullets > 0) { 
    shoot();
    if (bullets == 0 && currentMode != SHOT) {
      currentState = RELOADING;
      eventStartTime = millis();
      lastBlinkTime = eventStartTime;
      digitalWrite(_laser, LOW);
      updateDisplay(); // Update display to show "RELOADING"
    }
  }
}

void handleReloadingState() {
  updateDisplay();
  
  if (millis() - lastBlinkTime > RELOAD_BLINK_INTERVAL) {
    digitalWrite(_statusLed, !digitalRead(_statusLed));
    lastBlinkTime = millis();
  }

  // Check if reloading is finished
  if (millis() - eventStartTime >= RELOAD_DURATION) {
    bullets = maxBullets;
    digitalWrite(_statusLed, LOW);
    currentState = ACTIVE;
    digitalWrite(_laser, HIGH);
    updateDisplay(); 
    return;
  }

  if (digitalRead(_irSensor) == HIGH) {
    getHit();
  }
}

void handleHealingState() {
  updateDisplay();

  if (millis() - eventStartTime >= HEAL_DURATION) {
    health++;
    if (health > maxHealth) health = maxHealth;
    
    digitalWrite(_statusLed, LOW);
    digitalWrite(_laser, HIGH);
    currentState = ACTIVE;
    updateDisplay(); 
    return;
  }

  if (digitalRead(_irSensor) == HIGH) {
    eventStartTime = millis();
    digitalWrite(_statusLed, HIGH); delay(100); digitalWrite(_statusLed, LOW);
  }
  
  // If player tries to shoot, reset timer
  if (digitalRead(_trigger) == LOW) {
    eventStartTime = millis();
    digitalWrite(_statusLed, HIGH); delay(100); digitalWrite(_statusLed, LOW);
  }
}

void handleDeadState() {
  // Fast blinking LED to show the player is out
  if (millis() - lastBlinkTime > DEATH_BLINK_INTERVAL) {
    digitalWrite(_statusLed, !digitalRead(_statusLed));
    lastBlinkTime = millis();
  }
}


void shoot() {
  if (currentMode != SHOT) {
    bullets--;
  }
  updateDisplay(); // Update ammo count on screen immediately
  
  digitalWrite(_irLed, HIGH);
  delay(30); 
  digitalWrite(_irLed, LOW);
}

void getHit() {
  if (millis() - immunityStartTime < HIT_IMMUNITY_DURATION) return;

  health--;
  immunityStartTime = millis();
  
  digitalWrite(_laser, LOW);
  digitalWrite(_statusLed, HIGH);
  delay(200);
  digitalWrite(_statusLed, LOW);

  if (health > 0) {
    if (currentMode == HEALED) {
      currentState = HEALING;
      eventStartTime = millis();
      lastBlinkTime = eventStartTime;
    } else {
      if (currentState != RELOADING) {
        digitalWrite(_laser, HIGH);
      }
    }
  } else { // health <= 0
    currentState = DEAD;
    eventStartTime = millis();
    lastBlinkTime = eventStartTime;
    digitalWrite(_laser, LOW);
    digitalWrite(_irLed, LOW);
  }
  
  updateDisplay(); // Update health and state on screen after a hit
}
