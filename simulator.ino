// NOTE: switch prep is ON when status=0 (active=0, inactive=1)
// NOTE: switch expose is a toggle switch (active=1, inactive=0)

// interrupt inputs
const byte ready_interrupt21 = 21;
const byte switch_prep_interrupt3 = 3;
const byte switch_expose_interrupt2 = 2;
const byte exp_win_interrupt20 = 20;
const byte motion_stop_interrupt19 = 19;

// digital inputs
const byte motion_ready = 5;
const byte detector_ready = 6;
const byte xray_ready = 7;
const byte motion_stopped = 8;
const byte det_exp_win = 9;

// digital outputs
const byte motion_move = 30;
const byte detector_prep = 31;
const byte xray_prep = 32;
const byte xray_exp = 33;

// counting
static int counter = 0;
static int maxCount = 15; // 2D MODE = 1, TOMO MODE = 15

// debouncing
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers


// flag to keep track of whether it is the first acqusition or not
static bool seen_motion_stopped = false; 

// include the library code:
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(42, 43, 44, 45, 46, 47);


void setup() {
  Serial.begin(9600);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("Ready:");

  // all input pins
  pinMode(ready_interrupt21, INPUT_PULLUP);
  pinMode(switch_prep_interrupt3, INPUT_PULLUP);
  pinMode(switch_expose_interrupt2, INPUT_PULLUP);
  pinMode(motion_ready, INPUT_PULLUP);
  pinMode(detector_ready, INPUT_PULLUP);
  pinMode(xray_ready, INPUT_PULLUP);
  pinMode(exp_win_interrupt20, INPUT_PULLUP);
  pinMode(motion_stopped, INPUT_PULLUP);
  pinMode(det_exp_win, INPUT_PULLUP);
  pinMode(motion_stop_interrupt19, INPUT_PULLUP);

  // all interrupt pins
  attachInterrupt(digitalPinToInterrupt(ready_interrupt21), combined_ready, CHANGE);
  attachInterrupt(digitalPinToInterrupt(switch_prep_interrupt3), switch_prep, CHANGE);
  attachInterrupt(digitalPinToInterrupt(motion_stop_interrupt19), motion_stop_action, RISING);

  attachInterrupt(digitalPinToInterrupt(switch_expose_interrupt2), switch_expose, CHANGE);
  attachInterrupt(digitalPinToInterrupt(exp_win_interrupt20), exp_win_action, CHANGE);

  // all output pins
  pinMode(detector_prep, OUTPUT);
  pinMode(motion_move, OUTPUT);
  pinMode(xray_prep, OUTPUT);
  pinMode(xray_exp, OUTPUT);

  // write to output pins
  digitalWrite(motion_move, HIGH);
  digitalWrite(detector_prep, HIGH);
  digitalWrite(xray_prep, HIGH);
  digitalWrite(xray_exp, HIGH);

  restart();
}

void loop() {
//  Serial.println(digitalRead(switch_prep_interrupt3));
//    Serial.println("----------------------------------------");
//    Serial.print("Ready: ");
//    Serial.println(digitalRead(ready_interrupt21));
//    Serial.print("Switches: ");
//    Serial.print(digitalRead(switch_prep_interrupt3));
//    Serial.println(digitalRead(switch_expose_interrupt2));
//    Serial.print("Motion Ready: ");
//    Serial.println(digitalRead(motion_ready));
//    Serial.print("Det Ready: ");
//    Serial.println(digitalRead(detector_ready));
//    Serial.print("X-Ray Ready: ");
//    Serial.println(digitalRead(xray_ready));
//    Serial.print("Motion Stopped: ");
//    Serial.println(digitalRead(motion_stopped));
//    Serial.print("Det Exp Win: ");
//    Serial.println(digitalRead(det_exp_win));
//    //    Serial.print("AND: ");
//    //    Serial.println(digitalRead(motion_stop_exp_win_interrupt20));
//    //    Serial.print("X-Ray Exposed: ");
//    //    Serial.println(digitalRead(xray_exposed_interrupt19));
    lcd.setCursor(6, 0);
    lcd.print(digitalRead(ready_interrupt21));
    delay(2000);
}

