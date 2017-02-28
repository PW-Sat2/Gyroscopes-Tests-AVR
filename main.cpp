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

#include <string.h>
#include <stdlib.h>
#include "libs/SD/ff.h"
#include "libs/SD/integer.h"


FATFS FatFs;    // FatFs work area 
FIL *fp;        // fpe object 

using PCF8563_t = hal::PCF8563<hal::TWI>;
constexpr PCF8563_t rtc;

using namespace hal;
using namespace bsp;
using hal::InternalADC;


DWORD get_fattime() {

    PCF8563_t::Date date;
    PCF8563_t::Time time;

    rtc.get_date_time(date, time);
    // Returns current time packed into a DWORD variable 
    return    ((DWORD)(date.year - 1980) << 25)  // Year 2013 
    | ((DWORD)date.month << 21)              // Month 7 
    | ((DWORD)date.day << 16)              // Mday 28 
    | ((DWORD)time.hours << 11)             // Hour 0..24
    | ((DWORD)time.minutes << 5)              // Min 0 
    | ((DWORD)time.seconds >> 1);              // Sec 0
}

float pcb_temperature(uint16_t lsb) {
    return (lsb/1024.0*3000-2633)/(-13.6);
}

int main() {
    hal::Serial0.init(19200, hal::STDIO::ENABLE);
    hal::TWI::init<100000>();
    hal::TWI::enable_internal_pullups();

    InternalADC::init(InternalADC::Prescaler::DIV_128,
                      InternalADC::Reference::AVcc);
    InternalADC::select_channel(InternalADC::Input::ADC4);

    lcl.init(DigitalIO::Mode::OUTPUT);
    lcl.reset();
    _delay_ms(1000);
    lcl.set();
    _delay_ms(1000);

    led1.init();
    led2.init();
    led1.on();
    led2.off();


    using SoftI2C_1 = hal::SoftI2C<13, 14>;
    SoftI2C_1::init();

    using A3G4250D = hal::A3G4250D<SoftI2C_1>;
    constexpr A3G4250D A3G4250D_0(0b1101000);
    A3G4250D_0.set_data_rate_bandwidth(A3G4250D::DataRateCutOff::DR_00_BW_00_100_Hz_CF_12_5);
    A3G4250D_0.set_power_mode(A3G4250D::PowerMode::ACTIVE, A3G4250D::AxisPowerMode::NORMAL, A3G4250D::AxisPowerMode::NORMAL, A3G4250D::AxisPowerMode::NORMAL);
    A3G4250D_0.data_output_path(A3G4250D::DataOutputPath::LP2_HP_FILTERED);
    A3G4250D_0.high_pass_filter(A3G4250D::HighPassFilterMode::NORMAL_MODE, A3G4250D::HighPassFilterCutOff::HP_001_002_005_01_HZ);

    constexpr A3G4250D A3G4250D_1(0b1101001);
    A3G4250D_1.set_data_rate_bandwidth(A3G4250D::DataRateCutOff::DR_00_BW_00_100_Hz_CF_12_5);
    A3G4250D_1.set_power_mode(A3G4250D::PowerMode::ACTIVE, A3G4250D::AxisPowerMode::NORMAL, A3G4250D::AxisPowerMode::NORMAL, A3G4250D::AxisPowerMode::NORMAL);
    A3G4250D_1.data_output_path(A3G4250D::DataOutputPath::LP2_HP_FILTERED);
    A3G4250D_1.high_pass_filter(A3G4250D::HighPassFilterMode::NORMAL_MODE, A3G4250D::HighPassFilterCutOff::HP_001_002_005_01_HZ);

    using SoftI2C_2 = hal::SoftI2C<15, 16>;
    SoftI2C_2::init();

    using ITG3200 = hal::ITG3200<SoftI2C_2>;

    constexpr ITG3200 ITG3200_0(0b1101000);
    ITG3200_0.set_interrupt_flags(ITG3200::LatchMode::LATCH, ITG3200::LatchClearMethod::STATUS_REG_READ, ITG3200::InterruptControl::INT_ENABLED, ITG3200::InterruptControl::INT_ENABLED);
    ITG3200_0.init();
    ITG3200_0.set_clock(ITG3200::ClockSource::PLL_X_GYRO_REF);
    ITG3200_0.set_filters(1, ITG3200::LowPassFilter::LPF_256_HZ);

    constexpr ITG3200 ITG3200_1(0b1101001);
    ITG3200_1.set_interrupt_flags(ITG3200::LatchMode::LATCH, ITG3200::LatchClearMethod::STATUS_REG_READ, ITG3200::InterruptControl::INT_ENABLED, ITG3200::InterruptControl::INT_ENABLED);
    ITG3200_1.init();
    ITG3200_1.set_clock(ITG3200::ClockSource::PLL_X_GYRO_REF);
    ITG3200_1.set_filters(1, ITG3200::LowPassFilter::LPF_256_HZ);


    if (PCF8563_t::ClockStatus::STOPPED == rtc.getClockStatus()) {
        printf("Clock is not working, setting time!\r\n");
        rtc.clear_status();
        rtc.set_date_time({24, 5, 2, 2017}, {01, 47, 20});
    } else {
        printf("RTC is working!\r\n");
    }

    rtc.set_square_output(PCF8563_t::SquareOutput::SQW_1HZ);
    PCF8563_t::Date date;
    PCF8563_t::Time time;



    // init sdcard
    UINT bw;
    f_mount(0, &FatFs);     // Give a work area to the FatFs module 

    // open file
    fp = (FIL *)malloc(sizeof (FIL));
    if (f_open(fp, "file.txt", FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) { // Create a file 
        char *text = "Start!\r\n";
        printf("Opened\r\n");
        f_write(fp, text, strlen(text), &bw);   // Write data to the file 
        f_close(fp);// Close the file 
        if (bw == strlen(text)) { //we wrote the entire string
            led1.on(); // Lights LED if data written well (D4 led on atmega128 board)
            printf("Wrote entire \r\n");
        }
    } else {
        printf("Couldn't open!\r\n");
    }

     char str[12];
    
    // get card volume
    char szCardLabel[12] = {0};
    DWORD sn = 0;
    if (f_getlabel("", szCardLabel, &sn) == FR_OK) {

    }           
    
    // read from file
    /*if (f_open(fp, "file.txt", FA_READ ) == FR_OK) {    // Create a file
        char text[255]; 
        UINT br;
        f_read(fp, text, 255, &br);
        Serial0.print_byte_array((const uint8_t*)text, 255);
        f_close(fp);// Close the file
        // cut text the easy way
        text[10] = 0;
    }*/  

    char buffer[255];

    while (true) {

    ITG3200::GyroData ITG3200_data_0 = ITG3200_0.get_raw_gyro();
    ITG3200::GyroData ITG3200_data_1 = ITG3200_1.get_raw_gyro();
    printf("T: %f\tX: %d\tY: %d\tZ: %d\r\n", ITG3200_0.get_temperature(), ITG3200_data_0.X_axis, ITG3200_data_0.Y_axis, ITG3200_data_0.Z_axis);
    printf("T: %f\tX: %d\tY: %d\tZ: %d\r\n", ITG3200_1.get_temperature(), ITG3200_data_1.X_axis, ITG3200_data_1.Y_axis, ITG3200_data_1.Z_axis);
    
    A3G4250D::GyroData A3G4250D_data_0 = A3G4250D_0.get_raw_gyro();
    A3G4250D::GyroData A3G4250D_data_1 = A3G4250D_1.get_raw_gyro();
    printf("T: %d\tX: %d\tY: %d\tZ: %d\r\n", A3G4250D_0.get_temperature_raw(), A3G4250D_data_0.X_axis, A3G4250D_data_0.Y_axis, A3G4250D_data_0.Z_axis);
    printf("T: %d\tX: %d\tY: %d\tZ: %d\r\n", A3G4250D_1.get_temperature_raw(), A3G4250D_data_1.X_axis, A3G4250D_data_1.Y_axis, A3G4250D_data_1.Z_axis);
    
    float struct_temp = pcb_temperature(InternalADC::read());
    printf("PCB T: %f\r\n", struct_temp);

    if (f_open(fp, "file.txt", FA_WRITE | FA_OPEN_ALWAYS) == FR_OK) {   // Open existing or create new file
        if (f_lseek(fp, f_size(fp)) == FR_OK) 
            {
            char *text2 = "This is a new line, appended to existing file!\r\n";
            printf("New line appended...\r\n");

            snprintf(buffer, 255, "%02u:%02u:%02u\t%02u-%02u-%4u weekday: %u\r\n", time.hours, time.minutes, time.seconds,
                date.day, date.month, date.year, date.weekday);
            f_write(fp, buffer, strlen(buffer), &bw); // Write data to the file
            if (bw == strlen(text2)) { //we wrote the entire string

            }
            
        }
        f_close(fp);// Close the file       
    } else {
        printf("Opened\r\n");
    }



        if (PCF8563_t::ClockStatus::RUNNING == rtc.get_date_time(date, time)) {
            printf("%02u:%02u:%02u\t%02u-%02u-%4u weekday: %u\r\n",
                time.hours, time.minutes, time.seconds,
                date.day, date.month, date.year, date.weekday);
        } else {
            printf("RTC clock is not working...\r\n");
        }

        led1.toggle();
        led2.toggle();

        _delay_ms(1000);
    }
}
