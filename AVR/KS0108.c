#include "KS0108.h"

uint8 __GLCD_Buffer[__GLCD_Screen_Width][__GLCD_Screen_Lines];
	
GLCD_t __GLCD;

#define __GLCD_XtoChip(X)		((X < (__GLCD_Screen_Width / __GLCD_Screen_Chips)) ? Chip_1 : Chip_2)

//---------------------------//

//----- Prototypes ----------------------------//
static void GLCD_Send(const uint8 Data);
static void GLCD_WaitBusy(enum Chip_t Chip);
static void GLCD_BufferWrite(const uint8 X, const uint8 Y, const uint8 Data);
static uint8 GLCD_BufferRead(const uint8 X, const uint8 Y);
static void GLCD_SelectChip(enum Chip_t Chip);
static void __GLCD_GotoX(const uint8 X);
static void __GLCD_GotoY(const uint8 Y);
static void Int2bcd(uint32 Value, char BCD[]);
static inline void Pulse_En(void);
//---------------------------------------------//

//----- Functions -------------//
void GLCD_SendCommand(const uint8 Command, enum Chip_t Chip)
{
	//Check if busy
	if (Chip != Chip_All)
	{
		GLCD_WaitBusy(Chip);
	}
	else
	{
		GLCD_WaitBusy(Chip_1);
		GLCD_WaitBusy(Chip_2);
	}
	GLCD_SelectChip(Chip);
	
	DigitalWrite(GLCD_DI, Low);      //RS = 0
	DigitalWrite(GLCD_RW, Low);      //RW = 0
	
	//Send data
	GLCD_Send(Command);
}

void GLCD_SendData(const uint8 Data, enum Chip_t Chip)
{
	//Check if busy
	if (Chip != Chip_All)
	{
		GLCD_WaitBusy(Chip);
	}
	else
	{
		GLCD_WaitBusy(Chip_1);
		GLCD_WaitBusy(Chip_2);
	}
	GLCD_SelectChip(Chip);

	DigitalWrite(GLCD_DI, High);     //RS = 1
	DigitalWrite(GLCD_RW, Low);      //RW = 0

	//Send data
	GLCD_Send(__GLCD.Mode == GLCD_Non_Inverted ? Data : ~Data);
	
	__GLCD.X++;
	if (__GLCD.X == (__GLCD_Screen_Width / __GLCD_Screen_Chips))
		__GLCD_GotoX(__GLCD.X);
	else if (__GLCD.X >= __GLCD_Screen_Width)
		__GLCD.X = __GLCD_Screen_Width - 1;
}

void GLCD_Setup(void)
{
	//Setup pins
	PinMode(GLCD_D0, Output);	//GLCD pins = Outputs
	PinMode(GLCD_D1, Output);
	PinMode(GLCD_D2, Output);
	PinMode(GLCD_D3, Output);
	PinMode(GLCD_D4, Output);
	PinMode(GLCD_D5, Output);
	PinMode(GLCD_D6, Output);
	PinMode(GLCD_D7, Output);

	PinMode(GLCD_CS1, Output);
	PinMode(GLCD_CS2, Output);
	PinMode(GLCD_DI, Output);
	PinMode(GLCD_EN, Output);
	PinMode(GLCD_RW, Output);
	PinMode(GLCD_RST, Output);

	DigitalWrite(GLCD_DI, Low);		//GLCD pins = 0
	DigitalWrite(GLCD_RW, Low);
	DigitalWrite(GLCD_EN, Low);
	
	DigitalWrite(GLCD_RST, Low);	//!RST
	_delay_ms(5);
	DigitalWrite(GLCD_RST, High);
	_delay_ms(50);

	//Initialize chips
	GLCD_SendCommand(__GLCD_Command_On, Chip_All);
	GLCD_SendCommand(__GLCD_Command_Display_Start, Chip_All);

	//Go to 0,0
	GLCD_GotoXY(0, 0);
	
	//Reset GLCD structure
	__GLCD.Mode = GLCD_Non_Inverted;
	__GLCD.X = __GLCD.Y = __GLCD.Font.Width = __GLCD.Font.Height = __GLCD.Font.Lines = 0;
}

void GLCD_Render(void)
{
	uint8 i, j;
	
	for (j = 0 ; j < __GLCD_Screen_Height ; j += __GLCD_Screen_Line_Height)
	{
		__GLCD_GotoX(0);
		__GLCD_GotoY(j);
		for (i = 0 ; i < __GLCD_Screen_Width ; i++)
			GLCD_SendData(GLCD_BufferRead(i, __GLCD.Y), __GLCD_XtoChip(i));
	}
}

