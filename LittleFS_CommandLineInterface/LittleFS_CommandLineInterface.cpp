/*
  LittleFS_CommandLineInterface.h - Library for LittleFS command line interface.
  Created by George Modan, januar, 2023.
  Released into the public domain.
  Version 1.0.0
  
  Developed and used in the Arduino IDE. The interface its output write to serial port.
  Tested with ESP8266 in the Putty application, but can also be used in the Arduino IDE serial monitor.
  
  Command line interface is case-sensitive.
  If path not specified uses the work directory. Path separator is the slash character.
  It has command history with ten elements.
  
  More information can be obtained by help command.
  
  Készítette: Módán György, 2023 január
*/

#include "LittleFS_CommandLineInterface.h"
       
//-----------------------------------------------------
LittleFS_CommandLineInterface::LittleFS_CommandLineInterface(){
//-----------------------------------------------------
  LittleFS.begin();
  cmdHistIdx  = 0;
  pathPattern = "";
  setWorkDir("/");
  exit        = false;      
}
  
//-----------------------------------------------------
bool LittleFS_CommandLineInterface::readCommandLine() {                            
//-----------------------------------------------------
  String commandLine;
  char ch;
  char esc[10];

  if (exit){
      exit = false;
      return false;                
  }

  for (int i = 0 ; i < PARAM_COUNT; i++){ cmd[i] = ""; }

  Serial.print(prompt);
  do{           
    if(Serial.available()){
      ch = Serial.read();
      switch(ch){
        case 127:  if (commandLine.length() > 0) {                                                      // Back space
                      Serial.print(ch); 
                      commandLine = commandLine.substring(0,commandLine.length()-1);
                    }
                    break; 
        case  27:  Serial.readBytes(esc,2);                                                             // ESC key sequence read out
                    if (esc[1] < 65 || esc[1] > 66) {                                                   // VT sequences 
                      Serial.readStringUntil('~');
                    }else{                                                                              // xterm sequences (arrow keys)                           
                      commandLine = cmdHistoryControl(esc[1]);
                    } 
                    break;           
        case   4:  Serial.println(""); return false; break;                                             //CTRL+D
        default :  Serial.print(ch); commandLine += ch;
      }
    }
  }while(ch != '\r' && ch != '\n');
  
  if (ch == '\r') {       // last character in the command line
    Serial.read();        // reads out \n if exists
    Serial.println(""); 
  }

  cmdHistory(commandLine);
  splitLine(commandLine, 0);
  cmdInterpreter();
  
  return true;
}


//-----------------------------------------------------
void LittleFS_CommandLineInterface::setWorkDir(String path){                              
//-----------------------------------------------------
  workDir = path;
  prompt = "\r"+workDir+" >";
}

//-----------------------------------------------------
void LittleFS_CommandLineInterface::showSplitedCmd(){                              
//-----------------------------------------------------
  for (int i = 0 ; cmd[i].length() > 0; i++){
    Serial.println("--"+cmd[i]+"--");
  }
}

//-----------------------------------------------------
String LittleFS_CommandLineInterface::pad(String str, int length, char pad, char side){  
//-----------------------------------------------------
  String strPad;
  for (int i = str.length(); i < length; i++){
    strPad += pad;
  }
  if (side == 'L') return strPad+str;
  if (side == 'R') return str+strPad;
  return "";
}


// Becauseof LittleFs doing full path deletition
//-----------------------------------------------------
String LittleFS_CommandLineInterface::findWorkDir(String path){                    
//-----------------------------------------------------
  // If delete is on aother branch
  if (LittleFS.exists(workDir)){
    return workDir;
  }

  int lastSlash = path.lastIndexOf('/');
  while(!LittleFS.exists(path) && lastSlash != -1){
    path = path.substring(0,lastSlash);
    lastSlash = path.lastIndexOf('/');
  }
  return path.length() > 0 ? path : "/";
}

