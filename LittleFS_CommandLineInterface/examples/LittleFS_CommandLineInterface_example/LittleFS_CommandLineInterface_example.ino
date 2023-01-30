/*
  Command line interface is case-sensitive.
  If path not specified uses the work directory. Path separator is the slash character.
  It has command history with ten elements.
  
  More information can be obtained by help command.
  
*/

#include "LittleFS_CommandLineInterface.h"

LittleFS_CommandLineInterface Cli;

void setup(){                                                
     Serial.begin(115200);  
}

void loop(){
  // Any keystroke begins interpreter                                                 
  if (Serial.available()){
    while(Cli.readCommandLine()); // Cycle untile user exit
  }
  delay(10);
}