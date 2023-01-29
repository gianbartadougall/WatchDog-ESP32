# Watchdog ESP32 Version

# Software
- Workable GUI
   - Set the layout for the normal mode (i.e buttons, labels and dropboxs)
   - Set the layout for the camera view (images and buttons)
   - Make drop boxes not editable vis typing
   - Setup the flags so information can be passed to Maple
      - Come up with protocol for sending flag and data information from GUI to Maple
      - Time and date settings for taking photos
      - Camera resolution settings
      - Change settings for RTC time and date
      - button for transfering data from ESP32 to computer
      - button for running test
      - button for going into camera live stream + button for going back to normal mode
   - Setup flags to recieve data from Maple
      - updating images for camera live stream
      - status information
         - Error code
         - number of iamges
         - current date and time

# Firmware Software
- Code to get the camera working (DONE)
- Code to get the SD card working (DONE)
- Code to create a file structure on the SD card to save data and logs (DONE)
- Code to log operations and errors to the SD card (DONE)
- Get the RTC working on the STM32
- Be able to recieve flags from GUI and send to ESP32
      - Time and date settings for taking photos
      - Camera resolution settings
      - Settings for RTC time and date
      - Transfering data from ESP32 to computer
      - Tunning a test
      - Streaming photos from ESP32 to GUI
 - ESP32 recieve all of the above and change those settings
      - Save all these settings on SD card on the ESP32
 - Get the STM32 working in deep sleep mode
 - Change the names of the images to a hex ID with a text file that relates ids to the date and time and temperature data
 - Write the main loop in the STM32 to commuicate to the ESP32 and get everything functioning

# Software Features to be Implemented in Order
- Code to get the camera working (DONE)
- Code to get the SD card working (DONE)
- Code to create a file structure on the SD card to save data and logs (DONE)
- Code to log operations and errors to the SD card (DONE)
- Code to get the user interface working i.e showing errors/modes of operation 
- Code to get the on/off power button working
- Code to get the real time clock working
- Code to get the temperature sensor working
- Code to get the hardware to protect the camera working
- Code to get the ESP32 to take a photo and record data manually

# Hardware
- PCB
   - On / off button
   - Area for battery to be connected
   - Buck converter
   - Button to make the system take a test photo?
   - Hardware to protect the camera/ensure the camera has a clear view of the item it needs to take a photo of. Historically (using Raspberry Pi) this has been done with a servo
- Casing
   - Determine the structure of the casing and how the casing will keep all the electronics water proof

# Other
- Write user manual for trouble shooting and how to use

# If we have time
- Button for viewing the data on the ESP32 from the GUI
- Checking data input in the GUI before sending it to the ESP32
