// =================================================================
// ==           LASER TAG GAME - ADVANCED GAME MODES            ==
// =================================================================

// --- Pin Definitions ---
const int _laser = 2;
const int _irSensor = 3;
const int _irLed = 4;
const int _trigger = 5;
const int _statusLed = 13; // Using the built-in LED for status effects

// --- Game Mode Selection ---
// An enum to define the overall game rules.
// SET THE GAME MODE HERE!
enum GameMode {
  SHOT,    // Infinite bullets, 1 life.
  RELOAD,  // 3 lives, 2 bullets per clip.
  HEALED   // 5 lives, 6 bullets, can heal over time.
};
const GameMode currentMode = HEALED; // <<< CHOOSE YOUR GAME MODE HERE

// --- Game State Enum ---
// An enum for the player's current status. We add HEALING for the new mode.
enum PlayerState {
  ACTIVE,
  RELOADING,
  HEALING,  // New state for the HEALED game mode
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
unsigned long eventStartTime; // Used for reloading, death, and healing timers
unsigned long lastBlinkTime;
unsigned long immunityStartTime; // Used for post-hit immunity

// --- Durations (in milliseconds) ---
const unsigned long RELOAD_DURATION = 3000;       // 3 seconds to reload
const unsigned long DEATH_BLINK_INTERVAL = 150;     // Blink fast when dead
const unsigned long RELOAD_BLINK_INTERVAL = 400;    // Blink slower when reloading
const unsigned long HEAL_BLINK_INTERVAL = 600;      // Blink very slowly when healing
const unsigned long HIT_IMMUNITY_DURATION = 500;    // 0.5 second of immunity after being hit
const unsigned long HEAL_DURATION = 15000;      // 15 seconds to wait for a heal

// =================================================================
// ==                          SETUP                              ==
// =================================================================
void setup() {
  pinMode(_laser, OUTPUT);
  pinMode(_irSensor, INPUT); 
  pinMode(_irLed, OUTPUT); 
  pinMode(_trigger, INPUT_PULLUP); // Use INPUT_PULLUP to avoid a floating pin
  pinMode(_statusLed, OUTPUT);

  Serial.begin(9600);
  Serial.println("\n--- Laser Tag Initializing ---");

  // Configure game variables based on the selected GameMode
  switch (currentMode) {
    case SHOT:
      maxHealth = 1;
      maxBullets = 999; // Represents infinite bullets
      Serial.println("Game Mode: SHOT (1 Life, Infinite Ammo)");
      break;
    case RELOAD:
      maxHealth = 3;
      maxBullets = 2;
      Serial.println("Game Mode: RELOAD (3 Lives, 2-Bullet Clip)");
      break;
    case HEALED:
      maxHealth = 5;
      maxBullets = 6;
      Serial.println("Game Mode: HEALED (5 Lives, 6-Bullet Clip, Healing Enabled)");
      break;
  }
  
  // Initialize player stats
  health = maxHealth;
  bullets = maxBullets;
  
  digitalWrite(_laser, HIGH); // Start with the laser on
}

// =================================================================
// ==             MAIN LOOP: THE STATE MACHINE                    ==
// =================================================================
void loop() {
  // The loop dispatches to the correct handler based on the current state.
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

// =================================================================
// ==                  STATE HANDLER FUNCTIONS                    ==
// =================================================================

// Runs when the player is healthy and can shoot.
void handleActiveState() {
  // Check for being shot
  if (digitalRead(_irSensor) == HIGH) {
    getHit(); // Handle the logic for being hit
    return; // Exit function immediately after handling the hit
  }
  
  // Check for shooting
  if (digitalRead(_trigger) == LOW && bullets > 0) { // LOW because of INPUT_PULLUP
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

// Runs while the player is waiting for bullets to reload.
void handleReloadingState() {
  // Blinking LED to show reloading is in progress
  if (millis() - lastBlinkTime > RELOAD_BLINK_INTERVAL) {
    digitalWrite(_statusLed, !digitalRead(_statusLed)); // Toggle the LED
    lastBlinkTime = millis(); // Reset the blink timer
  }

  // Check if reloading is finished
  if (millis() - eventStartTime >= RELOAD_DURATION) {
    Serial.println("Reload complete!");
    bullets = maxBullets;
    digitalWrite(_statusLed, LOW); // Turn off status LED
    currentState = ACTIVE;
    digitalWrite(_laser, HIGH); // Turn laser back on
    return;
  }

  // You can still be shot while reloading!
  if (digitalRead(_irSensor) == HIGH) {
    getHit();
  }
}

// **NEW** Runs when the player is waiting to heal in HEALED mode.
void handleHealingState() {
  // --- VISUAL INDICATOR (Slow Blink) ---
  if (millis() - lastBlinkTime > HEAL_BLINK_INTERVAL) {
    digitalWrite(_statusLed, !digitalRead(_statusLed));
    lastBlinkTime = millis();
  }
  
  // --- CHECK FOR HEALING COMPLETION ---
  if (millis() - eventStartTime >= HEAL_DURATION) {
    health++; // Gain one health back
    if (health > maxHealth) health = maxHealth; // Clamp to max health
    
    Serial.print("Healed! Health is now: ");
    Serial.println(health);
    
    digitalWrite(_statusLed, LOW);
    digitalWrite(_laser, HIGH); // Player can shoot again
    currentState = ACTIVE;
    return;
  }

  // --- CHECK FOR TIMER-RESETTING INTERRUPTIONS ---
  // If shot at during healing, the timer resets but you take no damage.
  if (digitalRead(_irSensor) == HIGH) {
    Serial.println("Healing interrupted by enemy fire! Timer reset.");
    eventStartTime = millis(); // Reset the healing timer
    // Give a quick visual flash to show interruption
    digitalWrite(_statusLed, HIGH);
    delay(100);
    // The blinking will resume on the next loop pass
  }
  
  // If player tries to shoot, the timer also resets.
  if (digitalRead(_trigger) == LOW) {
    Serial.println("Cannot shoot while healing! Timer reset.");
    eventStartTime = millis(); // Reset the healing timer
    // Give a quick visual flash
    digitalWrite(_statusLed, HIGH);
    delay(100);
  }
}

// Runs when player health is 0.
void handleDeadState() {
  // Fast blinking LED to show the player is out of the game.
  if (millis() - lastBlinkTime > DEATH_BLINK_INTERVAL) {
    digitalWrite(_statusLed, !digitalRead(_statusLed)); // Toggle LED
    lastBlinkTime = millis();
  }
  // The player stays in this state until the Arduino is reset.
}

// =================================================================
// ==                     HELPER FUNCTIONS                        ==
// =================================================================

void shoot() {
  Serial.print("FIRE! Bullets left: ");

  // In SHOT mode, bullets are infinite and do not decrease.
  if (currentMode != SHOT) {
    bullets--;
  }
  Serial.println(bullets);
  
  digitalWrite(_irLed, HIGH);
  delay(30); // A short delay for the IR pulse is fine here.
  digitalWrite(_irLed, LOW);
}

void getHit() {
  // If we were just hit, do nothing (provides brief immunity)
  if (millis() - immunityStartTime < HIT_IMMUNITY_DURATION) return;

  health--;
  immunityStartTime = millis(); // Start the immunity timer
  
  Serial.print("HIT! Health left: ");
  Serial.println(health);

  // Turn things off briefly to indicate a hit
  digitalWrite(_laser, LOW);
  digitalWrite(_statusLed, HIGH);
  delay(200);
  digitalWrite(_statusLed, LOW);

  // If player is still alive, decide what to do next
  if (health > 0) {
    // If in HEALED mode, transition to the HEALING state
    if (currentMode == HEALED) {
      Serial.println("Healing protocol initiated. Wait 15 seconds without interruption.");
      currentState = HEALING;
      eventStartTime = millis(); // Start the healing timer
      lastBlinkTime = eventStartTime;
      // Laser remains off because you can't shoot while healing
    } else {
      // For all other modes, just turn the laser back on if not reloading.
      if (currentState != RELOADING) {
        digitalWrite(_laser, HIGH);
      }
    }
  } else { // health <= 0
    // Health is zero, transition to DEAD state
    Serial.println("You are dead!");
    currentState = DEAD;
    eventStartTime = millis();
    lastBlinkTime = eventStartTime;
    // Turn off all outputs permanently
    digitalWrite(_laser, LOW);
    digitalWrite(_irLed, LOW);
  }
}
