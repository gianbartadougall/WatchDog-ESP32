/**
 * @file ds18b20.c
 * @author Gian Barta-Dougall
 * @brief Driver for the DS18B20 temperature sensor
 *
 * This driver supports
 * 		- Sending any command to ds18b20
 * 		- Reading temperature data from ds18b20
 * 		- printing temperature data in readable format
 * 		- Reading the ROM from one ds18b20 sensor on the line
 * 		- Multiple DS18B20 sensors on the line provided their ROM codes have
 * 		been predefined in the ds18b20_init() function
 *
 * This driver does not support
 * 		- ROM searching
 * 		- Alarms
 * 		- Editing the settings of the temperature sensor
 * 		- Parasitic mode
 * 		- CRC checking
 *
 * @version 0.1
 * @date 2023-01-07
 *
 * @copyright Copyright (c) 2023
 *
 */
/* Private Includes */
#include "ds18b20.h"
#include "log.h"
#include "hardware_config.h"

/* Private Macros */
#define SET_PIN_LOW(port, pin) (port->BSRR |= (0x01 << (pin + 16)))

/* Private Variable Declarations */
ds18b20_t sensors[NUM_SENSORS];

/* Private Function Declarations */
void ds18b20_write_bit(uint8_t bit);
void ds18b20_delay_us(uint16_t delay);
void ds18b20_match_rom(ds18b20_t* ds18b20);
void ds18b20_print_temperature(uint8_t id);
void ds18b20_send_command(uint8_t command);
void print_64_bit(uint64_t number);

uint8_t ds18b20_convert_temperature(void);
uint8_t ds18b20_reset(void);
uint8_t ds18b20_read_bit(void);
uint8_t ds18b20_process_raw_temp_data(ds18b20_t* ds18b20, uint16_t rawTempData);
uint32_t ds18b20_pow(uint8_t base, uint8_t exponent);
uint8_t ds18b20_read_scratch_pad(ds18b20_t* ds18b20);
uint64_t ds18b20_read_rom(void);

/* ****************************** PUBLIC FUNCTIONS ****************************** */
/* ****************************************************************************** */
/* ****************************************************************************** */

void ds18b20_init() {

    // Start the microsecond clock. The paramters for this clock are defined in the
    // hardware configuration file
    DS18B20_TIMER->CR1 |= TIM_CR1_CEN;

    // Unique rom code for each temperature sensor
    sensors[DS18B20_SENSOR_ID_1].rom = 0x3f3c01d607587728;
    sensors[DS18B20_SENSOR_ID_2].rom = 0xa73c01d607368428;
}

void ds18b20_deinit(void) {
    DS18B20_TIMER->CR1 &= ~(TIM_CR1_CEN);
}

uint8_t ds18b20_read_temperature(uint8_t id) {

    if (ds18b20_convert_temperature() != DS18B20_SUCCESS) {
        debug_prints("Failed to convert temperature\r\n");
        return DS18B20_ERROR;
    }

    if (ds18b20_read_scratch_pad(&sensors[id]) != DS18B20_SUCCESS) {
        debug_prints("Failed to read scratch pad\r\n");
        return DS18B20_ERROR;
    }

    return DS18B20_SUCCESS;
}

void ds18b20_print_temperatures(void) {

    for (uint8_t i = 0; i < NUM_SENSORS; i++) {
        char msg[100];
        sprintf(msg, "%i: %s%i.%s%i\t", i + 1, sensors[i].sign == 1 ? "-" : "", sensors[i].decimal,
                sensors[i].fraction < 1000 ? "0" : "", sensors[i].fraction);
        debug_prints(msg);
    }

    debug_prints("\r\n");
}

void ds18b20_print_temperature(uint8_t id) {

    char msg[100];
    sprintf(msg, "Temp: %s%i.%s%i\r\n", sensors[id].sign == 1 ? "-" : "", sensors[id].decimal,
            sensors[id].fraction < 1000 ? "0" : "", sensors[id].fraction);
    debug_prints(msg);
}

