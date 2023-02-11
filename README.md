# Watchdog ESP32 Version

# Software
- Workable GUI
   - Set the layout for the normal mode (i.e buttons, labels and dropboxs) (DONE)
   - Set the layout for the camera view (images and buttons) (DONE)
   - Make drop boxes not editable via typing (DONE)
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

# Firmware
- Code to get the camera working (DONE)
- Code to get the SD card working (DONE)
- Code to create a file structure on the SD card to save data and logs (DONE)
- Code to log operations and errors to the SD card (DONE)
- Get the RTC working on the STM32 (DONE)
- Be able to recieve flags from GUI and send to ESP32 via the STM32 (DONE)
   - Time and date settings for taking photos (DONE)
   - Camera resolution settings (DONE)
   - Settings for RTC time and date (DONE)
   - Transfering data from ESP32 to computer (DONE)
   - Running a test (DONE)
   - Streaming photos from ESP32 to GUI (DONE)
- ESP32 recieve all of the above and change those settings (DONE)
   - Save all these settings on SD card on the ESP32 (DONE)
- Get the STM32 working in deep sleep mode
   - Setup interrupt so the ESP32 will wake up at the set times to take a photo before going back to sleep
   - Setup UART interrupt so the STM32 will wake up if it receives something on the UART line. It will then
      go back to deep sleep mode once it has processed the task
- Get the SD card working with long file names (DONE)
- Write the main loop in the STM32 to commuicate to the ESP32 and get everything functioning
- Setup ADC to measure voltage of the battery on the ESP32 and blink an LED a certain number of times to indicate the
   estimated time left

# Hardware
- PCB
   - On / off button
   - Area for battery to be connected
   - Low quiescent current linear voltage regulator
   - Button to make the system take a test photo?
   - Hardware to protect the camera/ensure the camera has a clear view of the item it needs to take a photo of. Historically (using Raspberry Pi) this has been done with a servo
- Casing
   - Determine the structure of the casing and how the casing will keep all the electronics water proof

# Other
- Write user manual for trouble shooting and how to use

# If we have time
- Check battery level
      - Maybe the LED and volatge divider option so it can be checked without a computer.
- Create a better solution for inputing the correct time for calibrating the real time clock
- Button for viewing the data on the ESP32 from the GUI
- Checking data input in the GUI before sending it to the ESP32
- Read the screen resolution of the device it is on to pick the size of the window
