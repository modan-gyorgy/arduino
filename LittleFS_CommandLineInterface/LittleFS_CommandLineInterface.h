/*
  LittleFS_CommandLineInterface.h - Library for LittleFS command line interface.
  Created by George Modan, januar, 2023.
  Released into the public domain.
  Version 1.0.0
  
  Készítette: Módán György, 2023 január
*/

#ifndef LittleFS_CommandLineInterface_h
#define LittleFS_CommandLineInterface_h

#include "LittleFS.h"
#include <String.h>

#define VERSION "1.0.0" 

/*------------------------------------------------------------*/
class LittleFS_CommandLineInterface{
/*------------------------------------------------------------*/

    const static int  PARAM_COUNT   = 10;
    const static int  PATH_LENGTH   = 32;
    const static int  HISTORY_COUNT = 10;
    const static char NEW_LINE_CHAR = '^';  // For loading from arduino IDE serial monitor

    String     cmd[PARAM_COUNT];
    String     cmdHist[HISTORY_COUNT + 2];  // cmdHist[0] and cmdHist[HISTORY_COUNT+1] are delimiters, always empty
    int        cmdHistIdx;
    String     workDir;
    String     pathPattern;
    String     prompt;
    bool       exit;

  public:
           LittleFS_CommandLineInterface();
    bool   readCommandLine();

  private:
    void   setWorkDir(String path);
    void   showSplitedCmd();
    String pad(String str, int length, char pad, char side);
    String findWorkDir(String path);
    bool   patternMatch(String path, String pattern);
    String pathValidate(String path, char type);
    void   tree(String path, int level);
    void   typeHexa(String path);
    void   type(String path);
    void   copyOneFile(String inPath, String outPath);
    void   cmdHistory(String cmd);
    String cmdHistoryControl(int arrowKey);
    void   splitLine(String cmdSlice, int idx);
    void   cmdInterpreter();
};


#endif
