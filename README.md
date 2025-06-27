# Smart Light Project
By Anindit Dewan

Welcome to my project using the ESP-IDF (Espressif Systems IoT Development Framework). 

This project is a smart light automation tool that allows users to control lights (whether it be an LED, or DC light bulb) using a web interface. There are various characteristics that the user can control about the lights such as allowing flickering, a certain brightness percentage they would like, timing the light to be on for certain intervals, and even adjusting the light based on the temperature in the room. 

## Skills & Technologies Used
* PWM for controlling light brightness and flickering  
* Timer for controlling time light will stay on
* Wi-Fi for creation of web server 
* I2C for communication with temperature sensor (BME280) 
* SPIFFS for storing HTML & CSS pages  
* Git for version control
* Breadboarding & Wiring 

## Technical Diagram 

## Video Demonstration 

## User Installation 
1. Clone the github repository 
2. Ensure that ESP-IDF is installed on your computer, this can be done by by installing the extension on Visual Studio Code. 
3. Open ESP-IDF terminal and enter `idf.py menuconfig` to open ESP-IDF configuration menu 
4. Go to Partition Table ->  Custom partition CSV file -> write `partitions.csv`
5. Escape/exit ESP-IDF configuration menu 
6. Enter `idf.py -p COMx flash monitor` to build, flash and open device monitor on the USB port of your choice. 