void GLCD_InvertMode(void)
{
	if (__GLCD.Mode == GLCD_Inverted)
		__GLCD.Mode = GLCD_Non_Inverted;
	else
		__GLCD.Mode = GLCD_Inverted;
}

void GLCD_Clear(void)
{
	GLCD_FillScreen(__GLCD.Mode == GLCD_Non_Inverted ? GLCD_White : GLCD_Black);
}

void GLCD_GotoX(const uint8 X)
{
	if (X < __GLCD_Screen_Width)
		__GLCD.X = X;
}

void GLCD_GotoY(const uint8 Y)
{
	if (__GLCD.Y < __GLCD_Screen_Height)
		__GLCD.Y = Y;
}

void GLCD_GotoXY(const uint8 X, const uint8 Y)
{
	GLCD_GotoX(X);
	GLCD_GotoY(Y);
}

void GLCD_GotoLine(const uint8 Line)
{
	if (Line < __GLCD_Screen_Lines)
		__GLCD.Y = Line * __GLCD_Screen_Line_Height;
}

void GLCD_FillScreen(enum Color_t Color)
{
	uint8 i, j;

	for (j = 0 ; j < __GLCD_Screen_Height ; j += __GLCD_Screen_Line_Height)
		for (i = 0 ; i < __GLCD_Screen_Width ; i++)
			GLCD_BufferWrite(i, j, Color);
}

void GLCD_SetFont(const uint8 *Name, const uint8 Width, const uint8 Height, enum PrintMode_t Mode)
{
	if ((Width < __GLCD_Screen_Width) && (Height < __GLCD_Screen_Height) && ((Mode == GLCD_Overwrite) || (Mode == GLCD_Merge)))
	{
		//Change font pointer to new font
		__GLCD.Font.Name = (uint8 *)(Name);
		
		//Update font's size
		__GLCD.Font.Width = Width;
		__GLCD.Font.Height = Height;
		
		//Update lines required for a character to be fully displayed
		__GLCD.Font.Lines = (Height - 1) / __GLCD_Screen_Line_Height + 1;
		
		//Update blending mode
		__GLCD.Font.Mode = Mode;
	}
}

