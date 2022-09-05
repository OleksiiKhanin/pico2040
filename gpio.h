#ifndef GPIO_H_
#define GPIO_H_

#define BLUE 20
#define GREEN 19
#define RED 18

#define BUTTON 23

#define MIN_COUNT_CHECK_BTN 2

void initLED();
void initInput();

extern const void* ClickEvent;
extern const void* PressedEvent;
extern const void* ReleasedEvent;

#endif /*GPIO_H_*/