#pragma once
#include <LiquidCrystal_I2C_Spark.h> // requirement: https://github.com/BulldogLowell/LiquidCrystal_I2C_Spark

// alignments
#define LCD_ALIGN_LEFT   1
#define LCD_ALIGN_RIGHT  2
//#define LCD_ALIGN_CENTER 3 // not yet implemented

// line parameters
#define LCD_LINE_END		 0 // code for line end
#define LCD_LINE_LENGTH  0 // code for full length of the text (if enough space)

// buffers
#define LCD_MAX_SIZE     80 // maximum number of characters on LCD

// Display class handles displaying information
class LoggerDisplay
{
private:

	// debug flag
	bool debug_display = false;

	// i2c addresses typically used for LCDs
	const uint8_t i2c_addrs[3] = {0x3f, 0x27, 0x23};
	uint8_t lcd_addr;

	// lcd exists?
	bool exists = true;

	// lcd actually present at one of the i2c addresses?
	bool present = false;

	// display object
	LiquidCrystal_I2C* lcd;

	// display layout
	const uint8_t cols, lines;

	// display data
	uint8_t col_now, line_now;		 // current print position on the display
	char buffer[LCD_MAX_SIZE + 1];   // the current text of the lcd display
	char memory[LCD_MAX_SIZE + 1];   // the memory text of the lcd display for non temporay messages
	bool temp_pos[LCD_MAX_SIZE + 1]; // which text is only temporary

	// temporary message parameters
	bool temp_text = false;					// whether there is any temporary text
	uint16_t temp_text_show_time = 3000;	// how long current temp text is being shown for (in ms)
	unsigned long temp_text_show_start = 0; // when the last temp text was started (changes reset the start time for all temp text!)

	// keep track of position / navigation
	void moveToPos(uint8_t line, uint8_t col);
	uint16_t getPos();
	uint16_t getPos(uint8_t line, uint8_t col);

public:

	// empty constructor (no screen)
	LoggerDisplay() : LoggerDisplay(0, 0) {
		exists = false;
	}

	// standard constructor
	LoggerDisplay(uint8_t lcd_cols, uint8_t lcd_lines) : cols(lcd_cols), lines(lcd_lines)
	{
		if (cols * lines > LCD_MAX_SIZE)
		{
			Serial.println("ERROR: LCD size larger than text buffers, you must adjust LCD_MAX_SIZE or prepare for unexpected behaviour!!!");
		}
	}

	// turn debug on
	void debug();

	// initialize the display
	void init();

	// check for valid i2c address
	bool checkAddress();

	// check presence
	bool checkPresent();

	// set temporary text show time (in seconds)
	void setTempTextShowTime(uint8_t show_time);

	// clears the line (overwrites spaces)
	void clearLine(uint8_t line, uint8_t start = 1, uint8_t end = LCD_LINE_END);

	// move to a specific line (e.g. before adding individual text with print)
	void goToLine(uint8_t line);

	// print normal text (temp text that is still visible is only overwritten with new temp text)
	void print(const char c[], bool temp = false);

	// print a whole line (shortens text if too long, pads with spaces if too short)
	void printLine(uint8_t line, const char text[], uint8_t start, uint8_t end, uint8_t align, bool temp);

	// simpler version of printLine with useful defaults (left aligned, start at first character, print whole line)
	void printLine(uint8_t line, const char text[], uint8_t length = LCD_LINE_LENGTH, uint8_t start = 1L);

	// same as the simpler version of printLine but right aligned
	void printLineRight(uint8_t line, const char text[], uint8_t length = LCD_LINE_LENGTH, uint8_t end = LCD_LINE_END);

	// same as the simpler version of printLine but only temporary text
	void printLineTemp(uint8_t line, const char text[], uint8_t length = LCD_LINE_LENGTH, uint8_t start = 1L);

	// same as the simpler version of printLineRight but only tempoary text
	void printLineTempRight(uint8_t line, const char text[], uint8_t length = LCD_LINE_LENGTH, uint8_t end = LCD_LINE_END);

	// clear all temporary text
	void clearTempText();

	// clear whole screen (temp text will stay until timer is up)
	void clearScreen(uint8_t start_line = 1L);

	// call in loop to keep temporary text up to date
	void update();
};
