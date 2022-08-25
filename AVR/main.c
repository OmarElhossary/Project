


#define  F_CPU 16000000
#include <avr/pgmspace.h>
#include "KS0108.h"
#include "Timer.h"
#include "FONT11x13.h"





extern uint32  Capture_Counter ;
extern uint32  ICU_Value_1 ;
extern uint32  ICU_Value_2 ;
extern uint32  ICU_Value_3 ;


int main(void)
{
  
  	uint32 duty_cycle = 0 , Freq = 0;
  	
  	PWM2_Init();
  	PWM2_Generate(90);
  	ICU_Init();
  	GLCD_Setup();
    GLCD_Clear();
	GLCD_InvertMode();
   
  while(1)
  {
	if(Capture_Counter == 3)
	{
		duty_cycle = (( ICU_Value_2 - ICU_Value_1) * 100)  /  ( ICU_Value_3 - ICU_Value_1) ;
		
		Freq = 8000000 /( ICU_Value_3 - ICU_Value_1) ;
		
		GLCD_SetFont(font11x13,11,13, GLCD_Merge);
		GLCD_GotoXY(0,0);
		GLCD_PrintString("Duty:");
		GLCD_PrintInteger(duty_cycle);
		GLCD_PrintString("%");
		GLCD_SetFont(font11x13,11,13, GLCD_Merge);
		GLCD_GotoXY(0,20);
		GLCD_PrintString("Freq:");
		GLCD_PrintInteger(Freq);
		GLCD_PrintString("Hz");
		GLCD_Render();
		
		Capture_Counter = 0;
		BitSet(TIMSK , 5);
		
		
	}
	  
  }

  
}

