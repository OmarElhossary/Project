

#include "Timer.h"
#include <avr/interrupt.h>

#define  null   ((void*) (0x00))

#define SREG   (*(volatile uint8*)(0x5F))



uint32  Timer1_OVF_FLAG = 0;
uint32  Capture_Counter = 0;
uint32  ICU_Value_1 = 0;
uint32  ICU_Value_2 = 0;
uint32  ICU_Value_3 = 0;


uint32 N_OVFlows ;
uint32 Init_Value;



void ICU_Init(void)
{
	BitClear(DDRD , 6);
	
	ICR1 = 0x0000 ;
	TCCR1B =0x41 ;
	TIMSK = 0X24;
	SREG = 0X80;
}

ISR(TIMER1_OVF_vect)
{
	Timer1_OVF_FLAG++;
}

ISR(TIMER1_CAPT_vect)
{
	Capture_Counter++;
	
	if(Capture_Counter == 1)
	{
		ICU_Value_1 = ICR1;
		Timer1_OVF_FLAG = 0;
		
		BitClear(TCCR1B , 6);
	}
	
	else if(Capture_Counter == 2)
	{
		ICU_Value_2 = ICR1 + (65535 * Timer1_OVF_FLAG);
		
		BitSet(TCCR1B , 6);
	}
	else if(Capture_Counter == 3)
	{
		ICU_Value_3 = ICR1 + (65535 * Timer1_OVF_FLAG);
		
		BitClear(TIMSK , 5);
	}
}


void PWM2_Init(void)
{
	DDRD |= 0X80 ;
	TCCR2 |= 0x6A ;
}

void PWM2_Generate(uint16 duty_cycle)
{
	OCR2 = ((duty_cycle * 256) / 100) - 1 ;
}