void ds18b20_print_rom(uint8_t id) {
    print_64_bit(sensors[id].rom);
}

/**
 * @brief This function is a test function you can run to ensure hardware
 * is working correctly. It also provides an example of how to use this driver
 * to get the temperaure from connected sensors
 */
void ds18b20_test(void) {

    // Intialise the driver
    ds18b20_init();

    while (1) {

        // Read the temperature of sensor 1 connected to the line
        if (ds18b20_read_temperature(DS18B20_SENSOR_ID_1) != DS18B20_SUCCESS) {
            debug_prints("Error reading temperature from sensor 1\r\n");
        }

        // Read the temperature of sensor 2 connected to the line
        if (ds18b20_read_temperature(DS18B20_SENSOR_ID_2) != DS18B20_SUCCESS) {
            debug_prints("Error reading temperature from sensor 2\r\n");
        }

        // Print the temperatures of all the sensors to the console
        ds18b20_print_temperatures();

        HAL_Delay(1000);
    }
}

/* ****************************** PRIVATE FUNCTIONS ****************************** */
/* ******************************************************************************* */
/* ******************************************************************************* */

/**
 * @brief Sends the match rom command to all DS18B20 sensors connected then sends the
 * unique ROM code of the specified DS18B20 sensor that should be selected
 *
 * @param ds18b20 A pointer to the struct of the given DS18B20 sensor to be selected
 */
void ds18b20_match_rom(ds18b20_t* ds18b20) {

    ds18b20_send_command(DS18B20_COMMAND_MATCH_ROM);

    // Send ROM of the temperature sensor you want to speak to
    for (uint8_t i = 0; i < 64; i++) {
        ds18b20_write_bit(((ds18b20->rom & ((uint64_t)0x01 << i)) != 0) ? 1 : 0);
    }
}

/**
 * @brief Prints an unsigned 64 bit number to the console in hex. The 64-bit number
 * is seperated by spaces every 2 bytes. This function is was used for debugging
 * when printing the ROM code/datareceived from the DS18B20 sensor. This function
 * has been kept incase further development on this peripheral drive is conducted
 * and debugging is required.
 *
 * @param number The 64 bit number to be printed to the console.
 */
void ds18b20_print_64_bit(uint64_t number) {

    uint16_t bytes12 = ((number >> 0) & 0xFFFF);
    uint16_t bytes34 = ((number >> 16) & 0xFFFF);
    uint16_t bytes56 = ((number >> 32) & 0xFFFF);
    uint16_t bytes78 = ((number >> 48) & 0xFFFF);

    char msg[100];
    sprintf(msg, "Num: %x %x %x %x\r\n", bytes78, bytes56, bytes34, bytes12);
    debug_prints(msg);
}

/**
 * @brief This function is used to determine the ROM code of a single DS18B20
 * sensor connected. To connect multiple sensors you need to know the ROM codes
 * of each sensor before they are all connected. Connecting a single sensor then
 * calling this function will save the ROM code of the sensor into the sensors
 * list which you can then print out to find out what the ROM code of that sensor
 * is. This function has no other use then to find the ROM codes of new sensors
 * to be added.
 *
 * This function is only needed because the search ROM functionality has not
 * been implemented yet. To find out more about how it works, refer to the
 * datasheet for the DS18B20 sensor.
 *
 * @param id The ID of the sensor to get the ROM code of. This will typically
 * be 0 as you should only have one sensor connected when using this function
 * so the ROM code that is read will be stored in the 0th index of the sensor
 * list
 * @return uint8_t DS18B20_SUCCESS if no errors occured else DS18B20_ERROR
 */
uint8_t ds18b20_update_rom(uint8_t id) {

    if (ds18b20_reset() != DS18B20_SUCCESS) {
        return DS18B20_ERROR;
    }

    sensors[id].rom = ds18b20_read_rom();

    return DS18B20_SUCCESS;
}

