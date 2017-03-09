#include <util/delay.h>
#include "Serial.h"
#include "I2C.h"
#include "SoftI2C.h"
#include "TWI.h"
#include "PCF8563.h"
#include "array.h"
#include "board.h"
#include "ITG3200.h"
#include "A3G4250D.h"
#include "Analog.h"
#include "Watchdog.h"
#include "mcu.h"

#include <string.h>
#include <stdlib.h>
#include "libs/SD/ff.h"
#include "libs/SD/integer.h"

#include "app/gyro_backend.h"

using namespace GyroAppBackend;

ISR(TIMER1_OVF_vect) {
    GyroAppBackend::flag_aquire = true;
}

int main() {
    Reset::Status rstat = Reset::read_flag();

    Watchdog::enable(Watchdog::Period::p_8000ms);
    Watchdog::kick();
    
    Serial0.init(19200, STDIO::ENABLE);
    printf("System after reboot\r\n");
    printf("Reboot source: %u\r\n", rstat);

    TWI::init<100000>();
    TWI::enable_internal_pullups();

    SoftI2C_1::init();
    SoftI2C_2::init();

    Watchdog::kick();
    GyroAppBackend::RTC::init({9, 4, 3, 2017}, {20, 43, 00});
    GyroAppBackend::LEDs::init();
    
    Watchdog::kick();
    GyroAppBackend::RTC::get_time();
    
    Watchdog::kick();
    GyroAppBackend::SD::init();

    Watchdog::kick();
    GyroAppBackend::SD::open();

    Watchdog::kick();
    GyroAppBackend::Gyroscopes::configure();
    
    Timer1::init(Timer1::Prescaler::DIV_8, Timer1::Mode::Normal);
    Timer1::enable_overflow_interrupt();
    sei();

    while (true) {        
        if (GyroAppBackend::flag_aquire) {
            Watchdog::kick();

            GyroAppBackend::flag_aquire = false;

            GyroAppBackend::RTC::get_time();

            GyroAppBackend::Gyroscopes::read();

            GyroAppBackend::format_data();

            GyroAppBackend::SD::save_on_card();
            
            Serial0.print_byte_array(reinterpret_cast<const uint8_t*>(SD::buffer), strlen(SD::buffer));

            led1.toggle();
        }
    }
}
