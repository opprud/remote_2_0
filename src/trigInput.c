/*
 * trigInput.c
 * @brief trigger input, detects AC input, between 10 - 100Hz or DC on optocoupler input
 *
 * @date Jan 18, 2015
 * @author mortenjakobsen
 */
#include "driver_config.h"
#include "trigInput.h"
#include "m0utils.h"
#include "defs.h"
#include "gpio.h"

#define TRIG_IN_PIN_1			10
#define TRIG_IN_PORT_1			1
#define TRIG_IN_PIN_2			2
#define TRIG_IN_PORT_2			2

#define TRIG_ACTIVE_LEVEL	0	//open collector pulled to ground

#define	TRIG_ON_AC		1
#define	TRIG_ON_DC		1

#define TRIG_TIME_MS_MAX	200

volatile unsigned long timeEdge1, timeLast1;
volatile unsigned long timeEdge2, timeLast2;

/**
 * @brief	interrupt handler, updates timer, when fired
 *
 * */
void PIOINT1_IRQHandler(void)
{
	GPIOIntClear( TRIG_IN_PORT_1, TRIG_IN_PIN_1);
	/* detect change time */
	timeEdge1 = millis();
}

/**
 * @brief	interrupt handler, updates timer, when fired
 *
 * */
void PIOINT2_IRQHandler(void)
{
	GPIOIntClear( TRIG_IN_PORT_2, TRIG_IN_PIN_2);
	/* detect change time */
	timeEdge2 = millis();
}

/**
 * *brief	init trig pin as input, install interrupt handler on event 1
 * */
void trigInputInit(void)
{
	/* init timer */
	timeEdge1 = 0;
	timeEdge2 = 0;

	/* allow interrupts on GPIO 1 & 2 */
	NVIC_EnableIRQ(EINT1_IRQn);
	NVIC_EnableIRQ(EINT2_IRQn);

	GPIOSetDir( TRIG_IN_PORT_1, TRIG_IN_PIN_1, 0);
	GPIOSetDir( TRIG_IN_PORT_2, TRIG_IN_PIN_2, 0);
	/* attach pin to falling edge Interrupt */
	GPIOSetInterrupt(TRIG_IN_PORT_1, TRIG_IN_PIN_1, 0, 0, 0);
	GPIOSetInterrupt(TRIG_IN_PORT_2, TRIG_IN_PIN_2, 0, 0, 0);
	/*enable interrupts*/
	GPIOIntEnable( TRIG_IN_PORT_1, TRIG_IN_PIN_1);
	GPIOIntEnable( TRIG_IN_PORT_2, TRIG_IN_PIN_2);
}

/**
 * @brief	calculate time between sucessive AC trigger pulses, or read a DC signal,
 * depending on configuration, return trigger status if signal is valid
 *
 * @return	TRIGGER_ACTIVE or	NO_TRIGGER
 * */
int trigInputRead(void)
{
	unsigned long timeNow1;
	unsigned long timeNow2;
	int ret = NO_TRIGGER;

	/* get diff in ms */
	timeNow1 = millis();
	timeNow2 = timeNow1;

#ifdef TRIG_ON_AC
	/* any changes within the last 100mS, the we have a trig....*/
	if ((timeNow1 - timeEdge1) < TRIG_TIME_MS_MAX)
		ret =  TRIGGER1_ACTIVE;

	/* any changes within the last 100mS, the we have a trig....*/
	if ((timeNow2 - timeEdge2) < TRIG_TIME_MS_MAX)
		ret |=  TRIGGER2_ACTIVE;
#endif

//#ifdef TRIG_ON_DC
#if 0
	/* if trigger is low, we have DC*/
	if (!(LPC_GPIO_PORT->PIN0 & TRIG_IN_PIN))
	{
		return TRIGGER_ACTIVE;
	}
#endif
	return ret;
}
