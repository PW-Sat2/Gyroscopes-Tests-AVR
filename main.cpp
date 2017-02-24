#include <util/delay.h>
#include "Serial.h"
#include "I2C.h"
#include "TWI.h"
#include "PCF8563.h"
#include "array.h"
#include "board.h"

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

int main() {
    hal::Serial0.init(4800, hal::STDIO::ENABLE);
    hal::TWI::init<100000>();
    hal::TWI::enable_internal_pullups();

    led1.init();
    led2.init();
    led1.on();
    led2.off();

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
        char *text = "Hello World! SDCard support up and running!\r\n";
        printf("Opened\r\n");
        f_write(fp, text, strlen(text), &bw);   // Write data to the file 
        f_close(fp);// Close the file 
        if (bw == strlen(text)) { //we wrote the entire string
            led1.on(); // Lights LED if data written well (D4 led on atmega128 board)
            printf("Wrote entire \r\n");
        }
        //else led2.Set(1);
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