void combined_ready() {
  // debounce
  if ((millis() - lastDebounceTime) < debounceDelay) {
    return;
  }
  lastDebounceTime = millis();

  // read from inputs
  bool ready_status = digitalRead(ready_interrupt21);
  bool motion_ready_status = digitalRead(motion_ready);
  bool detector_ready_status = digitalRead(detector_ready);
  bool xray_ready_status = digitalRead(xray_ready);
  Serial.print("Ready status:");
  Serial.println(ready_status);

  // print to lcd
  lcd.setCursor(6, 0);
  lcd.print(digitalRead(ready_interrupt21));
  
  // System is ready
  if (ready_status) {
    Serial.println("Ready = TRUE");
    lcd.setCursor(8, 0);
    lcd.print(" ");
    lcd.setCursor(0, 1);
    lcd.print("                ");

    // initiate detector and xray on 2nd and further exposures if ready
    // and the switch is pressed down
    bool switch_prep_status = digitalRead(switch_prep_interrupt3);
    bool switch_expose_status = digitalRead(switch_expose_interrupt2);
    Serial.print("Counter:");
    Serial.println(counter);
    Serial.print("Inverted switch prep status:");
    Serial.println(!switch_prep_status);
    Serial.print("Switch expose");
    Serial.println(switch_expose_status);
    if (counter > 0 && !switch_prep_status) {
      Serial.println("writing to detector");
      lcd.setCursor(0, 1);
      lcd.print("DET PREPPING    ");
      digitalWrite(detector_prep, LOW);
      
      
      // check if expose is on, and if so, also write to xray prep
      if (switch_expose_status) {
        Serial.println("WRITING xray prep");
        digitalWrite(xray_prep, LOW);
        lcd.setCursor(0, 1);
        lcd.print("XRAY PREPPING   ");
      }
      
    }

  
  } else {
    // System is not ready
  }

}

void switch_prep() {
  // debounce
  if ((millis() - lastDebounceTime) < debounceDelay) {
    return;
  }
  lastDebounceTime = millis();

  
  Serial.print("Received SWITCH PREP Signal:");


  // TODO: account for lag in switch prep signal
  Serial.println("**************");
  for (int i = 0; i < 100; i++) {
    //Serial.println(digitalRead(switch_prep_interrupt3));
    digitalRead(switch_prep_interrupt3);
  }
  Serial.println("**************");

  // read from inputs
  bool switch_prep_status = digitalRead(switch_prep_interrupt3);
  bool ready_status = digitalRead(ready_interrupt21);
  Serial.print("Switch Prep status:");
  Serial.println(switch_prep_status);

  // check that it is ready
  if (ready_status & !switch_prep_status) {
    Serial.println("WRITING motion move");
    lcd.setCursor(0, 1);
    lcd.print("MOTION MOVING   ");
    digitalWrite(motion_move, LOW);

  // switch prep was released -> restart
  } else {
    restart();
  }
}

void motion_stop_action() {
  // debounce
  if ((millis() - lastDebounceTime) < debounceDelay) {
    return;
  }
  lastDebounceTime = millis();


  // read from inputs
  bool motion_stopped_status = digitalRead(motion_stop_interrupt19);
  bool switch_prep_status = digitalRead(switch_prep_interrupt3);
  bool ready_status = digitalRead(ready_interrupt21);
  bool switch_expose_status = digitalRead(switch_expose_interrupt2);

  // turn off motion move once stop signal received
  if (motion_stopped_status) {
    Serial.print("Received MOTION STOPPED Signal:");
    Serial.println(motion_stopped_status);
    // turn off motion move
    digitalWrite(motion_move, HIGH);
  }

  // check that motion is stopped and on first movement, initiate detector
  if (motion_stopped_status && !seen_motion_stopped) {
    
    seen_motion_stopped = true;

//    Serial.print(counter);
//    Serial.print(switch_prep_status);
//    Serial.println(ready_status);

    // on the first motion movement, write to detector
    if (counter == 0 && !switch_prep_status && ready_status) {
      Serial.println("writing to detector");
      lcd.setCursor(0, 1);
      lcd.print("DET PREPPING    ");
      digitalWrite(detector_prep, LOW);

      // check if expose is on, and if so, also write to xray prep
      Serial.print("Switch expose status:");
      Serial.println(switch_expose_status);
      if (switch_expose_status) {
        Serial.println("WRITING xray prep");
        digitalWrite(xray_prep, LOW);
        lcd.setCursor(0, 1);
        lcd.print("XRAY PREPPING   ");
      }
    }
  }


}

