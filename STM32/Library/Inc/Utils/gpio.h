#ifndef GPIO_H
#define GPIO_H

/* Public Macros */
#define AF0  0x00
#define AF1  0x01
#define AF2  0x02
#define AF3  0x03
#define AF4  0x04
#define AF5  0x05
#define AF6  0x06
#define AF7  0x07
#define AF8  0x08
#define AF9  0x09
#define AF10 0x0A
#define AF11 0x0B
#define AF12 0x0C
#define AF13 0x0D
#define AF14 0x0E
#define AF15 0x0F

// Defines for the different configuration modes for the GPIO pins
#define GPIO_SET_MODE_INPUT(port, pin) (port->MODER &= ~(0x03 << (pin * 2)))
#define GPIO_SET_MODE_OUTPUT(port, pin)      \
    do {                                     \
        port->MODER &= ~(0x03 << (pin * 2)); \
        port->MODER |= (0x01 << (pin * 2));  \
    } while (0)
#define GPIO_SET_MODE_ALTERNATE_FUNCTION(port, pin, afNumber) \
    do {                                                      \
        port->MODER &= ~(0x03 << (pin * 2));                  \
        port->MODER |= (0x02 << (pin * 2));                   \
        port->AFR[pin / 8] &= ~(0x03 << ((pin % 8) * 4));     \
        port->AFR[pin / 8] |= ~(afNumber << ((pin % 8) * 4)); \
    } while (0)
#define GPIO_SET_MODE_ANALOGUE(port, pin) (port->MODER |= (0x03 << (pin * 2)))

#define GPIO_SET_TYPE_PUSH_PULL(port, pin)  (port->OTYPER &= ~(0x01 << pin))
#define GPIO_SET_TYPE_OPEN_DRAIN(port, pin) (port->OTYPER |= (0x01 << pin))

#define GPIO_SET_SPEED_LOW(port, pin) (port->OSPEEDR &= ~(0x03 << (pin * 2)))
#define GPIO_SET_SPEED_MEDIUM(port, pin)       \
    do {                                       \
        port->OSPEEDR &= ~(0x03 << (pin * 2)); \
        port->OSPEEDR |= (0x01 << (pin * 2));  \
    } while (0)
#define GPIO_SET_SPEED_HIGH(port, pin)         \
    do {                                       \
        port->OSPEEDR &= ~(0x03 << (pin * 2)); \
        port->OSPEEDR |= (0x02 << (pin * 2));  \
    } while (0)
#define GPIO_SET_SPEED_VERY_HIGH(port, pin) port->OSPEEDR |= (0x03 << (pin * 2))

#define GPIO_SET_PULL_AS_NONE(port, pin) (port->PUPDR &= ~(0x03 << (pin * 2)))
#define GPIO_SET_PULL_AS_PULL_UP(port, pin)  \
    do {                                     \
        port->PUPDR &= ~(0x03 << (pin * 2)); \
        port->PUPDR |= (0x01 << (pin * 2));  \
    } while (0)
#define GPIO_SET_PULL_AS_PULL_DOWN(port, pin) \
    do {                                      \
        port->PUPDR &= ~(0x03 << (pin * 2));  \
        port->PUPDR |= (0x02 << (pin * 2));   \
    } while (0)

#define GPIO_SET_HIGH(port, pin) (port->BSRR |= (0x01 << pin))
#define GPIO_SET_LOW(port, pin)  (port->BSRR |= (0x10000 << pin))
#define GPIO_TOGGLE(port, pin)                  \
    do {                                        \
        if ((port->IDR & (0x01 << pin)) == 0) { \
            GPIO_SET_HIGH(port, pin);           \
        } else {                                \
            GPIO_SET_LOW(port, pin);            \
        }                                       \
    } while (0)

#endif // GPIO_H