#ifndef KS0108_H_INCLUDED
#define KS0108_H_INCLUDED


//----- Headers ------------//
#define  F_CPU 16000000
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "SITT.h"
#include "CONFG.h"
//--------------------------//

//----- Auxiliary data ---------------------------//
#define __GLCD_Pulse_En					1

#define __GLCD_Command_On				0x3F
#define __GLCD_Command_Off				0x3E
#define __GLCD_Command_Set_Address		0x40
#define __GLCD_Command_Set_Page	   		0xB8
#define __GLCD_Command_Display_Start	0xC0

#define __GLCD_Screen_Width          	128
#define __GLCD_Screen_Height         	64
#define	__GLCD_Screen_Line_Height		8
#define __GLCD_Screen_Lines				__GLCD_Screen_Height / __GLCD_Screen_Line_Height
#define __GLCD_Screen_Chips				2

#define __GLCD_BUSY_FLAG				7

enum Chip_t
{
	Chip_1,
	Chip_2,
	Chip_All
};
enum ReadMode_t
{
	GLCD_Increment,
	GLCD_No_Increment
};
enum OperatingMode_t
{
	GLCD_Inverted,
	GLCD_Non_Inverted
};
enum PrintMode_t
{
	GLCD_Overwrite,
	GLCD_Merge
};
enum Color_t
{
	GLCD_White = 0x00,
	GLCD_Black = 0xFF
};

typedef struct
{
	uint8 *Name;
	uint8 Width;
	uint8 Height;
	uint8 Lines;
	enum PrintMode_t Mode;
}Font_t;

typedef struct
{
	uint8 X;
	uint8 Y;
	enum OperatingMode_t Mode;
	Font_t Font;
}GLCD_t;
//------------------------------------------------//

//----- Prototypes ------------------------------------------------------------//
void GLCD_SendCommand(const uint8 Command, enum Chip_t Chip);
void GLCD_SendData(const uint8 Data, enum Chip_t Chip);
void GLCD_Setup(void);
void GLCD_Render(void);
void GLCD_InvertMode(void);
void GLCD_Clear(void);
void GLCD_GotoX(const uint8 X);
void GLCD_GotoY(const uint8 Y);
void GLCD_GotoXY(const uint8 X, const uint8 Y);
void GLCD_GotoLine(const uint8 line);
void GLCD_FillScreen(enum Color_t Color);
void GLCD_InvertScreen(void);
void GLCD_SetFont(const uint8 *Name, const uint8 Width, const uint8 Height, enum PrintMode_t Mode);
void GLCD_PrintChar(char Character);
void GLCD_PrintString(const char *Text);
void GLCD_PrintInteger(const uint32 Value);
//-----------------------------------------------------------------------------//
#endif