#include "../../remote_lpc1114/config/driver_config.h"
#include "../../remote_lpc1114/config/target_config.h"
#include "../../remote_lpc1114/driver/gpio.h"
#include "../../remote_lpc1114/driver/timer32.h"
#include "../../remote_lpc1114/src/defs.h"
#include "../../remote_lpc1114/src/led.h"
#include "../../remote_lpc1114/src/m0utils.h"
#include "../../remote_lpc1114/src/rfcomm.h"
#include "../../remote_lpc1114/src/RF22.h"
#include "../../remote_lpc1114/src/RHReliableDatagram.h"
#include "../../remote_lpc1114/src/spi.h"
#include "../../remote_lpc1114/src/trigInput.h"

unsigned int MsCount;

#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2

uint8_t data[] = "Hello World!";
// Dont put this on the stack:
uint8_t buf[44];

/* SysTick interrupt happens every 10 ms */
void SysTick_Handler(void)
{
	//GPIOSetValue( LED_PORT, LED_BIT, LED_OFF);

	MsCount += 1;
	//GPIOSetValue( LED_PORT, LED_BIT, LED_ON);
}

/**
 * @brief	read HW adress from 4 input jumpers
 *
 * @return	HW_ID 4 bits
 * */
void readHwId(void)
{
	unsigned int val;
#if 0
	val = LPC_GPIO_PORT->PIN0;
	MY_ID =1;	//ID1 is the lowest possible
	if(val & (1 << ID_BIT0))
	MY_ID += 1;
	if(val & (1 << ID_BIT1))
	MY_ID += 2;
	if(val & (1 << ID_BIT2))
	MY_ID += 4;
	if(!(val & (1 << ID_BIT3)))
	MY_ID += 8;
#endif
}

/**
 * @brief	initialization error, when communicating with RFM22
 * 			signal to the world using the LED
 * */
void initError(void)
{
	/* display SOS -*- on the LED */
	while (1)
	{
		updateLed(LED_ERROR);
	}
}

/**
/**
 * @brief 	setup systems & platform
 *
 * */
void sysInit(void)
{

	//__disable_irq();
	/* update core clk */
	SystemCoreClockUpdate();

	/* enale brown out reset when VCC < 2,4V*/
	//BOD_Init();
	/* Called for system library in core_cmx.h(x=0 or 3). */
	SysTick_Config((SysTick->CALIB / 10) + 1);

	/* init periphereals*/
	GPIOInit();
	//initKeys(); - NOT used, we use pins instead, for reading HW ID, since they have pull up/dwn
	initLed();
	trigInputInit();

	/* #NSEL is GPIO controlled*/
	GPIOSetDir(SEL_PORT, SEL_PIN, 1);
	GPIOSetValue(SEL_PORT, SEL_PIN, 1);

	/* SDN - shutdown is GPIO controlled*/
	GPIOSetDir(SDN_PORT, SDN_PIN, 1);
	GPIOSetValue(SDN_PORT, SDN_PIN, 0);	//dont shut down

	/* Config pins for SPI port 0*/

	/* Setup SPI port 0*/
	SPI_Init();

	/* init radio ( chip select - D6/p0.6 ; nIRQ - D56/p2.4 ; 1 == SSP1 */
	if (RF22init(6, 56, 1) == 0)
		initError();

	/* 17dBm TX power - max is 20dBm */
	setTxPower(RF22_TXPOW_17DBM);

	initRHReliableDatagram(1);

#if 1
	/*  un-select RFM22 */
	//GPIOSetDir(SEL_PORT, SEL_PIN, 1);
	/* inputs for reading HW adress */
	GPIOSetDir(ID_PORT0, ID_BIT0, 0);
	GPIOSetDir(ID_PORT1, ID_BIT1, 0);
	GPIOSetDir(ID_PORT2, ID_BIT2, 0);
	GPIOSetDir(ID_PORT3, ID_BIT3, 0);

	/*set pulldowns - solder jumpers connects to 3v3 */
	LPC_IOCON->PIO2_10 &= ~0x18;
	LPC_IOCON->PIO2_10 |= 0x8;
	LPC_IOCON->PIO2_11 &= ~0x18;
	LPC_IOCON->PIO2_11 |= 0x8;
	LPC_IOCON->PIO3_0 &= ~0x18;
	LPC_IOCON->PIO3_0 |= 0x8;
	LPC_IOCON->PIO3_1 &= ~0x18;
	LPC_IOCON->PIO3_1 |= 0x8;
#endif

	//__enable_irq();
}

/* Main Program */

int main(void)
{

	uint8_t to;
	uint8_t id;
	uint8_t flags;

	/* 0 = ignore */
	to = id = flags = 0;

	// Now wait for a reply from the server
	uint8_t len = sizeof(buf);
	uint8_t from;

	LED_STATUS_t state = LED_IDLE;

	//while(0);
	/* setup GPIO and RFM22 */
	sysInit();

	/*rxta all*/
	setPromiscuous(1);

	/* allow system to settle ?WTF? before reading HW ID*/
	delay(300);

//	recvfromAckTimeout(buf, &len, 2000, &from, &to, &id, &flags);
	while (1)
	{

		if (available())
		{
			// Wait for a message addressed to us from the client
			uint8_t len = sizeof(buf);
			uint8_t from;
			//if (recvfromAck(buf, &len, &from))
			if (recvfromAckTimeout(buf, &len, 2000, &from, &to, &id, &flags))
			{
				//printf("got request from : 0x");
				/*Serial.print(from, HEX);
				Serial.print(": ");
				Serial.println((char*) buf);
*/
				// Send a reply back to the originator client
				if (!sendtoWait(data, sizeof(data), from))
					from--;
					//printf("sendtoWait failed");
			}
		}

		/* update LED's*/
		if (updateLed(state) == SEQUENCE_END)
			state = LED_IDLE;

	}
}

