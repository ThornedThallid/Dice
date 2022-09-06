#include <Arduino.h>
#include <SPIFFS.h>
#include <SD.h>
#include <mpu6886.hpp>
#include <ip5306.hpp>
#include <tft_io.hpp>
#include <ili9341.hpp>
#include <w2812.hpp>
#include <htcw_button.hpp>
#include <m5fire_audio.hpp>
#include <lcd_miser.hpp>
#include <gfx.hpp>
// font for example
// not necessary
#include "Ubuntu.hpp"
using namespace arduino;
using namespace gfx;

// pin assignments
constexpr static const uint8_t spi_host = VSPI;
constexpr static const int8_t lcd_pin_bl = 32;
constexpr static const int8_t lcd_pin_dc = 27;
constexpr static const int8_t lcd_pin_rst = 33;
constexpr static const int8_t lcd_pin_cs = 14;
constexpr static const int8_t sd_pin_cs = 4;
constexpr static const int8_t speaker_pin = 25;
constexpr static const int8_t mic_pin = 34;
constexpr static const int8_t button_a_pin = 39;
constexpr static const int8_t button_b_pin = 38;
constexpr static const int8_t button_c_pin = 37;
constexpr static const int8_t led_pin = 15;
constexpr static const int8_t spi_pin_mosi = 23;
constexpr static const int8_t spi_pin_clk = 18;
constexpr static const int8_t spi_pin_miso = 19;

using bus_t = tft_spi_ex<spi_host, 
                        lcd_pin_cs, 
                        spi_pin_mosi, 
                        -1, 
                        spi_pin_clk, 
                        SPI_MODE0,
                        true, 
                        320 * 240 * 2 + 8, 2>;

using lcd_t = ili9342c<lcd_pin_dc, 
                      lcd_pin_rst, 
                      lcd_pin_bl, 
                      bus_t, 
                      1, 
                      true, 
                      400, 
                      200>;

// lcd colors
using color_t = color<typename lcd_t::pixel_type>;
// led strip colors
using lscolor_t = color<typename w2812::pixel_type>;

lcd_t lcd;

// declare the MPU6886 that's attached
// to the first I2C host
mpu6886 gyro(i2c_container<0>::instance());
// the following is equiv at least on the ESP32
// mpu6886 gyro(Wire);

ip5306 power(i2c_container<0>::instance());

m5fire_audio sound;

w2812 led_strips({5,2},led_pin,NEO_GBR);

button<button_a_pin,10,true> button_a;
button<button_b_pin,10,true> button_b;
button<button_c_pin,10,true> button_c;

constexpr static const uint32_t lcd_dimmer_timout_ms = 5*1000;

lcd_miser<lcd_pin_bl,true> lcd_dimmer;

// initialize M5 Stack Fire peripherals/features
void initialize_m5stack_fire() {
    Serial.begin(115200);
    SPIFFS.begin(false);
    SD.begin(4,spi_container<spi_host>::instance());
    lcd.initialize();
    led_strips.initialize();
    led_strips.fill(led_strips.bounds(),lscolor_t::purple);
    lcd.fill(lcd.bounds(),color_t::purple);
    rect16 rect(0,0,64,64);
    rect.center_inplace(lcd.bounds());
    lcd.fill(rect,color_t::white);
    lcd.fill(rect.inflate(-8,-8),color_t::purple);
    gyro.initialize();
    sound.initialize();
    // see https://github.com/m5stack/m5-docs/blob/master/docs/en/core/fire.md
    pinMode(led_pin, OUTPUT_OPEN_DRAIN);
    button_a.initialize();
    button_b.initialize();
    button_c.initialize();
    lcd_dimmer.initialize();
    lcd_dimmer.timeout(lcd_dimmer_timout_ms);
}

