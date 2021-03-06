/*
 *  RDAS Nunchuck Control
 *  ---------------------
 *  Arduino UNO
 *  
 *  xbee rx to 3
 *  xbee tx to 2
 *  nunchuck purple (d) to a4
 *  nunchuck orange (c) to a5
 *  
 *  0 - gnd
 *  1 - 5v
 *  2 - A4
 *  3 - A5
 *  4 - pin 3
 *  5 - pin 2
 * 
 *  Erin RobotGrrl
 *  Jan. 25, 2016
 * 
 */

#include <Wire.h>
#include <ArduinoNunchuk.h>
#include <SoftwareSerial.h>
#include <Streaming.h>
#include "Promulgate.h"

boolean DEBUG = false;
boolean MYO_MODE = false;

SoftwareSerial mySerial(3, 2); // RX, TX
Promulgate promulgate = Promulgate(&mySerial, &mySerial);
Promulgate promulgate_hw = Promulgate(&Serial, &Serial);

int led = 13;
long current_time = 0;
long last_time = 0;
boolean meepmeep = true;

ArduinoNunchuk nunchuk = ArduinoNunchuk();

int max_x = 228;
int min_x = 34;
int max_y = 223;
int min_y = 34;
int home_x = 125;
int home_y = 135;

int tilt_mode = 0;
int max_tilt_modes = 5;
long last_c = 0;
long last_control = 0;
long claw_time = 0;

int read_speed = 0;
int read_dir = 0;

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  //Serial.println("RDAS Nunchuck Control");
  
  promulgate.LOG_LEVEL = Promulgate::ERROR_;
  promulgate.set_rx_callback(received_action);
  promulgate.set_tx_callback(transmit_complete);

  promulgate_hw.LOG_LEVEL = Promulgate::ERROR_;
  promulgate_hw.set_rx_callback(received_action_hw);
  promulgate_hw.set_tx_callback(transmit_complete_hw);
  
  pinMode(led, OUTPUT);
  nunchuk.init();
}

