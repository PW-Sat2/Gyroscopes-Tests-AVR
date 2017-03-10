#ifndef APP_GYRO_BACKEND_H_
#define APP_GYRO_BACKEND_H_

namespace GyroAppBackend {
volatile bool flag_aquire = false;

using namespace hal;
using namespace bsp;
using hal::InternalADC;

namespace RTC {
using PCF8563_t = hal::PCF8563<hal::TWI>;
constexpr PCF8563_t rtc;
PCF8563_t::Date date;
PCF8563_t::Time time;

void init(PCF8563_t::Date date, PCF8563_t::Time time) {
    if (PCF8563_t::ClockStatus::STOPPED == rtc.getClockStatus()) {
        printf("RTC is not working, setting time!\r\n");
        rtc.clear_status();
        rtc.set_date_time(date, time);
    } else {
        printf("RTC is working!\r\n");
    }
}

void get_time() {
    if (PCF8563_t::ClockStatus::RUNNING == rtc.get_date_time(date, time)) {
        //printf("%02u:%02u:%02u\t%02u-%02u-%4u weekday: %u\r\n",
        //    time.hours, time.minutes, time.seconds,
        //    date.day, date.month, date.year, date.weekday);
    } else {
        printf("RTC not working!\r\n");
        date = {99, 99, 99, 99};
        time = {99, 99, 99};
    }

    rtc.set_square_output(PCF8563_t::SquareOutput::SQW_1HZ);
}
}  // namespace RTC

namespace SD {
FATFS FatFs; 
FIL *fp;
UINT bw;
constexpr uint16_t buff_len = 512;
char buffer[buff_len];

uint16_t save_fails = 0;
uint16_t chunk_file_number = 0;

void init() {
    lcl.init(DigitalIO::Mode::OUTPUT);
    lcl.reset();
    _delay_ms(1000);
    lcl.set();
    _delay_ms(1000);

    f_mount(0, &FatFs);
    fp = (FIL *)malloc(sizeof(FIL));
}

void open() {
    RTC::PCF8563_t::Date date;
    RTC::PCF8563_t::Time time;
    char filename[50];

    chunk_file_number++;

    if (RTC::PCF8563_t::ClockStatus::RUNNING == RTC::rtc.get_date_time(date, time)) {
        snprintf(filename, 50, "gyro-%02u-%02u-%u-%02u-%02u-%02u-%u.txt", date.day, date.month, date.year, time.hours, time.minutes, time.seconds, chunk_file_number);
    } else {
        InternalADC::select_channel(InternalADC::Input::ADC5);
        srand(InternalADC::read());
        InternalADC::select_channel(InternalADC::Input::ADC4);
        snprintf(filename, 50, "gyro-rand-%u-%u.txt", rand(), chunk_file_number);
    }

    printf("Filename: %s\r\n", filename);

    printf("Check for card\r\n");
    if (f_open(fp, reinterpret_cast<const TCHAR*>(filename), FA_WRITE | FA_OPEN_ALWAYS) == FR_OK) {
        printf("Card OK!\r\n");
        led2.on();
        _delay_ms(100);
    } else {
        printf("Card NOK!\r\n");
    }
}

void close() {
    f_close(fp);
}

void save_on_card() {
    if (f_lseek(fp, f_size(fp)) == FR_OK) {
        f_write(fp, buffer, strlen(buffer), &bw);

        printf("Written ");

        if (bw == strlen(buffer)) {
            printf("entire");
            led2.on();
        } else {
            led2.off();
            save_fails++;
        }
        printf("\r\n");

        f_sync(fp); // sync the file
    } else {
        led2.off();
        save_fails++;
    }

    // if there are problems with SD card wait for 20 until WDT reset
    if (save_fails > 100) {
        save_fails = 0;
        printf("Problem with SD, wait for reset...\r\n");
        for (uint16_t i = 0; i < 20; i++) {
            _delay_ms(1000);
        }
    }
}
}  // namespace SD

namespace LEDs {
void init() {
    led1.init();
    led2.init();
    led1.on();
    led2.off();
}
}  // namespace LEDs


using SoftI2C_1 = hal::SoftI2C<13, 14>;
using SoftI2C_2 = hal::SoftI2C<15, 16>;

namespace Gyroscopes {
using A3G4250D_t = hal::A3G4250D<SoftI2C_1>;
using ITG3200_t = hal::ITG3200<SoftI2C_2>;

constexpr ITG3200_t ITG3200_0(0b1101000);
constexpr ITG3200_t ITG3200_1(0b1101001);
constexpr A3G4250D_t A3G4250D_0(0b1101000);
constexpr A3G4250D_t A3G4250D_1(0b1101001);
ITG3200_t::GyroData ITG3200_data_0;
ITG3200_t::GyroData ITG3200_data_1;
A3G4250D_t::GyroData A3G4250D_data_0;
A3G4250D_t::GyroData A3G4250D_data_1;
uint32_t count_data = 0;

void configure() {
    A3G4250D_0.set_data_rate_bandwidth(A3G4250D_t::DataRateCutOff::DR_00_BW_00_100_Hz_CF_12_5);
    A3G4250D_0.set_power_mode(A3G4250D_t::PowerMode::ACTIVE, A3G4250D_t::AxisPowerMode::NORMAL, A3G4250D_t::AxisPowerMode::NORMAL, A3G4250D_t::AxisPowerMode::NORMAL);
    A3G4250D_0.data_output_path(A3G4250D_t::DataOutputPath::LP2_FILTERED);

    A3G4250D_1.set_data_rate_bandwidth(A3G4250D_t::DataRateCutOff::DR_00_BW_00_100_Hz_CF_12_5);
    A3G4250D_1.set_power_mode(A3G4250D_t::PowerMode::ACTIVE, A3G4250D_t::AxisPowerMode::NORMAL, A3G4250D_t::AxisPowerMode::NORMAL, A3G4250D_t::AxisPowerMode::NORMAL);
    A3G4250D_1.data_output_path(A3G4250D_t::DataOutputPath::LP2_FILTERED);

    ITG3200_0.set_interrupt_flags(ITG3200_t::LatchMode::LATCH, ITG3200_t::LatchClearMethod::STATUS_REG_READ, ITG3200_t::InterruptControl::INT_ENABLED, ITG3200_t::InterruptControl::INT_ENABLED);
    ITG3200_0.init();
    ITG3200_0.set_clock(ITG3200_t::ClockSource::PLL_X_GYRO_REF);
    ITG3200_0.set_filters(1, ITG3200_t::LowPassFilter::LPF_20_HZ);

    ITG3200_1.set_interrupt_flags(ITG3200_t::LatchMode::LATCH, ITG3200_t::LatchClearMethod::STATUS_REG_READ, ITG3200_t::InterruptControl::INT_ENABLED, ITG3200_t::InterruptControl::INT_ENABLED);
    ITG3200_1.init();
    ITG3200_1.set_clock(ITG3200_t::ClockSource::PLL_X_GYRO_REF);
    ITG3200_1.set_filters(1, ITG3200_t::LowPassFilter::LPF_20_HZ);
}

void read() {
    ITG3200_data_0 = ITG3200_0.get_raw_gyro();
    ITG3200_data_1 = ITG3200_1.get_raw_gyro();

    A3G4250D_data_0 = A3G4250D_0.get_raw_gyro();
    A3G4250D_data_1 = A3G4250D_1.get_raw_gyro();
}

}  // namespace Gyroscopes

namespace PCB_Temperature {
constexpr uint16_t ReferenceVoltage_mV = 3000;

void init() {
    InternalADC::init(InternalADC::Prescaler::DIV_128,
        InternalADC::Reference::AVcc);
    InternalADC::select_channel(InternalADC::Input::ADC4);
}

float measure() {
    uint16_t lsb = InternalADC::read();
    return (lsb/1024.0*ReferenceVoltage_mV-2633)/(-13.6);
}
} //  namespace PCB_Temperature

void format_data() {
    snprintf(SD::buffer, SD::buff_len,
        "%02u;%02u;%02u;%02u;%02u;%02u;%lu;%.2f;%u,%.2f;%d;%d;%d;%u,%.2f;%d;%d;%d;%u,%d;%d;%d;%d;%u,%d;%d;%d;%d;\r\n",
        RTC::date.day, RTC::date.month, RTC::date.year,
        RTC::time.hours, RTC::time.minutes, RTC::time.seconds, // time
        Gyroscopes::count_data++,
        PCB_Temperature::measure(),   // struct temperature
        Gyroscopes::ITG3200_0.get_id(),
        Gyroscopes::ITG3200_0.get_temperature(),
        Gyroscopes::ITG3200_data_0.X_axis, Gyroscopes::ITG3200_data_0.Y_axis, Gyroscopes::ITG3200_data_0.Z_axis,
        Gyroscopes::ITG3200_1.get_id(),
        Gyroscopes::ITG3200_1.get_temperature(),
        Gyroscopes::ITG3200_data_1.X_axis, Gyroscopes::ITG3200_data_1.Y_axis, Gyroscopes::ITG3200_data_1.Z_axis,
        Gyroscopes::A3G4250D_0.get_id(),
        Gyroscopes::A3G4250D_0.get_temperature_raw(),
        Gyroscopes::A3G4250D_data_0.X_axis, Gyroscopes::A3G4250D_data_0.Y_axis, Gyroscopes::A3G4250D_data_0.Z_axis,
        Gyroscopes::A3G4250D_1.get_id(),
        Gyroscopes::A3G4250D_1.get_temperature_raw(),
        Gyroscopes::A3G4250D_data_1.X_axis, Gyroscopes::A3G4250D_data_1.Y_axis, Gyroscopes::A3G4250D_data_1.Z_axis
    );
}
} // namespace GyroAppBackend

DWORD get_fattime() {
    GyroAppBackend::RTC::PCF8563_t::Date date;
    GyroAppBackend::RTC::PCF8563_t::Time time;
    GyroAppBackend::RTC::rtc.get_date_time(date, time);

    return ((DWORD)(date.year - 1980) << 25)
    | ((DWORD)date.month << 21)
    | ((DWORD)date.day << 16)
    | ((DWORD)time.hours << 11)
    | ((DWORD)time.minutes << 5)
    | ((DWORD)time.seconds >> 1);
}

#endif  // APP_GYRO_BACKEND_H_
