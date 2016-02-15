# ArduinoCNC
Arduino CNC

http://www.HomoFaciens.de/technics-machines-cnc-v2_en_navion.htm

Installation:
1. Copy the files to a directory of your choice.
2. Compile 'plotter_v2.ino' and load the resulting code to the Arduino Uno
3. Open a terminal (Ctrl + T in Ubuntu) and change to your installation directory:
   cd /YourDirectory
4. Compile the source with:
   gcc commands-CNC.c -lm -o commands-CNC
5. Start the program with:
   ./commands-CNC
   

How to cut a file with the CNC router:

1. Press 'f' to select a file 
2. Press 's' to scale that file if needed (e.g. set scale to 0.5 to get just half the original file)
3. Press 'a' to set the speed if the router is down. The variable sets the time in milliseconds between two steps, thus the lower the value, the faster the router. Set the value to 1ms if you are using the maschine as plotter (with a pen un the Z carriage).
4. Press 'z' to set zHub
Drive the router to the deepest point with the page down key, so that it (nearly) touches the surface. You can toggle the number of steps the router moves by pressing 'm' from 1 to 10, 100 or 1000 steps. If the drill touches the surface, press 'Enter'.
Now drive the router to the highest point with the page up key (at least 2mm above the surface) and press 'Enter'.
5. Press '-' to reduce the lowest point of the router (about 0.5mm with each key press).
6. Drive the router to the starting point with the cursor keys.
7. Start the CNC machine by pressing 'p'.
If the material is to be cut in steps:
8. Reduce the lowest point of the router by pressing '-' and repeat the cutting process by pressing 'p'.

Video:
https://www.youtube.com/watch?v=mJ-TZvFpY58