void GLCD_PrintChar(char Character)
{
	uint16 fontStart, fontRead, fontReadPrev;
	uint8 x, y, y2, i, j, width, lines, overflow, data, dataPrev;
	fontStart = fontRead = fontReadPrev = x = y = y2 = i = j = width = lines = overflow = data = dataPrev = 0;
	
	//#1 - Save current position
	x = __GLCD.X;
	y = y2 = __GLCD.Y;
	
	//#2 - Remove leading empty characters
	Character -= 32;														//32 is the ASCII of the first printable character
	
	//#3 - Find the start of the character in the font array
	fontStart = Character * (__GLCD.Font.Width * __GLCD.Font.Lines + 1);		//+1 due to first byte of each array line being the width
	
	//#4 - Update width - First byte of each line is the width of the character
	width = pgm_read_byte(&(__GLCD.Font.Name[fontStart++]));
	data = __GLCD.X + width;											//"data" is used temporarily
	//If character exceed screen bounds, reduce
	if (data >= __GLCD_Screen_Width)
		width -= data-__GLCD_Screen_Width;
	
	//#5 - Update lines
	lines = __GLCD.Font.Lines;
	data = __GLCD.Y / __GLCD_Screen_Line_Height + lines;				//"data" is used temporarily
	//If character exceed screen bounds, reduce
	if (data > __GLCD_Screen_Lines)
		lines -= data - __GLCD_Screen_Lines;
	
	//#6 - Calculate overflowing bits
	overflow = __GLCD.Y % __GLCD_Screen_Line_Height;
		
	//#7 - Print the character
	//Scan the lines needed
	for (j = 0 ; j < lines ; j++)
	{
		//Go to the start of the line
		GLCD_GotoXY(x, y);
		
		//Update the indices for reading the line
		fontRead = fontStart + j;
		fontReadPrev = fontRead - 1;

		//Scan bytes of selected line
		for (i = 0 ; i < width ; i++)
		{
			//Read byte
			data = pgm_read_byte(&(__GLCD.Font.Name[fontRead]));
			
			//Shift byte
			data <<= overflow;
			
			//Merge byte with previous one
			if (j > 0)
			{
				dataPrev = pgm_read_byte(&(__GLCD.Font.Name[fontReadPrev]));
				dataPrev >>= __GLCD_Screen_Line_Height - overflow;
				data |= dataPrev;
				fontReadPrev += __GLCD.Font.Lines;
			}
			//Edit byte depending on the mode
			if (__GLCD.Font.Mode == GLCD_Merge)
				data |= GLCD_BufferRead(__GLCD.X, __GLCD.Y);
			
			//Send byte
			GLCD_BufferWrite(__GLCD.X++, __GLCD.Y, data);
			
			//Increase index
			fontRead += __GLCD.Font.Lines;
		}
		//Send an empty column of 1px in the end
		if (__GLCD.Font.Mode == GLCD_Overwrite)
			data = GLCD_White;
		else
			data = GLCD_BufferRead(__GLCD.X, __GLCD.Y);
		GLCD_BufferWrite(__GLCD.X, __GLCD.Y, data);
		
		//Increase line counter
		y += __GLCD_Screen_Line_Height;
	}

	//#8 - Update last line, if needed
	if (lines > 1)
	{
		//Go to the start of the line
		GLCD_GotoXY(x, y);
		
		//Update the index for reading the last printed line
		fontReadPrev = fontStart + j - 1;

		//Scan bytes of selected line
		for (i = 0 ; i < width ; i++)
		{
			//Read byte
			data = GLCD_BufferRead(__GLCD.X, __GLCD.Y);
			
			//Merge byte with previous one
			dataPrev = pgm_read_byte(&(__GLCD.Font.Name[fontReadPrev]));
			dataPrev >>= __GLCD_Screen_Line_Height - overflow;
			data |= dataPrev;
			
			//Edit byte depending on the mode
			if (__GLCD.Font.Mode == GLCD_Merge)
				data |= GLCD_BufferRead(__GLCD.X, __GLCD.Y);
			
			//Send byte
			GLCD_BufferWrite(__GLCD.X, __GLCD.Y, data);

			//Increase index
			fontReadPrev += __GLCD.Font.Lines;
		}
		//Send an empty column of 1px in the end
		if (__GLCD.Font.Mode == GLCD_Overwrite)
			data = GLCD_White;
		else if (__GLCD.Font.Mode == GLCD_Merge)
			data = GLCD_BufferRead(__GLCD.X, __GLCD.Y);
		else
			data = ~GLCD_BufferRead(__GLCD.X, __GLCD.Y);
		GLCD_BufferWrite(__GLCD.X++, __GLCD.Y,data);
	}

	
	//Move cursor to the end of the printed character
	GLCD_GotoXY(x + width + 1, y2);
}

void GLCD_PrintString(const char *Text)
{
	while(*Text)
	{
		if ((__GLCD.X + __GLCD.Font.Width) >= __GLCD_Screen_Width)
			break;
		
		GLCD_PrintChar(*Text++);
	}
}

void GLCD_PrintInteger(const uint32 Value)
{
	if (Value == 0)
	{
		GLCD_PrintChar('0');
	}
	else if ((Value > 0) && (Value <= 4000000))
	{
		//int32_max + sign + null = 12 bytes
		char bcd[12] = { '\0' };
		
		//Convert integer to array
		Int2bcd(Value, bcd);
		
		//Print from first non-zero digit
		GLCD_PrintString(bcd);
	}
}

static void GLCD_Send(const uint8 Data)
{
	//Send nibble
	DigitalWrite(GLCD_D0, BitCheck(Data, 0));
	DigitalWrite(GLCD_D1, BitCheck(Data, 1));
	DigitalWrite(GLCD_D2, BitCheck(Data, 2));
	DigitalWrite(GLCD_D3, BitCheck(Data, 3));
	DigitalWrite(GLCD_D4, BitCheck(Data, 4));
	DigitalWrite(GLCD_D5, BitCheck(Data, 5));
	DigitalWrite(GLCD_D6, BitCheck(Data, 6));
	DigitalWrite(GLCD_D7, BitCheck(Data, 7));
	Pulse_En();
}

static void GLCD_WaitBusy(enum Chip_t Chip)
{
	uint8 status = 0;
	
	GLCD_SelectChip(Chip);
	
	//Busy pin = Input
	PinMode(GLCD_D7, Input);

	DigitalWrite(GLCD_DI, Low);
	DigitalWrite(GLCD_RW, High);
	DigitalWrite(GLCD_EN, Low);
	_delay_us(__GLCD_Pulse_En);
	
	//Send Enable pulse and wait till busy flag goes Low
	do
	{
		DigitalWrite(GLCD_EN, High);
		_delay_us(__GLCD_Pulse_En);
		
		status = DigitalRead(GLCD_D7) << 7;
		
		DigitalWrite(GLCD_EN, Low);
		_delay_us(__GLCD_Pulse_En<<3);
	}
	while(BitCheck(status, __GLCD_BUSY_FLAG));

	DigitalWrite(GLCD_RW, Low);
	
	//Busy pin = Output
	PinMode(GLCD_D7, Output);
}

