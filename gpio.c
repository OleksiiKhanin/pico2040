#include "gpio.h"
#include "femtox/TaskMngr.h"
#include "femtox/PlatformSpecific.h"

#include <hardware/gpio.h>

static const Time_t checkButtonDelay = TICK_PER_SECOND>>4;

static void checkBtnPressed(BaseSize_t count, BaseParam_t arg_p);
static void checkBtnReleased(BaseSize_t count, BaseParam_t arg_p);
static void buttonPressedHandler(uint gpio, uint32_t event);
const void* ClickEvent = (void*)buttonPressedHandler;
const void* PressedEvent = (void*)checkBtnPressed;
const void* ReleasedEvent = (void*)checkBtnReleased;

static void checkBtnPressed(BaseSize_t count, BaseParam_t arg_p) {
    bool_t state = gpio_get(BUTTON);
    if(count == MIN_COUNT_CHECK_BTN) {
        emitSignal(PressedEvent, 0, NULL);
    }
    if(!state) {
        SetTimerTask(checkBtnPressed, count+1, (BaseParam_t)0, checkButtonDelay);
        return;
    } else if(count >= MIN_COUNT_CHECK_BTN) {
        emitSignal(ClickEvent, count, (BaseParam_t)(count*(checkButtonDelay)));
    }
    gpio_set_irq_enabled(BUTTON, GPIO_IRQ_EDGE_FALL, true);
}

static void checkBtnReleased(BaseSize_t count, BaseParam_t arg_p) {
    bool state = gpio_get(BUTTON);
    if(state && count < MIN_COUNT_CHECK_BTN) {
        SetTimerTask(checkBtnReleased, count+1, arg_p, checkButtonDelay);
        return;
    } else if(state) {
        emitSignal(ReleasedEvent, 0, NULL);
    }
    gpio_set_irq_enabled(BUTTON, GPIO_IRQ_EDGE_RISE, true);
}

static void buttonPressedHandler(uint gpio, uint32_t event) {
    gpio_set_irq_enabled(
        BUTTON, 
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
        false);
    if(event && GPIO_IRQ_EDGE_FALL) {
        SetTask(checkBtnPressed, 0, NULL); //check button still pressed
    }
    if(event && GPIO_IRQ_EDGE_RISE) {
        SetTask(checkBtnReleased, 0, NULL); //check button still pressed
    }
}

void initInput() {
    gpio_init(BUTTON);
    gpio_set_dir(BUTTON, GPIO_IN);
    gpio_pull_up(BUTTON);
    gpio_set_irq_enabled_with_callback(
        BUTTON, 
        GPIO_IRQ_EDGE_FALL |
        GPIO_IRQ_EDGE_RISE,
        true, 
        buttonPressedHandler);
}

void initLED() {
    gpio_init(BLUE);
    gpio_init(GREEN);
    gpio_init(RED);
    gpio_set_dir(BLUE, GPIO_OUT);
    gpio_set_dir(GREEN, GPIO_OUT);
    gpio_set_dir(RED, GPIO_OUT);
    gpio_set_slew_rate(BLUE, GPIO_SLEW_RATE_SLOW);
    gpio_set_slew_rate(GREEN, GPIO_SLEW_RATE_SLOW);
    gpio_set_slew_rate(RED, GPIO_SLEW_RATE_SLOW);
    gpio_put(BLUE, 1);
    gpio_put(GREEN, 1);
    gpio_put(RED, 1);
}
