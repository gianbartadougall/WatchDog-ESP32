#ifndef DS18B20_H
#define DS18B20_H

/* C Library Includes */
#include <stdint.h>

/* Personal Includes */
#include "custom_data_types.h"

/* Public Macros */

// Requests DS18B20 sensor to write it's 64-bit unique rom code. Note you must send 64
// read slots to the DS18B20 after sending this command so it can write it's rom code
#define DS18B20_COMMAND_READ_ROM 0x33

// Requests the DS18B20 sensor to read the next 64-bits written to the line as a ROM code
// This is used so you can select a particular ds18b20 sensor if multiple are on the same
// line
#define DS18B20_COMMAND_MATCH_ROM 0x55

// Requests the selected DS18B20 sensor to record the current temperature and save it
#define DS18B20_COMMAND_CONVERT_TEMP 0x44

/** Requests the selected DS18B20 to send 8 bytes of data to the master. These 8 bytes
 * consist of:
 * Byte 0: LSB of temperature data
 * Byte 1: MSB of temperature data
 * Byte 2: TH register
 * Byte 3: TL register
 * Byte 4: Configuration data
 * Byte 5, 6, 7: Reserved
 * byte 8: CRC code
 *
 * Note, you must send 64 read slots to the DS18B20 sensor after sending this command to
 * it for it to be able to write this data back
 */
#define DS1B20_COMMAND_READ_SCRATCH_PAD 0xBE

/** Allows you to send a single command to all the DS18B20 sensors on the line without
 * having to select a particular one i.e you can skip rom and then send a convert
 * temperature command which will mean all the DS18B20 sensors will convert their
 * temperature at once. You cannot skip the ROM if the DS18B20 will write back to the
 * master if their are multiple sensors on the line as there will be a data collision
 */
#define DS18B20_COMMAND_SKIP_ROM 0xCC

/** The number of sensors this driver can handle. The reason this is defined is that the
 * search rom command has not been implemented so all the sensors you will connect need
 * to be specified upfront as to communicate with them you need to know their ROM codes.
 * If the search functionality is implemented then writing the number of sensors being
 * used is not neccessary. This macro is used to define an array in ds1b20.c that holds
 * a list of ds18b20_t structs which contain the ROM codes of each sensor in them
 */
#define NUM_SENSORS 2

// IDs for each different sensor. These are used to access the list of ds18b20_t structs
// so you can pick a sensor that you want to commuincate with
#define DS18B20_ID_OFFSET   23
#define DS18B20_SENSOR_ID_1 (DS18B20_ID_OFFSET + 0)
#define DS18B20_SENSOR_ID_2 (DS18B20_ID_OFFSET + 1)

#define DS18B20_ASSERT_VALID_TEMPERATURE(temp)           \
    do {                                                 \
        if ((temp->sign == 0) && (temp->decimal > 60)) { \
            return FALSE;                                \
        }                                                \
                                                         \
        if ((temp->sign == 1) && (temp->decimal > 10)) { \
            return FALSE;                                \
        }                                                \
                                                         \
    } while (0)

/* Public Enumerations and Structures */

/**
 * @brief This function showcases an example use case of this driver to print the temperatures
 * of two DS18B20 sensors connected
 */
void ds18b20_test(void);

/**
 * @brief Starts the microsecond clock that this driver uses for timing. Note this function
 * does not setup the clock it just starts the clock. The initialisation of the clock
 * should occur in the hardware configuration files (usuall called hardware_config.c/.h) for
 * this project.
 *
 * The function also inialises the predefined ROM codes for each sensor that is to be connected
 * to the line. This function must be called before communicating with the DS18B20 sensor
 * otherwise the communication will fail
 */
void ds18b20_init(void);

/**
 * @brief Stops the microsecond timer. To be used when you no longer need to communicate with
 * any DS18B20 sensor for a while so stoppping the timer will save some power. To communicate
 * with the sensor again after calling this function, you must recall the ds18b20_init()
 * function
 */
void ds18b20_deinit(void);

/**
 * @brief Reads the temperature from the desired DS18B20 sensor. ds18b20_init() must be called
 * before calling this function.
 *
 * @param id The id specifies which DS18B20 sensor to read the temperature from
 * @return uint8_t DS18B20_SUCCESS if the temperature was updated correctly else DS18B20_ERROR
 */
uint8_t ds18b20_read_temperature(uint8_t id);

/**
 * @brief Prints the last temperature read from each sensor to the console.
 */
void ds18b20_print_temperatures(void);

/**
 * @brief Prints the temperature of the given DS18B20 sensor
 *
 * @param id The id of the DS18B20 sensor to print the temperature of
 */
void ds18b20_print_temperature(uint8_t id);

/**
 * @brief Stores the temperature of the given DS18B20 sensor into
 * the given struct
 *
 * @param id The id of the DS18B20 sensor to print the temperature of
 */
uint8_t ds18b20_copy_temperature(uint8_t id, cdt_dbl_16_t* temp);

/**
 * @brief Prints the ROM of a given sensor to the console
 *
 * @param id The ID of the sensor to be printed to the console
 */
void ds18b20_print_rom(uint8_t id);

void ds18b20_get_temperature(uint8_t id, char tempStr[30]);

#endif // DS18B20_H