//-----------------------------------------------------        
bool LittleFS_CommandLineInterface::patternMatch(String path, String pattern){     
//-----------------------------------------------------          
  bool same      = true;
  bool starFlag  = false;
  int patternIdx = 0;
  int pathIdx    = 0;
  
  if (pattern.length() == 0){
    return true;
  }

  path += '\0';
  pattern += '\0';

  while(same && patternIdx < pattern.length() && pathIdx < path.length()){

    if (pattern.charAt(patternIdx) == '?'){  patternIdx++;    pathIdx++;    continue;  } 
    if (pattern.charAt(patternIdx) == '*'){  starFlag = true; patternIdx++; continue;  }  
    if (starFlag){  
      while (pathIdx < path.length() && pattern.charAt(patternIdx) != path.charAt(pathIdx)) pathIdx++;   
      starFlag = false;     continue;  
    }
    if (pattern.charAt(patternIdx) != path.charAt(pathIdx)) {  same = false;    continue;    }
        
    patternIdx++;
    pathIdx++;
  }
  
  if (pathIdx == path.length() && patternIdx == pattern.length())
    return true;
  else
    return false;
}

//-----------------------------------------------------        
String LittleFS_CommandLineInterface::pathValidate(String path, char type){        
//-----------------------------------------------------          
  int lastSlash = path.lastIndexOf('/');

  // Is there substite character, if yes, there is filename pattern
  if (path.indexOf('?') > lastSlash || path.indexOf('*') > lastSlash){
    pathPattern = path.substring(lastSlash + 1);
    path.replace(pathPattern,"");
  }else{
    pathPattern = "";          
  }

  // Cutting last slash, if exists
  if (path.length() > 0 && lastSlash == path.length()-1 && path != "/" && path != "./" && path != "../"){
    path = path.substring(0, lastSlash);
  }
  
  // Resolve ./ and ../ directory references.
  if (path.length() == 0){
    path = workDir;
  }else if (path.startsWith("./")){
    path != "./" ? path.replace(".",workDir) : path.replace(path,workDir);
  }else if (path.startsWith("../")){
    String parent =  workDir.substring(0,workDir.lastIndexOf('/'));
    path != "../" || parent.length() == 0 ? path.replace("..",parent) : path.replace("../",parent);
  }else if (!path.startsWith("/")){
    workDir == "/" ? path = workDir+path : path = workDir+'/'+path;
  }

  // Path length check. LittleFS limitation 32.
  if (path.length() > PATH_LENGTH){
    Serial.println(path+" to long! Max 32 character.");
    return "";
  }

  // Break. Exit without file check.
  if (type == 'B'){
    return path;
  }

  if (!LittleFS.exists(path)){
    Serial.println(path+" path not exists!");
    return "";            
  }

  File f = LittleFS.open(path,"r");
  if (type == 'D' && !f.isDirectory()) {
    Serial.println(path+" directory not exists!");
    return "";
  }
  if (type == 'F' && !f.isFile()) {
    Serial.println(path+" file not exists!");
    return "";
  }
  f.close();
  
  return path;
}

//-----------------------------------------------------        
void LittleFS_CommandLineInterface::tree(String path, int level){                  
//-----------------------------------------------------          
  long size;
  String branch = "|-- ";
  
  Dir dir = LittleFS.openDir(path);
  while (dir.next()) {            
    if (dir.isFile()){
      Serial.print(pad(branch,3+level*4,' ','L'));
      Serial.println(dir.fileName());
    }
  }
  dir = LittleFS.openDir(path);
  while (dir.next()) {            
    if (dir.isDirectory()){
      Serial.print(pad(branch,3+level*4,' ','L'));
      Serial.println(dir.fileName() + "  <dir>");
      tree(path+'/'+dir.fileName(), level + 1);
    }
  }

  return;
}

//-----------------------------------------------------        
void LittleFS_CommandLineInterface::typeHexa(String path){                         
//-----------------------------------------------------          
  int ch, c;
  long byteCounter = 0;
  long byteCountTitle = 0;
  char buffer[20];
  int idx = 0;

  File f = LittleFS.open(path, "r");
  if (!f) {
    Serial.println(cmd[1] + " file open failed!");
  } else {
    for (int i = 0; i < 20; i++){                  
          buffer[i] = 0;
    }

    while (f.available()) {
      ch = f.read();
      byteCounter++;
      buffer[idx++] = ch;
      if (!(byteCounter % 20) || !f.available()){
        Serial.print(pad((String)(byteCountTitle),6,' ','L') + " | "); 
        // write hexa
        for (int i = 0; i < 20; i++){
          if (buffer[i] < 16) Serial.print('0');
          Serial.print(buffer[i], HEX);
          Serial.print(' ');
          if (i == 9) Serial.print("| ");
        }
        // write characters
        Serial.print("| ");
        for (int i = 0; i < 20; i++){                  
          buffer[i] >= 32 ? Serial.print(buffer[i]) : Serial.print(' ');
          buffer[i] = 0;
        }
        Serial.println("");
        idx = 0;
        byteCountTitle += 20;
      }
    }
    f.close();
    Serial.println("");
  }        
}

