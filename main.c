#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>

#include <hardware/timer.h>
#include <hardware/spi.h>
#include <hardware/clocks.h>
#include <hardware/pll.h>
#include <hardware/structs/pll.h>
#include <hardware/structs/clocks.h>

#include "gpio.h"
#include "st7789/st7789.h"
#include "femtox/TaskMngr.h"
#include "femtox/PlatformSpecific.h"
#include "femtox/String.h"

#define PLL_SYS_KHZ (120 * KHZ)

#define DISPLAY_ON 2
#define DATA_DIR 1
#define DISPLAY_RST 0

#define SPI_TX  7
#define SPI_SCK 6

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240

void measure_freqs(void) {
    uint f_pll_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
    uint f_pll_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
    uint f_rosc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
    uint f_clk_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
    uint f_clk_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
    uint f_clk_adc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
    uint f_clk_rtc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);
}

void initDisplay(struct st7789_config* display) {
    display->spi = spi0;
    display->clk_perif_khz = PLL_SYS_KHZ;
    display->gpio_din = SPI_TX;
    display->gpio_clk = SPI_SCK;
    display->gpio_bl = DISPLAY_ON;
    display->gpio_dc = DATA_DIR;
    display->gpio_rst = DISPLAY_RST;
}

void displayTestTask(BaseSize_t count, BaseParam_t time) {
    Time_t t = (Time_t)time;
    switch(count) {
        case 0:
            gpio_put(BLUE, 1);
            gpio_put(GREEN, 0);
            st7789_fill(ST_COLOR_GREEN);
            count++;
            break;
        case 1: 
            gpio_put(GREEN, 1);
            gpio_put(RED, 0);
            st7789_fill(ST_COLOR_RED);
            count++;
            break;
        case 2:
            gpio_put(RED, 1);
            gpio_put(BLUE, 0);
            st7789_fill(ST_COLOR_BLUE);
            count = 0;
            break;
    }
    SetTimerTask(displayTestTask, count, time, t);
}

static u16 currentBackground = ST_COLOR_BLACK;
static u16 currentColor[2] = {ST_COLOR_WHITE, ST_COLOR_RED};
void showTimeDate() {
    const u08 date_startX = 10;
    const u08 date_startY = 20;
    const u08 time_startX = 35;
    const u08 time_startY = 50;
	static Date_t prev = {0};
	static u16 prevBackGround = ST_COLOR_BLACK;
	static u16 prevFontColor = ST_COLOR_WHITE;
	Date_t current = getDateFromSeconds(getAllSeconds(), TRUE);
	char dateStr[19] = {0};
	dateToString(dateStr, &current);
	strSplit(' ', dateStr);
	if(prevBackGround != currentBackground) {
		st7789_fill(currentBackground);
	}
	if(current.year != prev.year || current.mon != prev.mon || current.day != prev.day || prevFontColor != currentColor[0]) {
		prev.year = current.year;
		prev.mon = current.mon;
		prev.day = current.day;
		st7789_write_string(date_startX,date_startY, dateStr, Font_16x26, currentColor[0], currentBackground);
	}
	if(current.hour != prev.hour || current.min != prev.min || prevFontColor != currentColor[0]) {
		prev.hour = current.hour;
		prev.min = current.min;
		char *timeStr = dateStr + strSize(dateStr) + 1;
		timeStr[6] = END_STRING;
		st7789_write_string(time_startX,time_startY, timeStr, Font_16x26, currentColor[0], currentBackground);
	}
	st7789_write_string(time_startX+16*6,time_startY, dateStr+15, Font_16x26, currentColor[1], currentBackground);
	prevBackGround = currentBackground;
	prevFontColor = currentColor[0];
}

void standWithUkraine(u32 xy, BaseParam_t logoXY) {
    u16 x = (u16)xy & 0xFFFF;
    u16 y = (u16)(xy >> 16);
    u32 logo = (u32)(logoXY);
    u16 logoX = (u16)logo & 0xFFFF;
    u16 logoY = (u16)(logo>>16);
	st7789_write_string(x, y, "WITH UKRAINE", Font_11x18, currentColor[0], currentBackground);
	st7789_draw_filled_rectangle(logoX, logoY, 60, 20, ST_COLOR_BLUE);
	st7789_draw_filled_rectangle(logoX, logoY+20, 60, 20, ST_COLOR_YELLOW);
}


void testBtnClick(BaseSize_t count, BaseParam_t tickTime) {
    Time_t ticks = (Time_t)tickTime;
    printf("button clicked by %d ticks\n", ticks);
}

void testBtnPressed() {
    puts("button pressed");
}

void testBtnRelesed() {
    puts("button released");
}

void testButton(){
    connectTaskToSignal((TaskMng)testBtnClick, ClickEvent);
    connectTaskToSignal((TaskMng)testBtnPressed, PressedEvent);
    connectTaskToSignal((TaskMng)testBtnRelesed, ReleasedEvent);
}

