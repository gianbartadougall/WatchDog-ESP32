# Watchdog ESP32 Version

# Application Operation
When uploaded to the ESP32 cam, this project will initialise the SD card and the camera and take a photo (jpeg) every 5 seconds and then save that photo to the SD card. Everything the system does is logged to the SD card so if something isn't working, unplug the system and look at the logs.txt file in the SD card which is under watchdog/logs in the SD card.

The red LED on the underside of the esp32 turns off when the ESP32 is taking a photo and saving the photo to the SD card (happen once every 5 seconds). Do not turn off the power to the ESP32 whilst this is happening as you will most likely damage the SD card. Wait for the LED to turn back on as this will give you 5 seconds to turn off the power.

If you see the red LED blinking once per second then an error has occured. You can unplug the ESP32 at any time and look at the logs in the SD card

# Hardware Features to be Implemented in Order
- Determine which pins can be used in conjunction with the camera and the SD card. Map out how these pins will connect to the required peripherals. These peripherals include:
   - Temperature Sensor
   - Real Time Clock
   - User Interface to show any errors that have occured and potnetially when the ESP32 is in the process of taking a photo or writing to the SD card
   - Button to make the system take a photo
   - Button to turn the system on and off
   - Hardware to protect the camera/ensure the camera has a clear view of the item it needs to take a photo of. Historically (using Raspberry Pi) this has been done with a servo
   - Plan what hardware may be required to setup something like LoRa in the future. Plan what pins may be useable for any additonal sensors that may be required in the future
- Determine how the ESP32 will be powered
- Determine the structure of the casing and how the casing will keep all the electronics water proof

# Software Features to be Implemented in Order
- Code to create a file structure on the SD card to save data and logs (DONE)
- Code to log operations and errors to the SD card (DONE)
- Code to get the user interface working i.e showing errors/modes of operation 
- Code to get the on/off power button working
- Code to get the real time clock working
- Code to get the temperature sensor working
- Code to get the hardware to protect the camera working
- Code to get the ESP32 to take a photo and record data manually