/**
 * @brief Changes the mode of the pin on the mcu that connects to the DS18B20
 * data line to output mode
 */
void ds18b20_set_pin_output(void) {
    DS18B20_PORT->MODER &= ~(0x3 << (DS18B20_PIN * 2));
    DS18B20_PORT->MODER |= (0x01 << (DS18B20_PIN * 2));
}

/**
 * @brief Changes the mode of the pin on the mcu that connects to the DS18B20
 * data line to output input
 */
void ds18b20_set_pin_input(void) {
    DS18B20_PORT->MODER &= ~(0x11 << (DS18B20_PIN * 2));
}

/**
 * @brief This function will delay for the specified number of microseconds
 *
 * @param delay The number of micrseconds to delay for
 */
void ds18b20_delay_us(uint16_t delay) {
    DS18B20_TIMER->CNT = 0;
    while (DS18B20_TIMER->CNT < delay) {}
}

/**
 * @brief Reads the unique 64-bit serial number from the a DS18B20 sensor on the
 * bus
 *
 * @return uint64_t The unique serial number returned from the temperature sensor
 */
uint64_t ds18b20_read_rom(void) {
    ds18b20_send_command(DS18B20_COMMAND_READ_ROM);

    uint64_t romAddress = 0;

    for (uint8_t i = 0; i < 64; i++) {
        romAddress |= (((uint64_t)ds18b20_read_bit()) << i);
    }

    return romAddress;
}

/**
 * @brief Reads the scratch pad of the given sensor. The scratch pad contains the
 * latest temperature data the sensor has recorded among other things (refer to
 * ds18b20.h file for list of things the scratch pad contains)
 *
 * @param ds18b20 Pointer to the struct of the DS18B20 sensor to read the scratch
 * pad of
 * @return uint8_t DS18B20_SUCCESS if no errors occured else DS18B20_ERROR
 */
uint8_t ds18b20_read_scratch_pad(ds18b20_t* ds18b20) {

    if (ds18b20_reset() != DS18B20_SUCCESS) {
        return DS18B20_ERROR;
    }

    // Select the ds18b20 sensor to read the scratch pad of
    ds18b20_match_rom(ds18b20);

    // Tell sensor to send data when the next 64 read slots
    // are sent to it
    ds18b20_send_command(DS1B20_COMMAND_READ_SCRATCH_PAD);

    // Read data from sensor
    uint64_t data = 0;
    for (uint8_t i = 0; i < 64; i++) {
        data |= (((uint64_t)ds18b20_read_bit()) << i);
    }

    // Extract the raw temperature data from the ds18b20 and process it. The temperature
    // data is in bytes 1 and 2 of the 8 bytes sent from the sensor
    uint16_t rawTemperatureData = data & 0xFFFF;
    if (ds18b20_process_raw_temp_data(ds18b20, rawTemperatureData) != DS18B20_SUCCESS) {
        return DS18B20_ERROR;
    }

    return DS18B20_SUCCESS;
}

/**
 * @brief Converts the raw temperature data sent from the DS18B20 sensor to
 * a readable form which this function stores in the given struct of the
 * ds18b20 sensor that the data came from
 *
 * This function also does a check on theon the raw temperature data to
 * ensure the data is valid
 *
 * @param ds18b20 Pointer to the struct the processed temperature data will
 * be stored in
 * @param rawTempData The raw tempreature data sent from the DS18B20 sensor
 * @return uint8_t DS18B20_SUCCESS if no errors occured else DS18B20_ERROR
 */