//-----------------------------------------------------        
void LittleFS_CommandLineInterface::type(String path){                             
//-----------------------------------------------------          
  int ch;

  File f = LittleFS.open(path, "r");
  if (!f) {
    Serial.println(cmd[1] + " file open failed!");
  } else {
    while (f.available()) {
      ch = f.read();
      if (ch == '\r') Serial.write('\n');
      if (ch == '\n') continue;
      Serial.write(ch);
    }
    f.close();
    Serial.println("");
  }        
}

// Does not overwrite the file
//-----------------------------------------------------
void LittleFS_CommandLineInterface::copyOneFile(String inPath, String outPath){                             
//-----------------------------------------------------          
  File f_in, f_out;

  if (f_out = LittleFS.open(outPath, "r")){
    Serial.println(outPath+" file already exists!");
    f_out.close();
    return;
  }

  f_in = LittleFS.open(inPath, "r");
  if (!f_in) {
    Serial.println(inPath + " file read open failed!");
    return;
  }

  f_out = LittleFS.open(outPath, "w");
  if (!f_out) {
    Serial.println(outPath + " file write open failed!");
    f_in.close();
    return;
  }

  while (f_in.available()) {
    f_out.write(f_in.read());
  }
  f_out.flush();
  f_out.close();
  f_in.close();
}

//-----------------------------------------------------
void LittleFS_CommandLineInterface::cmdHistory(String cmd){                        
//-----------------------------------------------------
  cmd.remove(cmd.indexOf('\n'),1);
  cmd.remove(cmd.indexOf('\r'),1);
  
  if (cmd.length() > 0 && cmd != cmdHist[1]){
    for (int i = HISTORY_COUNT ; i > 1 ; i--){
      cmdHist[i] = cmdHist[i-1];
    }
    cmdHist[1] = cmd;
    cmdHistIdx = 0;  
  }
}

//-----------------------------------------------------
String LittleFS_CommandLineInterface::cmdHistoryControl(int arrowKey){            
//-----------------------------------------------------
  Serial.print(prompt);
  for(int i = 0; i < cmdHist[cmdHistIdx].length(); i++) {
    Serial.print(" ");
  }  
  Serial.print(prompt);

  if (arrowKey == 65){ // up arrow key
    if (cmdHist[cmdHistIdx + 1 ].length() > 0) {
      Serial.print(cmdHist[++cmdHistIdx]);      
    }else{
      Serial.print(cmdHist[cmdHistIdx]);      
    }        
  }
  if (arrowKey == 66 && cmdHistIdx > 0){ // down arrow key
    Serial.print(cmdHist[--cmdHistIdx]); 
  }
  
  return cmdHist[cmdHistIdx];
}

//-----------------------------------------------------
void LittleFS_CommandLineInterface::splitLine(String cmdSlice, int idx){           
//-----------------------------------------------------
    int beginIdx, endIdx, aposBeginIdx, aposEndIdx;
    
    if (idx == PARAM_COUNT) return;
    
    cmdSlice.trim();
    beginIdx = 0;
    endIdx = cmdSlice.indexOf(' ');
    aposBeginIdx = cmdSlice.indexOf('"');
    aposEndIdx = cmdSlice.indexOf('"',1);
    
    if (aposBeginIdx == 0 && aposEndIdx != -1){
        beginIdx = 1;
        endIdx = aposEndIdx;            
    }
    if (endIdx != -1) {
        cmd[idx] = cmdSlice.substring(beginIdx,endIdx);
        splitLine(cmdSlice.substring(endIdx+1), ++idx);
    }else{
        cmd[idx] = cmdSlice;
    }
}