void switch_expose() {
  // debounce
  if ((millis() - lastDebounceTime) < debounceDelay) {
    return;
  }
  lastDebounceTime = millis();

  bool switch_expose_status = digitalRead(switch_expose_interrupt2);
  
  Serial.println("************************");
  Serial.print("Received SWITCH EXPOSE Signal:");
  Serial.println(switch_expose_status);
}

void exp_win_action() {
  // debounce
  if ((millis() - lastDebounceTime) < debounceDelay) {
    return;
  }
  lastDebounceTime = millis();
  
  Serial.print("Received DET EXP WIN Signal:");

  // read from inputs
  bool exp_win_status = digitalRead(exp_win_interrupt20);
  bool switch_prep_status = digitalRead(switch_prep_interrupt3);
  bool ready_status = digitalRead(ready_interrupt21);
  bool switch_expose_status = digitalRead(switch_expose_interrupt2);

  // detector exposure window is active and switch expose is pressed -> activate xray exposure
  Serial.print("Switch expose status:");
  Serial.println(switch_expose_status);
  if (exp_win_status && ready_status && !switch_prep_status && switch_expose_status) {
    Serial.println("Writing to xray expose");
    digitalWrite(xray_exp, LOW);
    lcd.setCursor(0, 1);
    lcd.print("DET EXPOSING     ");
  }

  // detector exposure window ends
  if (!exp_win_status && ready_status && !switch_prep_status) {

    // turn off xray exposure
    Serial.println("Turn off detector and xray");
    digitalWrite(xray_exp, HIGH);
    digitalWrite(xray_prep, HIGH);
    digitalWrite(detector_prep, HIGH);

    // increment counter
    counter = counter + 1;
    if (counter >= maxCount) {
      Serial.println("----------------FINISHED----------------");
      lcd.setCursor(0, 1);
      lcd.print("FINISHED        ");
      Serial.println("WRITING motion move"); // returns to home
      digitalWrite(motion_move, LOW);
      delayMicroseconds(1000);
      restart();
    } else {
      Serial.println("Beginning next sequence once detector is ready");
      Serial.println("writing motion move");
      digitalWrite(motion_move, LOW);
    }

  }

}

void restart() {
  // print status
  Serial.println("************************");
  Serial.println("RESETTING: Turn OFF All Switches");
  Serial.println("************************");
  Serial.print("Ready: ");
  Serial.println(digitalRead(ready_interrupt21));
  Serial.print("Switches: ");
  Serial.print(digitalRead(switch_prep_interrupt3));
  Serial.println(digitalRead(switch_expose_interrupt2));
  Serial.print("Motion Ready: ");
  Serial.println(digitalRead(motion_ready));
  Serial.print("Det Ready: ");
  Serial.println(digitalRead(detector_ready));
  Serial.print("X-Ray Ready: ");
  Serial.println(digitalRead(xray_ready));
  Serial.print("Motion Stopped: ");
  Serial.println(digitalRead(motion_stopped));
  Serial.print("Det Exp Win: ");
  Serial.println(digitalRead(det_exp_win));
  //  Serial.print("AND: ");
  //  Serial.println(digitalRead(motion_stop_exp_win_interrupt20));
  //  Serial.print("X-Ray Exposed: ");
  //  Serial.println(digitalRead(xray_exposed_interrupt19));
  
  lcd.setCursor(0, 1);
  lcd.print("RESETTING       ");

  // write to output pins
  digitalWrite(motion_move, HIGH);
  digitalWrite(detector_prep, HIGH);
  digitalWrite(xray_prep, HIGH);
  digitalWrite(xray_exp, HIGH);

  counter = 0;
  seen_motion_stopped = false;
}