void DrawDie(int posX, int posY, int dieValue) {
    draw::filled_rounded_rectangle(lcd, rect16(point16(posX, posY), size16(84, 84)), .1, color_t::white);
    switch(dieValue) {
        case 1:
            draw::filled_ellipse(lcd, rect16(point16(posX + 41, posY + 41), 10), color_t::black);
            break;
        case 2:
            draw::filled_ellipse(lcd, rect16(point16(posX + 13, posY + 13), 10), color_t::black);
            draw::filled_ellipse(lcd, rect16(point16(posX + 69, posY + 69), 10), color_t::black);
            break;
        case 3:
            draw::filled_ellipse(lcd, rect16(point16(posX + 13, posY + 13), 10), color_t::black);
            draw::filled_ellipse(lcd, rect16(point16(posX + 41, posY + 41), 10), color_t::black);
            draw::filled_ellipse(lcd, rect16(point16(posX + 69, posY + 69), 10), color_t::black);
            break;
        case 4:
            draw::filled_ellipse(lcd, rect16(point16(posX + 13, posY + 13), 10), color_t::black);
            draw::filled_ellipse(lcd, rect16(point16(posX + 69, posY + 13), 10), color_t::black);
            draw::filled_ellipse(lcd, rect16(point16(posX + 13, posY + 69), 10), color_t::black);
            draw::filled_ellipse(lcd, rect16(point16(posX + 69, posY + 69), 10), color_t::black);
            break;
        case 5:
            draw::filled_ellipse(lcd, rect16(point16(posX + 13, posY + 13), 10), color_t::black);
            draw::filled_ellipse(lcd, rect16(point16(posX + 69, posY + 13), 10), color_t::black);
            draw::filled_ellipse(lcd, rect16(point16(posX + 41, posY + 41), 10), color_t::black);
            draw::filled_ellipse(lcd, rect16(point16(posX + 13, posY + 69), 10), color_t::black);
            draw::filled_ellipse(lcd, rect16(point16(posX + 69, posY + 69), 10), color_t::black);
            break;
        default:
            draw::filled_ellipse(lcd, rect16(point16(posX + 13, posY + 13), 10), color_t::black);
            draw::filled_ellipse(lcd, rect16(point16(posX + 69, posY + 13), 10), color_t::black);
            draw::filled_ellipse(lcd, rect16(point16(posX + 13, posY + 41), 10), color_t::black);
            draw::filled_ellipse(lcd, rect16(point16(posX + 69, posY + 41), 10), color_t::black);
            draw::filled_ellipse(lcd, rect16(point16(posX + 13, posY + 69), 10), color_t::black);
            draw::filled_ellipse(lcd, rect16(point16(posX + 69, posY + 69), 10), color_t::black);
    }
}

// for the button callbacks
char button_states[3];
void buttons_callback(bool pressed, void* state) {
    char ch = *(char*)state;
    Serial.printf("Button %c %s\n",ch,pressed?"pressed":"released");
}

void setup() {
    initialize_m5stack_fire();
    // setup the button callbacks (optional)
    button_states[0]='a';
    button_states[1]='b';
    button_states[2]='c';
    button_a.callback(buttons_callback,button_states);
    button_b.callback(buttons_callback,button_states+1);
    button_c.callback(buttons_callback,button_states+2);
    
    // your code here
    // example - go ahead and delete
    draw::filled_rectangle(lcd,lcd.bounds(),color_t::black); // Clear

}

void loop() {
    // pump the dimmer
    lcd_dimmer.update();
    // pump the buttons to make sure
    // their callbacks (if any) get
    // fired
    button_a.update();
    button_b.update();
    button_c.update();

    int DiePosition[12] = {17, 24, 118, 24, 219, 24, 17, 132, 118, 132, 219, 132};
    int myDie;
    int c;
    float x,y,z;
 
    gyro.acc(&x,&y,&z);
    if(x>2.0 || y>2.0) {
        for(myDie = 0; myDie <= 5; myDie++) {
            lcd_dimmer.wake();
            c = analogRead(2) % 7;
            while(!(c > 0 && c < 7)) c = analogRead(2) % 7;
            DrawDie(DiePosition[(myDie * 2)], DiePosition[(myDie * 2) + 1], c);
        }
    }

    if(lcd_dimmer.dimmed()) {
        led_strips.fill(led_strips.bounds(),lscolor_t::black);
    }
}