uint8_t ds18b20_process_raw_temp_data(ds18b20_t* ds18b20, uint16_t rawTempData) {

    // Reset temperature value
    ds18b20->decimal  = 0;
    ds18b20->fraction = 0;
    ds18b20->sign     = 0;

    // Temperature data is stored in 2's complement. Convert if temperature is negative
    if ((rawTempData & (0x01 << 11)) != 0) {
        ds18b20->sign = 1;

        // Convert negative data to positive data
        rawTempData = (~rawTempData) + 1;
    }

    // Extract decimal component
    for (uint8_t i = 4; i < 11; i++) {
        if ((rawTempData & (0x01 << i)) != 0) {
            ds18b20->decimal += ds18b20_pow(2, i - 4);
        }
    }

    // Extract Fractional component
    for (uint8_t i = 0; i < 4; i++) {
        if ((rawTempData & (0x01 << i)) != 0) {
            ds18b20->fraction += 625 * ds18b20_pow(2, i);
        }
    }

    // Bits 11 - 15 should all be the same as they are sign bits
    if (((uint8_t)(rawTempData >> 11) != 0x00) && ((uint8_t)(rawTempData >> 11) != 0x1F)) {
        return DS18B20_ERROR;
    }

    // Extract sign bit
    ds18b20->sign = ((rawTempData & (0x01 << 12)) != 0) ? 1 : 0;
    return DS18B20_SUCCESS;
}

/**
 * @brief Tells every DS18B20 sensor connected to record the current temperature
 *
 * @return uint8_t DS18B20_SUCCESS if no errors occured else DS18B20_ERROR
 */
uint8_t ds18b20_convert_temperature(void) {

    // Reset the DS1820 temperature sensor
    if (ds18b20_reset() != DS18B20_SUCCESS) {
        return DS18B20_ERROR;
    }

    // We want all the sensors connected to record the temperature
    // so skipping the ROM lets us talk to all the sensors instead
    // of just one of the sensors
    ds18b20_send_command(DS18B20_COMMAND_SKIP_ROM);

    // Send command to convert temperature
    ds18b20_send_command(DS18B20_COMMAND_CONVERT_TEMP);

    // Wait until the temperature conversion has finished or timeout
    // has occured
    ds18b20_set_pin_input();

    // Wait for DS18B20 to finish temperature conversion
    while (ds18b20_read_bit() == 0) {
        HAL_Delay(10);
    }

    return DS18B20_SUCCESS;
}

/**
 * @brief Write an 8 bit command to the DS18B20 temperature sensor. All the
 * valid commands are listed as macros with descriptions of what they do in
 * the ds18b20.h file
 *
 * @param command The 8 bit command to send
 */
void ds18b20_send_command(uint8_t command) {

    // Loop through the 8 bit command and write each bit to the line
    // one at a time starting with the LSB
    for (uint8_t i = 0; i < 8; i++) {
        ds18b20_write_bit((command & (0x01 << i)) != 0);
    }
}

/**
 * @brief Calcualtes the power of a number to a given exponent.
 *
 * Example: ds18b20_pow(2, 5) = 2^5
 *
 * @param base The base to be raised to a power
 * @param exponent The exponent
 * @return uint32_t The caluculated value
 */
uint32_t ds18b20_pow(uint8_t base, uint8_t exponent) {

    if (exponent == 0) {
        return 1;
    }

    return base * ds18b20_pow(base, exponent - 1);
}

/**
 * @brief Reads a single bit from the DS18B20 temperature sensor
 *
 * To read from the ds18b20 temperature sensor, the master must send
 * a read slot. To do so:
 * 		1. Set the pin low for a minimum of 1us and then set the pint to input
 * 		2. Wait 15us
 * 		3. Record whether the pin is high or low
 * 		4. Ensure the function takes a minimum time of 61us to return
 *
 * @return uint8_t Value of the bit it read: either 0 or 1
 */
