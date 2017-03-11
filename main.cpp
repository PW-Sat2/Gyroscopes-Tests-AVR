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

using namespace AppBackend;

ISR(TIMER1_OVF_vect) {
    AppBackend::flag_aquire = true;
}

int main() {
    uint8_t rstat = ResetFlag::read();

    Watchdog::enable(Watchdog::Period::p_8000ms);
    Watchdog::kick();
    
    Serial0.init(38400, STDIO::ENABLE);
    printf("DB:\tSystem after reboot\r\n");
    printf("DB:\tReboot source: %u\r\n", rstat);

    TWI::init<100000>();
    TWI::enable_internal_pullups();

    SoftI2C_1::init();
    SoftI2C_2::init();

    PCB_Temperature::init();

    Watchdog::kick();
    AppBackend::RTC::init({9, 4, 3, 2017}, {20, 43, 00});
    AppBackend::LEDs::init();
    
    Watchdog::kick();
    AppBackend::RTC::get_time();
    
    Watchdog::kick();
    AppBackend::SD::init();

    Timer1::init(Timer1::Prescaler::DIV_8, Timer1::Mode::Normal);
    Timer1::enable_overflow_interrupt();
    sei();

    while (true) {
        if (AppBackend::flag_aquire) {
            AppBackend::flag_aquire = false;
            Watchdog::kick();

            // avoid long files, set how many samples should be in one file at max.
            constexpr uint32_t max_data_len_in_file = 100000;
            if (0 == (AppBackend::Gyroscopes::count_data % max_data_len_in_file)) {
                AppBackend::SD::close();
                Watchdog::kick();
                AppBackend::SD::open();
                Watchdog::kick();
                AppBackend::Gyroscopes::configure_operational();
            }

            AppBackend::RTC::get_time();

            AppBackend::Gyroscopes::read();

            AppBackend::format_data();

            AppBackend::SD::save_on_card();
            
            Serial0.print_byte_array(reinterpret_cast<const uint8_t*>(SD::buffer), strlen(SD::buffer));

            led1.toggle();
        }
    }
}