void loop() {

  current_time = millis();

  /*
  // somethin' buggy with this
  if(mySerial.available()) {
    char c = mySerial.read();
    Serial << c;
    //promulgate.organize_message(c);
    digitalWrite(led, !digitalRead(led));
  }
  */
 
  if(MYO_MODE) {

    if(Serial.available()) {
      read_speed = Serial.parseInt();
    }

    promulgate.transmit_action('@', 'L', 1, read_speed, '!');
    promulgate.transmit_action('@', 'R', 1, read_speed, '!');

  }


  /*
  // old way - of using the Myo key mapper
  if(Serial.available()) {
    char c = Serial.read();
  
    if(MYO_MODE) {
      if(c == 'B') { // wave left
        promulgate.transmit_action('#', 'L', 1, 255, '!');
        promulgate.transmit_action('#', 'R', 1, 255, '!');
      } else if(c == 'C') { // wave right
        promulgate.transmit_action('#', 'L', 0, 255, '!');
        promulgate.transmit_action('#', 'R', 0, 255, '!');
      } else if(c == 'A') { // fist
        promulgate.transmit_action('#', 'L', 1, 0, '!');
        promulgate.transmit_action('#', 'R', 1, 0, '!');
      }
    }

  }
  */

  /*  
  // nunchuck debug print stuff
  Serial.print(nunchuk.analogX, DEC);
  Serial.print(' ');
  Serial.print(nunchuk.analogY, DEC);
  Serial.print(' ');
  Serial.print(nunchuk.accelX, DEC);
  Serial.print(' ');
  Serial.print(nunchuk.accelY, DEC);
  Serial.print(' ');
  Serial.print(nunchuk.accelZ, DEC);
  Serial.print(' ');
  Serial.print(nunchuk.zButton, DEC);
  Serial.print(' ');
  Serial.println(nunchuk.cButton, DEC);

  delay(100);
  */

  if(!MYO_MODE) {

  nunchuk.update();
    
  //if(current_time-last_control >= 20) {

      if(nunchuk.zButton == 0 && nunchuk.cButton == 0) { // drive
    
        int motor_speed = 0;
        boolean motor_dir = true;
    
        if(nunchuk.analogY > max_y) nunchuk.analogY = max_y;
        if(nunchuk.analogY < min_y) nunchuk.analogY = min_y;
        if(nunchuk.analogX > max_x) nunchuk.analogX = max_x;
        if(nunchuk.analogX < min_x) nunchuk.analogX = min_x;
    
        boolean blorp = false;
    
    
        if(nunchuk.analogY >= (home_y-10) && nunchuk.analogY <= (home_y+10)
           && nunchuk.analogX >= (home_x-10) && nunchuk.analogX <= (home_x+10)) {
          
          // stand still
          promulgate.transmit_action('#', 'L', 1, 0, '!');
          promulgate.transmit_action('#', 'R', 1, 0, '!');
          
        } else if(nunchuk.analogY >= (home_y-10) && nunchuk.analogY <= (home_y+10)) { // turning
          
          if(nunchuk.analogX >= (min_x+10)) {
            promulgate.transmit_action('@', 'L', 0, 255, '!');
            promulgate.transmit_action('@', 'R', 1, 255, '!');
          }
          if(nunchuk.analogX <= (max_x-10)) {
            promulgate.transmit_action('@', 'L', 1, 255, '!');
            promulgate.transmit_action('@', 'R', 0, 255, '!');
          }
          
        } else if(nunchuk.analogY >= home_y) { // fwd
          
          motor_speed = map(nunchuk.analogY, home_y, max_y, 0, 255);
          motor_dir = true;
          blorp = true;
          
        } else { // bwd
          
          motor_speed = map(nunchuk.analogY, min_y, home_y, 255, 0);
          motor_dir = false;
          blorp = true;
          
        }
    
        if(blorp) { // calculate the motor speed for L and R
          
          float percent_r = (float)map(nunchuk.analogX, min_x, max_x, 0, 100);
          percent_r /= 100.0;
          float percent_l = 1.0-percent_r;
      
          int speed_r = (int)((float)motor_speed * percent_r);
          int speed_l = (int)((float)motor_speed * percent_l);
      
          if(DEBUG) Serial << "speed L: " << speed_l << " R: " << speed_r << endl;
          
          // sending the data
          if(motor_dir) {
            promulgate.transmit_action('#', 'L', 1, motor_speed, '!');
            promulgate.transmit_action('#', 'R', 1, motor_speed, '!');
          } else {
            promulgate.transmit_action('#', 'L', 0, motor_speed, '!');
            promulgate.transmit_action('#', 'R', 0, motor_speed, '!');
          }
    
        }
    
        if(DEBUG) {
          if(motor_dir) {
            Serial << "FWD- ";
          } else {
            Serial << "BWD- ";
          }
        
          Serial << "motor_speed: " << motor_speed << endl;
        }
        
      } else if(nunchuk.zButton == 1 && nunchuk.cButton == 0) { // arm

        if(current_time-claw_time < 500) return; // claw buttons released check
    
        digitalWrite(led, LOW);
    
        int servo_pos = map(nunchuk.analogY, min_y, max_y, 0, 45);
        
        promulgate.transmit_action('#', 'S', 0, servo_pos, '!');
    
        Serial << "arm pos: " << servo_pos << endl;
        
      } else if(nunchuk.zButton == 0 && nunchuk.cButton == 1) { // tilt

        if(current_time-claw_time < 500) return; // claw buttons released check

        /*
        // old tilt code
        int tilt_pos = map(nunchuk.analogY, min_y, max_y, 0, 45);

        if(nunchuk.analogY >= (home_y-10) && nunchuk.analogY <= (home_y+10)
           && nunchuk.analogX >= (home_x-10) && nunchuk.analogX <= (home_x+10)) {

            tilt_pos = 999; // placeholder value for home pos
            
           }

        promulgate.transmit_action('#', 'T', 0, tilt_pos, '!');

        Serial << "tilt pos: " << tilt_pos << endl;
        */
        
        /*
        // no longer doing it this way
        if(current_time-last_c >= 1000) { // cycle through tilt modes
          digitalWrite(led, HIGH);
          tilt_mode++;
          if(tilt_mode > max_tilt_modes) tilt_mode = 0;
          
          promulgate.transmit_action('#', 'T', 0, tilt_mode, '!');
    
          Serial << "tilt mode: " << tilt_mode << endl;
          last_c = current_time;
        }
        */
        
        
      } else if(nunchuk.zButton == 1 && nunchuk.cButton == 1) { // claw

        if(current_time-last_c >= 1000) {
          digitalWrite(led, HIGH);

          promulgate.transmit_action('#', 'C', 0, 0, '!');

          Serial << "claw triggered" << endl;
          
          last_c = current_time;
          digitalWrite(led, LOW);
        }

        claw_time = current_time;

      }

  //last_control = current_time;
  //}

  }

  delay(20);

}



void received_action(char action, char cmd, uint8_t key, uint16_t val, char delim) {
  
  if(DEBUG) {
    Serial << "---CALLBACK---" << endl;
    Serial << "action: " << action << endl;
    Serial << "command: " << cmd << endl;
    Serial << "key: " << key << endl;
    Serial << "val: " << val << endl;
    Serial << "delim: " << delim << endl;
  }

  if(cmd == 'G') {
    int soil_reading = val;
    //Serial << "Soil, " << soil_reading << endl;
  }
  
}

void transmit_complete() {
  if(DEBUG) Serial << "transmit complete!" << endl;
}




void received_action_hw(char action, char cmd, uint8_t key, uint16_t val, char delim) {
  
  if(DEBUG) {
    Serial << "---CALLBACK---" << endl;
    Serial << "action: " << action << endl;
    Serial << "command: " << cmd << endl;
    Serial << "key: " << key << endl;
    Serial << "val: " << val << endl;
    Serial << "delim: " << delim << endl;
  }

  // just forward this message, if we receive one (ie, via serial terminal)
  if(cmd == 'L' || cmd == 'R') {
    promulgate.transmit_action(action, 'L', key, val, delim);
    promulgate.transmit_action(action, 'R', key, val, delim);
    digitalWrite(led, !digitalRead(led));
  }
  
}

void transmit_complete_hw() {
  if(DEBUG) Serial << "transmit complete!" << endl;
}