uint8_t ds18b20_read_bit(void) {

    // Initialise read by pulling data line low
    ds18b20_set_pin_output();
    SET_PIN_LOW(DS18B20_PORT, DS18B20_PIN);

    // Wait for minimum of 1us
    DS18B20_TIMER->CNT = 0;
    while (DS18B20_TIMER->CNT < 1) {}

    // Set the pin to an input
    ds18b20_set_pin_input();

    // Read the value of the line before 15us has occured since the
    // read was initialised
    while (DS18B20_TIMER->CNT < 10) {}
    uint8_t bit = ((DS18B20_PORT->IDR & (0x01 << DS18B20_PIN)) != 0) ? 1 : 0;

    // Wait minimum of 61us before exiting function. 65us gives small buffer
    // incase clk varies a bit
    while (DS18B20_TIMER->CNT < 65) {}

    // Return the value that was read from DS18B20 temperature sensor
    return bit;
}

/**
 * @brief Write a bit to the DS18B20 temperature sensor.
 *
 * To write a 1 to the DS18B20, you must:
 * 1. Pull the line low for 1us < t < 15us
 * 2. Set the line to input so the pull up resistor can pull the line high
 * 4. Wait at least 61us for the DS18B20 to read the value
 *
 * To write a 0 to the DS18B20 you must:
 * 1. Pull the line for a minimum of 60us
 * 2. Set the line to input so the pull up resistor can pull the line high
 *
 * @param bit The bit to write (0 or 1)
 */
void ds18b20_write_bit(uint8_t bit) {

    // Pull the bus low for 2us to ensure it goes low for at least a small period
    ds18b20_set_pin_output();
    SET_PIN_LOW(DS18B20_PORT, DS18B20_PIN);
    ds18b20_delay_us(2);

    if (bit == 1) {

        ds18b20_set_pin_input();
        ds18b20_delay_us(60); // Wait maximum of 60us for ds18b20 to read logical 1
        return;
    }

    ds18b20_delay_us(60); // Keep the line low for 60us
    ds18b20_set_pin_input();

    ds18b20_delay_us(5); // Wait additonal 5us for line to be pulled high by pull up resistor
}

/**
 * @brief This function needs to be called at the start of every command you want
 * to send to the DS18B20 temperature sensor. To reset the DS18B20 temperature
 * sensor:
 * 		1. Pull the data line low for a minimum of 480us
 * 		2. Release the data line and Wait 15us - 60us
 * 		3. Wait a maximum of 240us for the data line to be pulled low by ds18b20.
 * 		4. Wait at least 480us after releasing the data line before exiting the
 * 		   function
 *
 * @return uint8_t DS18B20_SUCCESS if the DS18B20 was reset else DS18B20_ERROR
 */
uint8_t ds18b20_reset(void) {

    // Initialise reset by pulling the line low for at least 480us
    ds18b20_set_pin_output();
    SET_PIN_LOW(DS18B20_PORT, DS18B20_PIN);
    ds18b20_delay_us(500);

    ds18b20_set_pin_input();

    // Wait for at least 15us and for the pull up resistor to pull the line high
    DS18B20_TIMER->CNT = 0;
    while (DS18B20_TIMER->CNT < 15 || (DS18B20_PORT->IDR & (0x01 << DS18B20_PIN)) == 0) {

        // Return error if timeout occurs
        if (DS18B20_TIMER->CNT > 60) {
            return DS18B20_ERROR;
        }
    }

    // If the DS18B20 resets correctly, it will pull the line low as an ackowledgmennt.
    // At least 480us must pass after the DS18B20 has ackowledged before you can communicate
    // with the sensor again
    uint8_t ds18b20Responded = DS18B20_ERROR;
    DS18B20_TIMER->CNT       = 0;
    while (DS18B20_TIMER->CNT < 480) {

        // Read the data line for response from the ds18b20 temperature sensor
        if ((DS18B20_PORT->IDR & (0x01 << DS18B20_PIN)) == 0) {
            ds18b20Responded = DS18B20_SUCCESS;
        }
    }

    if (ds18b20Responded == DS18B20_ERROR) {
        debug_prints("Failed to reset\r\n");
    }

    return ds18b20Responded;
}