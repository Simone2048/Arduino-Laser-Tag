// Pin Definitions
const int _laser = 2;
const int _irSensor = 3;
const int _irLed = 4;
const int _trigger = 5;
const int _statusLed = 13; // Using the built-in LED for status effects

// Game State Enum
// An enum makes the code much cleaner than using multiple boolean flags.
enum PlayerState {
  ACTIVE,
  RELOADING,
  DEAD
};
PlayerState currentState = ACTIVE;

// Game Settings
bool kidsMode = false;
const int MAX_HEALTH = 3;
const int MAX_BULLETS = 3;

// Game Variables
int health = MAX_HEALTH;
int bullets = MAX_BULLETS;

// Timers for non-blocking events
unsigned long eventStartTime;
unsigned long lastBlinkTime;

// Durations (in milliseconds)
const unsigned long RELOAD_DURATION = 3000; // 3 seconds to reload
const unsigned long DEATH_BLINK_INTERVAL = 150; // Blink fast when dead
const unsigned long RELOAD_BLINK_INTERVAL = 400; // Blink slower when reloading
const unsigned long HIT_IMMUNITY_DURATION = 500; // 0.5 second of immunity after being hit

void setup() {
  pinMode(_laser, OUTPUT);
  pinMode(_irSensor, INPUT); 
  pinMode(_irLed, OUTPUT); 
  pinMode(_trigger, INPUT_PULLUP); // Use INPUT_PULLUP for triggers to avoid a floating pin
  pinMode(_statusLed, OUTPUT);

  Serial.begin(9600);

  // Start with the laser on if not in kids mode
  if (!kidsMode) {
    digitalWrite(_laser, HIGH);
  }
}

// --- Main Loop: The State Machine Dispatcher ---
void loop() {
  // The loop's only job is to call the correct handler
  // based on the current state.
  switch (currentState) {
    case ACTIVE:
      handleActiveState();
      break;
    case RELOADING:
      handleReloadingState();
      break;
    case DEAD:
      handleDeadState();
      break;
  }
}

// --- State Handler Functions ---

// This runs when the player is healthy and has bullets.
void handleActiveState() {
  // Check for being shot
  if (digitalRead(_irSensor) == HIGH) {
    getHit(); // Handle the logic for being hit
    return; // Exit function immediately after getting hit
  }
  
  // Check for shooting
  // Note: digitalRead is LOW when a button connected to ground is pressed with INPUT_PULLUP
  if (digitalRead(_trigger) == HIGH && bullets > 0) {
    shoot();
    if (bullets == 0) {
      // Out of bullets, transition to RELOADING state
      Serial.println("Out of bullets! Reloading...");
      currentState = RELOADING;
      eventStartTime = millis(); // Record when reloading starts
      lastBlinkTime = eventStartTime;
      digitalWrite(_laser, LOW); // Turn off laser while reloading
    }
  }
}

// This runs while the player is waiting for bullets to reload.
void handleReloadingState() {
  // --- ASYNC LOADING SCREEN EFFECT (Blinking LED) ---
  // Check if it's time to toggle the status LED
  if (millis() - lastBlinkTime > RELOAD_BLINK_INTERVAL) {
    digitalWrite(_statusLed, !digitalRead(_statusLed)); // Toggle the LED
    lastBlinkTime = millis(); // Reset the blink timer
  }

  // --- CHECK IF RELOADING IS FINISHED ---
  if (millis() - eventStartTime >= RELOAD_DURATION) {
    // Reloading complete!
    Serial.println("Reload complete!");
    bullets = MAX_BULLETS;
    digitalWrite(_statusLed, LOW); // Turn off status LED
    currentState = ACTIVE; // Go back to the active state
    if (!kidsMode) {
      digitalWrite(_laser, HIGH); // Turn laser back on
    }
    return;
  }

  // IMPORTANT: You can still be shot while reloading!
  if (digitalRead(_irSensor) == HIGH) {
    getHit();
  }
}

// This runs when player health is 0.
void handleDeadState() {
  // --- ASYNC DEATH SCREEN EFFECT (Fast Blinking LED) ---
  // The player is "dead" and can't do anything but show a visual effect.
  if (millis() - lastBlinkTime > DEATH_BLINK_INTERVAL) {
    digitalWrite(_statusLed, !digitalRead(_statusLed)); // Toggle LED
    lastBlinkTime = millis();
  }
  // The player stays in this state forever (or until reset).
}


// --- Helper Functions ---

void shoot() {
  Serial.print("FIRE! Bullets left: ");
  Serial.println(bullets - 1);
  
  digitalWrite(_irLed, HIGH);
  delay(30); // A short delay for the IR pulse is fine, as it's not user-facing
  digitalWrite(_irLed, LOW);
  bullets -= 1;
}

void getHit() {
  // If we were just hit, do nothing (provides brief immunity)
  if (currentState == RELOADING && millis() - eventStartTime < HIT_IMMUNITY_DURATION) return;
  if (currentState == ACTIVE && millis() - lastBlinkTime < HIT_IMMUNITY_DURATION && health < MAX_HEALTH) return;

  health -= 1;
  lastBlinkTime = millis(); // Use lastBlinkTime to track the hit for immunity
  Serial.print("HIT! Health left: ");
  Serial.println(health);

  // Turn things off briefly to indicate a hit
  digitalWrite(_laser, LOW);
  digitalWrite(_statusLed, HIGH);
  delay(200); // A short visual effect delay is okay here
  digitalWrite(_statusLed, LOW);
  if (currentState != RELOADING && !kidsMode) { // Don't turn laser back on if reloading
      digitalWrite(_laser, HIGH);
  }
  
  if (health <= 0) {
    // Health is zero, transition to DEAD state
    Serial.println("You are dead!");
    currentState = DEAD;
    eventStartTime = millis(); // Record when death occurs
    lastBlinkTime = eventStartTime;
    // Turn off all outputs permanently
    digitalWrite(_laser, LOW);
    digitalWrite(_irLed, LOW);
  }
}
