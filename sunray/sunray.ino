// Ardumower Sunray 
// Copyright (c) 2013-2020 by Alexander Grau, Grau GmbH
// Licensed GPLv3 for open source use
// or Grau GmbH Commercial License for commercial use (http://grauonline.de/cms2/?page_id=153)

// Reference build target:
//   Arduino IDE 1.8.19
//   Adafruit Grand Central M4 / Adafruit SAMD Boards 1.7.5
// Newer Adafruit SAMD core versions may produce compilation errors.


//  ---------------------------------------------------------------------------------------------------------
//
//  NOTE: Before uploading the code, please: 
//  1. Rename file 'config_example.h' into 'config.h'
//  2. Open the file config.h and verify (configure) your hardware modules!
//  ---------------------------------------------------------------------------------------------------------

#include "config.h"  // see note above if you get an error here!
#include "robot.h"


void setup(){
  start();
} 

void loop(){  
  run();
}