//-----------------------------------------------------
void LittleFS_CommandLineInterface::cmdInterpreter(){                              
//-----------------------------------------------------
  FSInfo64 fs_info;

  //.........................................
  if (cmd[0] == "info"){
  //.........................................  
    LittleFS.info64(fs_info);
    Serial.println("-- Little File System Info --------------------------");
    Serial.print("   Total Bytes     : ");            Serial.println(fs_info.totalBytes);
    Serial.print("   Used Bytes      : ");            Serial.println(fs_info.usedBytes);
    Serial.print("   Block Size      : ");            Serial.println(fs_info.blockSize);
    Serial.print("   Page Size       : ");            Serial.println(fs_info.pageSize);
    Serial.print("   Max Open Files  : ");            Serial.println(fs_info.maxOpenFiles);
    Serial.print("   Max Path Length : ");            Serial.println(fs_info.maxPathLength);
    Serial.println("-----------------------------------------------------");
    Serial.print("   LittleFS command line interface version : ");   Serial.println(VERSION);
    return;            
  }

  //.........................................
  if (cmd[0] == "help"){
  //.........................................  
    Serial.println("\n Command line file manager based on the LittleFS file system.");
    Serial.println(" Developed and used in the Arduino IDE. The interface its output write to serial port.");
    Serial.println(" Tested with ESP8266 in the PuTTY application, but can also be used in the Arduino IDE serial monitor.");
    Serial.println(" Command line interface is case-sensitive.");
    Serial.println(" If path not specified uses the work directory. Path separator is the slash character."); 
    Serial.println(" It has command history with ten elements."); 
	Serial.println(" Able to load file content from clipboard.\n");
	
    Serial.println("\n-- Commands ---------------------------------------\n");
    Serial.println("  help");
    Serial.println("             Shows this screen.\n");
    Serial.println("  info");
    Serial.println("             Shows \"Little file system\" features.\n");
    Serial.println("  mkdir [path/]name");
    Serial.println("             Makes a directory with specific name. Path must be existed.\n");
    Serial.println("  rmdir [path/]name");
    Serial.println("             Removes directory. Directory must be under work directory.");
    Serial.println("             Only empty directory can be deleted. (Does full path delete, if parent directory is empty too)\n");
    Serial.println("  dir [path[/fileNamePattern]]");
    Serial.println("             Lists directory content. File name can be given by pattern too.");
    Serial.println("             (? = one character, * = more characters)\n" );
    Serial.println("  tree [path]");
    Serial.println("             Shows directory tree.\n");
    Serial.println("  cd [path]");
    Serial.println("             Changes work directory.\n");
    Serial.println("  load [path/]fileName");
    Serial.println("             Creates a file with specific name and loads content of clipboard into the file.");
    Serial.println("             Creates the path, if not exists yet.");
    Serial.println("             At the Arduino IDE, new line characters must be replaced with '^' character before load.");
    Serial.println("             At the PuTTY, clipboard content can be inserted with right mouse button click.\n");
    Serial.println("  del [path][/fileNamePattern]");
    Serial.println("             Deletes specific file or files. File name can be given by pattern too.");
    Serial.println("             (? = one character, * = more characters)\n");
    Serial.println("  ren [path/]fromFileName [path/]toFileName");
    Serial.println("             Renames or moves the file.\n");
    Serial.println("  copy [path][/fromFileNamePattern] [path][/toFileName]");
    Serial.println("             Copies file or files. \"From\" file name can be given by pattern too.");
    Serial.println("             (? = one character, * = more characters)" );
    Serial.println("             If fromFileName parameter specified by pattern, then toFileName parameter must be a directory.\n" );          
    Serial.println("  type [path/]fileName [hex]");
    Serial.println("             Writes out to screen the file content. If \"hex\" parameter is given too, then in hexadecimal format.\n");
    Serial.println("  exit");
    Serial.println("             Exits the interface program. Can do it with ctrl+D keystrokes too.\n");          
    Serial.println("  format");
    Serial.println("             Formats the file system. Deletes all content.\n");
    Serial.println("  begin");
    Serial.println("             Mounts the file system.\n");
    Serial.println("  end");
    Serial.println("             Unmounts the file system.\n");
    Serial.println("-----------------------------------------------------");
    return;          
  }
  
  //.........................................
  if (cmd[0] == "mkdir" && cmd[1].length() > 0){
  //.........................................  
    cmd[1] = pathValidate(cmd[1], 'B');
    if (cmd[1].length() == 0){    return;    }

    if (!LittleFS.mkdir(cmd[1])){
      Serial.println(cmd[1] + " directory create failed!");
    }
    return;          
  }

  //.........................................
  if (cmd[0] == "rmdir" && cmd[1].length() > 0){
  //.........................................  
    cmd[1] = pathValidate(cmd[1], 'D');
    if (cmd[1].length() == 0){    return;      }

    if (workDir.length() >= cmd[1].length() && workDir.startsWith(cmd[1])){
      Serial.println(cmd[1] + " want deleted directory must be under the work directory!");
      return;
    }

    if (!LittleFS.rmdir(cmd[1])){
      Serial.println(cmd[1] + " directory delete failed!");
      return;            
    }

    setWorkDir(findWorkDir(cmd[1]));
    return;          
  }

  //.........................................
  if (cmd[0] == "dir"){                   
  //.........................................  
    Dir dir;
    int fileCount = 0;
    int dirCount  = 0;
    long size     = 0;
    long sumSize  = 0;
    
    cmd[1] = pathValidate(cmd[1], 'B');
    if (cmd[1].length() == 0){    return;      }

    dir = LittleFS.openDir(cmd[1]);
    while (dir.next()) {
      if (!patternMatch(dir.fileName(),pathPattern)){
        continue;
      }
      Serial.print(pad(dir.fileName(),35,' ','R'));
      if (dir.isFile()){
        size = dir.fileSize();              
        Serial.println(pad(String(size),10,' ','L')+" bytes");
        fileCount++;
        sumSize += size;             
      }
      if (dir.isDirectory()){
        Serial.println("<dir>");
        dirCount++;
      }
    }
    Serial.println("\n"+pad(String(fileCount),25,' ','L') + (fileCount > 1 ? " files    "   : " file     ") + pad(String(sumSize),10,' ','L') + " bytes");
    Serial.println(     pad(String(dirCount),25,' ','L')  + (dirCount > 1  ? " directories" : " directory"));
    return;          
  }

  //.........................................
  if (cmd[0] == "tree"){                  
  //.........................................
    FSInfo64 fs_info;  
    
    cmd[1] = pathValidate(cmd[1], 'D');
    if (cmd[1].length() == 0){   return;     }
    
    cmd[1] == "/" ? Serial.println("  "+cmd[1]+" root") :Serial.println("  "+cmd[1]);
    tree(cmd[1], 1);

    if (cmd[1] == "/"){
      LittleFS.info64(fs_info);
      Serial.println("");
      Serial.print("   Total Bytes    : ");            Serial.println(fs_info.totalBytes);
      Serial.print("   Used Bytes     : ");            Serial.println(fs_info.usedBytes);
    }
    Serial.println("");
    return;          
  }
  //.........................................
  // Work directory managament
  if (cmd[0] == "cd" && cmd[1].length() > 0){   
  //.........................................  
    cmd[1] = pathValidate(cmd[1], 'D');
    if (cmd[1].length() == 0){    return;     }

    setWorkDir(cmd[1]);
    return;          
  }
  
  //.........................................
  if (cmd[0] == "type" && cmd[1].length() > 0){  
  //.........................................
    cmd[1] = pathValidate(cmd[1], 'F');
    if (cmd[1].length() == 0){   return;    }

    if (cmd[2] == "hex"){
      typeHexa(cmd[1]);
    } else {
      type(cmd[1]);
    }
    return;          
  }

  //.........................................        
  if (cmd[0] == "load" && cmd[1].length() > 0) {   
  //.........................................
    int ch;
    bool beginReadFlag = false;
    long pauseCounter  = 0;           
    
    cmd[1] = pathValidate(cmd[1], 'B');
    if (cmd[1].length() == 0){  return;   }
    
    File f = LittleFS.open(cmd[1], "w");
    if (!f) {
      Serial.println(cmd[1] + " file open failed!");
    }else{
      Serial.print("Insert from clipboard!");

      do{
          if (Serial.available()){
            beginReadFlag = true;
            pauseCounter = 0;                
            ch = Serial.read();
            if ( cmd[2] != "bin" && ch == NEW_LINE_CHAR ) { // new line character convert
              f.write('\n'); f.write('\r'); 
            }else{
              f.write(ch);
            }
          }
          if (beginReadFlag) pauseCounter++;
      }while(pauseCounter < 10000);

      f.flush();
      f.close();
      Serial.print("\r                                                 \r");
      Serial.println(cmd[1]+" file created\r\n");
    }
    return;          
  }

  //.........................................        
  if (cmd[0] == "del" && cmd[1].length() > 0) {
  //.........................................
    Dir dir;
    String fileName;

    cmd[1] = pathValidate(cmd[1], 'B');
    if (cmd[1].length() == 0){   return;   }

    if (pathPattern == ""){
      if (!LittleFS.remove(cmd[1])){
        Serial.println(cmd[1] + " file delete failed!");
        return;
      }
    }else{  
      dir = LittleFS.openDir(cmd[1]);
      while (dir.next()) {
        fileName = dir.fileName();
        if (!patternMatch(fileName,pathPattern) || dir.isDirectory()){
          continue;
        }
        if (!LittleFS.remove(cmd[1]+"/"+fileName)){
          Serial.println(fileName + " file delete failed!");
        }
      }
    }
    setWorkDir(findWorkDir(cmd[1]));
    return;          
  }

  //.........................................        
  if (cmd[0] == "ren" && cmd[1].length() > 0 && cmd[2].length() > 0) {
  //.........................................
    cmd[1] = pathValidate(cmd[1], 'B');
    if (cmd[1].length() == 0){   return;    }

    cmd[2] = pathValidate(cmd[2], 'B');
    if (cmd[2].length() == 0){    return;   }

    if (!LittleFS.rename(cmd[1], cmd[2])){
      Serial.println(cmd[1] + " file rename failed!");
    }
    return;
  }

  //.........................................        
  if (cmd[0] == "copy" && cmd[1].length() > 0 && cmd[2].length() > 0) {
  //.........................................
    Dir dir;
    String fileName, pattern;

    cmd[1] = pathValidate(cmd[1], 'B');

    if (pathPattern == "") {
      cmd[1] = pathValidate(cmd[1], 'F');          
      if (cmd[1].length() == 0){  return;  }

      cmd[2] = pathValidate(cmd[2], 'B');
      if (cmd[2].length() == 0){  return;  }

      copyOneFile(cmd[1],cmd[2]);
    }else{
      pattern = pathPattern;
      cmd[2] = pathValidate(cmd[2], 'D');
      if (cmd[2].length() == 0){  return;  }            

      dir = LittleFS.openDir(cmd[1]);
      while (dir.next()) {
        fileName = dir.fileName();
        if (!patternMatch(fileName, pattern) || dir.isDirectory()){
          continue;
        }
        copyOneFile(cmd[1]+"/"+fileName, cmd[2]+"/"+fileName);
      }
    }
    return;
  }

  //.........................................        
  if (cmd[0] == "exit") {                 
  //.........................................
    exit = true;  
    return;
  }

  //.........................................        
  if (cmd[0] == "format") {                 
  //.........................................
    char answer;

    Serial.print("Realy want to format the file system? Y/N  ");
    while(!Serial.available());
    answer = Serial.read();
    Serial.println(answer);
    if (answer == 'Y' || answer == 'y'){
      if (!LittleFS.format()){
        Serial.println("Format failed!");
      }else{
        Serial.println("Format done!");
      }
    }
    return;
  }

  //.........................................        
  if (cmd[0] == "begin") {                 
  //.........................................
    char answer;

    if (!LittleFS.begin()){
      Serial.println("Mount file system failed!");
    }else{
      Serial.println("Mount file system done!");
    }
    return;
  }

  //.........................................        
  if (cmd[0] == "end") {                 
  //.........................................
    LittleFS.end();
    Serial.println("Unmount done!");
    return;
  }

  if (cmd[0].length() > 0){
    Serial.println("Wrong command line instruction!");
  }

}
