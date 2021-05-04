# Robot Buddies 
A GUI simulator for simple robots that allows a total of 75 robots to connect and bounce around the screen

Program Author: Nuha Sheikh 

Libraries required:
- "-lm"
- "-lpthread"
- "1X11"

Launch: (Lanuch required in Linux environment) 
1. In command prompt, navigate to directory containing files 
2. In command prompt, type "make"
3. In command prompt, type "./environmentServer &" to launch GUI simulator 
4. In command prompt, type "./robotClient &" this can be typed repeatedly to add as many robots as you would like (max. 75) 
5. In command prompt, type "./stop" to quit GUI simulator 
6. In command prompt, type "make clean" to handle dynamic memory deallocation

Files:
- robotClient.c
- stop.c
- simulator.c
- environmentServer.c
- display.c (unoriginal code) 
- makefile

