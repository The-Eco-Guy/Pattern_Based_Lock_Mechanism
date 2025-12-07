#include <EEPROM.h>
#include <Servo.h>

const byte PASSWORD_LENGTH = 4;
const byte SERVO_OPEN_ANGLE = 180;
const byte SERVO_CLOSED_ANGLE = 0;
const unsigned long UNLOCK_DURATION = 5000;
const unsigned long LOCKOUT_DURATION = 50000;
const byte MAX_TIMEOUT = 3;

Servo myServo;
byte timeout = 0;

const int password_leds[] = {8, 9, 10, 11};
const int password_change_led = 12;
const int password_change_pb = 2;
const int pbs[] = {3, 4, 5, 6};

int password[PASSWORD_LENGTH] = {0};

const int address = 0;

enum State {password_change, password_check, password_correct, password_wrong};

volatile byte currentState;

void passwordChangeISR()
{
  currentState = password_change;
}

void setup()
{
  for (int i = 0; i < PASSWORD_LENGTH; i++) {
    pinMode(password_leds[i], OUTPUT);
    pinMode(pbs[i], INPUT_PULLUP);
  }
  pinMode(password_change_led, OUTPUT);
  pinMode(password_change_pb, INPUT_PULLUP);
  myServo.attach(13);
  
  attachInterrupt(digitalPinToInterrupt(password_change_pb), passwordChangeISR, FALLING);

  if (EEPROM.read(address) == 0)
    currentState = password_change;
  else {
    currentState = password_check;
    for (int i = 0; i < PASSWORD_LENGTH; i++) {
      password[i] = EEPROM.read(address + i);
    }
  }
}

void loop()
{
  updateStateMachine();
}

void updateStateMachine()
{
  switch (currentState)
  {
    case password_change:
    {
      digitalWrite(password_change_led, HIGH);
      int i = 0;
      while (i < PASSWORD_LENGTH)
      {
        for (int j = 0; j < PASSWORD_LENGTH; j++)
        {
          if (digitalRead(pbs[j]) == LOW)
          {
            delay(50);
            while (digitalRead(pbs[j]) == LOW);
            password[i] = j;
            EEPROM.update(address + i, j);
            i++;
            break;
          }
        }
      }
      digitalWrite(password_change_led, LOW);
      currentState = password_check;
      break;
    }

    case password_check:
    {
      int i = 0;
      while (i < PASSWORD_LENGTH)
      {
        if (currentState == password_change) break; 
        
        bool broken = false;
        for (int j = 0; j < PASSWORD_LENGTH; j++)
        {
          if (digitalRead(pbs[j]) == LOW)
          {
            delay(50);
            while (digitalRead(pbs[j]) == LOW);
            if (j == password[i])
            {
              digitalWrite(password_leds[i], HIGH);
              i++;
            }
            else
            {
              currentState = password_wrong;
              broken = true;
              break;
            }
          }
        }
        if (broken) break;
      }
      
      if (currentState != password_wrong && currentState != password_change) 
        currentState = password_correct;
        
      if (currentState != password_correct) {
        for (int k = 0; k < PASSWORD_LENGTH; k++) digitalWrite(password_leds[k], LOW);
      }
      break;
    }

    case password_correct:
    {
      myServo.write(SERVO_OPEN_ANGLE);
      delay(UNLOCK_DURATION);
      myServo.write(SERVO_CLOSED_ANGLE);
      for (int k = 0; k < PASSWORD_LENGTH; k++) digitalWrite(password_leds[k], LOW);
      currentState = password_check;
      break;
    }

    case password_wrong:
    {
      if (timeout < MAX_TIMEOUT)
      {
        timeout += 1;
        unsigned long start = millis();
        while (millis() - start < 1000)
        {
          for (int i = 0; i < PASSWORD_LENGTH; i++)
          {
            int pin = password_leds[i];
            digitalWrite(pin, digitalRead(pin) ^ 1);
          }
          delay(50);
        }
        currentState = password_check;
      }
      else
      {
        timeout = 0;
        delay(LOCKOUT_DURATION);
        currentState = password_check;
      }
      break;
    }
  }
}