void disableDisplay(BaseSize_t arg_n, BaseParam_t arg_p);
void enableDisplay(BaseSize_t n, BaseParam_t arguments);
void stopwatchTask();

static u32 stopwatchTimer;
static void showTimer() {
    updateTimer(disableDisplay,0, NULL, TICK_PER_SECOND<<1);
    u32 currentTicks = getTick();
    u32 ticks = 0;
    if(currentTicks > stopwatchTimer) {
        ticks = currentTicks - stopwatchTimer;
    } else {
        ticks = 0xFFFFFFFF - stopwatchTimer + currentTicks;
    }
    Time_t seconds = ticks / TICK_PER_SECOND;
    ticks %= TICK_PER_SECOND;
    char secondsStr[10];
    char ticksStr[5];
    toStringDec(seconds, secondsStr);
    toStringDec(ticks, ticksStr);
    st7789_write_string(10 ,SCREEN_HEIGHT/2-10, secondsStr, Font_16x26, currentColor[0], currentBackground);
    st7789_write_string(SCREEN_WIDTH-strSize(ticksStr)*16-10, SCREEN_HEIGHT/2-10, ticksStr, Font_16x26, currentColor[1], currentBackground);
}

void clearStopWatch() {
    st7789_draw_filled_rectangle(10, SCREEN_HEIGHT/2-10, SCREEN_WIDTH, 26, currentBackground);
    execCallBack(clearStopWatch);
}

void stopwatchTask() {
    static u08 state = 0;
    if(state) {
        SetTask((TaskMng)delCycleTask, 0, (BaseParam_t)showTimer);
        updateTimer(disableDisplay,0, NULL, 12*TICK_PER_SECOND);
        state = 0;
        execCallBack(stopwatchTask);
        return;
    }
    state++;
    stopwatchTimer = getTick();
    clearStopWatch();
    SetCycleTask(TICK_PER_SECOND>>4, showTimer, TRUE);
}

void disableDisplay(BaseSize_t arg_n, BaseParam_t arg_p) {
    disconnectTaskFromSignal(stopwatchTask, PressedEvent);
    connectTaskToSignal(enableDisplay, PressedEvent);
	delCycleTask(arg_n, showTimeDate);
    clearStopWatch();
	display_enable(false);
	execCallBack(disableDisplay);
}

void enableDisplay(BaseSize_t n, BaseParam_t arguments) {
    disconnectTaskFromSignal(enableDisplay, PressedEvent);
    connectTaskToSignal(stopwatchTask, PressedEvent);
	Time_t seconds = 12;
	if(n>0) {
		seconds = toIntDec(arguments);
	}
	display_enable(true);
	SetCycleTask(TICK_PER_SECOND, showTimeDate, TRUE);
	SetTimerTask(disableDisplay,0, NULL, seconds*TICK_PER_SECOND);
	execCallBack(enableDisplay);
}

static void displayCtr() {
    connectTaskToSignal(enableDisplay, PressedEvent);
}

int main() {
    // Set the system frequency to 133 MHz. vco_calc.py from the SDK tells us
    // this is exactly attainable at the PLL from a 12 MHz crystal: FBDIV =
    // 133 (so VCO of 1596 MHz), PD1 = 6, PD2 = 2. This function will set the
    // system PLL to 133 MHz and set the clk_sys divisor to 1.
    set_sys_clock_khz(PLL_SYS_KHZ, true);

    // The previous line automatically detached clk_peri from clk_sys, and
    // attached it to pll_usb, so that clk_peri won't be disturbed by future
    // changes to system clock or system PLL. If we need higher clk_peri
    // frequencies, we can attach clk_peri directly back to system PLL (no
    // divider available) and then use the clk_sys divider to scale clk_sys
    // independently of clk_peri.
    clock_configure(
        clk_peri,
        CLOCKS_CLK_PERI_CTRL_AUXSRC_RESET,                // No glitchless mux
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
        PLL_SYS_KHZ * KHZ,                               // Input frequency
        PLL_SYS_KHZ * KHZ                                // Output (must be same as no divider)
    );

    stdio_init_all();
    initLED();
    initInput();
    struct st7789_config display;
    initDisplay(&display);
    for(uint8_t i = 0; i<2; i++) {
        st7789_init(&display, SCREEN_WIDTH, SCREEN_HEIGHT);
    }
    st7789_fill(currentBackground);
    st7789_rotate_display(3);
    initFemtOS();
    setSeconds(1645653600); // 24.02.22 russia-ukraine war start
    initWatchDog();
    SetCycleTask(TICK_PER_SECOND>>1, resetWatchDog, TRUE);
    SetIdleTask(idle);
    SetTask(standWithUkraine, (SCREEN_HEIGHT-40)<<16|20, (BaseParam_t)(((u32)(SCREEN_HEIGHT-40))<<16 | (SCREEN_WIDTH-60)));
    SetTask((TaskMng)testButton, 0, NULL);
    SetTask((TaskMng)displayCtr, 0, NULL);
    multicore_launch_core1(runFemtOS);
    runFemtOS();
    return 0;
}