static void GLCD_BufferWrite(const uint8 X, const uint8 Y, const uint8 Data)
{
	//a>>3 = a/8
	__GLCD_Buffer[X][Y>>3] = Data;
}

static uint8 GLCD_BufferRead(const uint8 X, const uint8 Y)
{
	//a>>3 = a/8
	return (__GLCD_Buffer[X][Y>>3]);
}

static void GLCD_SelectChip(enum Chip_t Chip)
{
	uint8 on, off;

	#if (GLCD_ACTIVE_LOW != 0)
		on = 0;
		off = 1;
	#else
		on = 1;
		off = 0;
	#endif

	if(Chip == Chip_1)
	{
		DigitalWrite(GLCD_CS2, off);
		DigitalWrite(GLCD_CS1, on);
	}
	else if (Chip == Chip_2)
	{
		DigitalWrite(GLCD_CS1, off);
		DigitalWrite(GLCD_CS2, on);
	}
	else
	{
		DigitalWrite(GLCD_CS1, on);
		DigitalWrite(GLCD_CS2, on);
	}
}

static void __GLCD_GotoX(const uint8 X)
{
	if (X < __GLCD_Screen_Width)
	{
		uint8_t chip, cmd;
		
		//Update command
		chip = __GLCD_XtoChip(X);
		cmd = ((chip == Chip_1) ? (__GLCD_Command_Set_Address | X) : (__GLCD_Command_Set_Address | (X - __GLCD_Screen_Width / __GLCD_Screen_Chips)));
		
		//Update tracker
		__GLCD.X = X;
		
		//Send command
		GLCD_SendCommand(cmd, chip);
	}
}

static void __GLCD_GotoY(const uint8 Y)
{
	if (Y < __GLCD_Screen_Height)
	{
		uint8 cmd;
		
		//Update command
		cmd = __GLCD_Command_Set_Page | (Y / __GLCD_Screen_Line_Height);
				
		//Update tracker
		__GLCD.Y = Y;
		
		//Send command
		GLCD_SendCommand(cmd, Chip_All);
	}
}

static inline void Pulse_En(void)
{
	DigitalWrite(GLCD_EN, High);
	_delay_us(__GLCD_Pulse_En);
	DigitalWrite(GLCD_EN, Low);
	_delay_us(__GLCD_Pulse_En);
}

static void Int2bcd(uint32 Value, char BCD[])
{
	uint8 isNegative = 0;
	
	BCD[0] = BCD[1] = BCD[2] =
	BCD[3] = BCD[4] = BCD[5] =
	BCD[6] = BCD[7] = BCD[8] =
	BCD[9] = BCD[10] = '0';
	
	if (Value < 0)
	{
		isNegative = 1;
		Value = -Value;
	}
	
	while (Value > 1000000000)
	{
		Value -= 1000000000;
		BCD[1]++;
	}
	
	while (Value >= 100000000)
	{
		Value -= 100000000;
		BCD[2]++;
	}
	
	while (Value >= 10000000)
	{
		Value -= 10000000;
		BCD[3]++;
	}
	
	while (Value >= 1000000)
	{
		Value -= 1000000;
		BCD[4]++;
	}
	
	while (Value >= 100000)
	{
		Value -= 100000;
		BCD[5]++;
	}

	while (Value >= 10000)
	{
		Value -= 10000;
		BCD[6]++;
	}

	while (Value >= 1000)
	{
		Value -= 1000;
		BCD[7]++;
	}
	
	while (Value >= 100)
	{
		Value -= 100;
		BCD[8]++;
	}
	
	while (Value >= 10)
	{
		Value -= 10;
		BCD[9]++;
	}

	while (Value >= 1)
	{
		Value -= 1;
		BCD[10]++;
	}

	uint8_t i = 0;
	//Find first non zero digit
	while (BCD[i] == '0')
	i++;

	//Add sign
	if (isNegative)
	{
		i--;
		BCD[i] = '-';
	}

	//Shift array
	uint8_t end = 10 - i;
	uint8_t offset = i;
	i = 0;
	while (i <= end)
	{
		BCD[i] = BCD[i + offset];
		i++;
	}
	BCD[i] = '\0';
}
//-----------------------------//
