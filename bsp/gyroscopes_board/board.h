#ifndef GYROSCOPES_TESTS_AVR_BSP_GYROSCOPES_BOARD_H_
#define GYROSCOPES_TESTS_AVR_BSP_GYROSCOPES_BOARD_H_

#include "periph/DigitalIO.h"
#include "devices/LED.h"

namespace hal {
namespace bsp {
namespace pins {
static constexpr DigitalIO::Pin LED_1 = 11;
static constexpr DigitalIO::Pin LED_2 = 12;
}  // namespace pins

constexpr static hal::LED led1(pins::LED_1);
constexpr static hal::LED led2(pins::LED_2);
}  // namespace bsp
}  // namespace hal

#endif  // GYROSCOPES_TESTS_AVR_BSP_GYROSCOPES_BOARD_H_
