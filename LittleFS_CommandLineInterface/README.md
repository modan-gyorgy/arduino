# Preamble

 Command line file manager based on the LittleFS file system.
 Developed and used in the Arduino IDE. The interface its output write to serial port.
 Tested with ESP8266 in the PuTTY application, but can also be used in the Arduino IDE serial monitor.
 Command line interface is case-sensitive.
 If path not specified uses the work directory. Path separator is the slash character.
 It has command history with ten elements.
 Able to load file content from clipboard.

 # Installation
 
 Copy LittleFS Command Line Interface directory into libraries directory of your sketch directory.
 Start again the IDE and under sketch/include libraries menupoint you can see the LittleFS_CommandLineInterface library, and also see under file/examples the LittleFS_CommandLineInterface_example program.

 # Usable commands

  ### help
             Shows this screen.

  ### info
             Shows "Little file system" features.

  ### mkdir [path/]name
             Makes a directory with specific name. Path must be existed.

  ### rmdir [path/]name
             Removes directory. Directory must be under work directory.
             Only empty directory can be deleted. (Does full path delete, if parent directory is empty too.)

  ### dir [path[/fileNamePattern]]
             Lists directory content. File name can be given by pattern too.
             (? = one character, * = more characters)

  ### tree [path]
             Shows directory tree.

  ### cd [path]
             Changes work directory.

  ### load [path/]fileName
             Creates a file with specific name and loads content of clipboard into the file.
             Creates the path, if not exists yet.
             At the Arduino IDE, new line characters must be replaced with '^' character before load.
             At the PuTTY, clipboard content can be inserted with right mouse button click.

  ### del [path][/fileNamePattern]
             Deletes specific file or files. File name can be given by pattern too.
             (? = one character, * = more characters)

  ### ren [path/]fromFileName [path/]toFileName
             Renames or moves the file.

  ### copy [path][/fromFileNamePattern] [path][/toFileName]
             Copies file or files. "From" file name can be given by pattern too.
             (? = one character, * = more characters)
             If fromFileName parameter specified by pattern, then toFileName parameter must be a directory.

  ### type [path/]fileName [hex]
             Writes out to screen the file content. If "hex" parameter is given too, then in hexadecimal format.

  ### exit
             Exits the interface program. Can do it with ctrl+D keystrokes too.

  ### format
             Formats the file system. Deletes all content.

  ### begin
             Mounts the file system.

  ### end
             Unmounts the file system.

