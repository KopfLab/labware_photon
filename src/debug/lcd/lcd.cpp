// TODO: implement address check based on how i2c_scanner checks for valid addresses to avoid hanging photon if display is missing.

/*
 * Wiring from LCD backpack to photon:
 *   - SDA to pin D0
 *   - SCL to pin D1
 *   - VCC to VIN
 *   - GND to GND
 *
 */

#include <LiquidCrystal_I2C_Spark.h> // requirement: https://github.com/BulldogLowell/LiquidCrystal_I2C_Spark

// alignments
#define LCD_ALIGN_LEFT 1
#define LCD_ALIGN_RIGHT 2
//#define LCD_ALIGN_CENTER 3 // not yet implemented

// line parameters
#define LCD_LINE_END 0	// code for line end
#define LCD_LINE_LENGTH 0 // code for full length of the text (if enough space)

// buffers
#define LCD_MAX_SIZE 80 // maximum number of characters on LCD

// debug mode (comment in to get Serial debug messages)
// #define LCD_DEBUG_ON

// Display class handles displaying information
// for the peristaltic pumping system
class DeviceDisplay : public LiquidCrystal_I2C
{
private:
	// i2c address and presence
	const uint8_t lcd_addr;
	bool present = false;

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
	// standard constructor
	DeviceDisplay(uint8_t lcd_Addr, uint8_t lcd_cols, uint8_t lcd_lines) : // addr, en,rw,rs,d4,d5,d6,d7,bl,blpol
																		   LiquidCrystal_I2C(lcd_Addr, lcd_cols, lcd_lines), lcd_addr(lcd_Addr), cols(lcd_cols), lines(lcd_lines)
	{
		if (cols * lines > LCD_MAX_SIZE)
		{
			Serial.println("ERROR: LCD size larger than text buffers, you must adjust LCD_MAX_SIZE or prepare for unexpected behaviour!!!");
		}
	}

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

void DeviceDisplay::init()
{

	// checking that device available at the supposed address
	if (checkAddress()) present = true;
	else Serial.println("WARNiNG: LCD functionality disabled.");

	if (present)
	{
		LiquidCrystal_I2C::init();
		backlight();
		clear();
		for (int i = 0; i < cols * lines; i++)
		{
			temp_pos[i] = false;
			buffer[i] = ' ';
			memory[i] = ' ';
		}
		buffer[cols * lines] = 0;
		memory[cols * lines] = 0;
		moveToPos(1, 1);
	}
}

bool DeviceDisplay::checkAddress()
{
	// checking that device available at the supposed address
	Wire.begin();
	Serial.print("Info: checking for I2C device at address 0x");
	if (lcd_addr < 16)
		Serial.print("0");
	Serial.print(lcd_addr, HEX);
	Serial.print("...");
	Wire.beginTransmission(lcd_addr);
	bool success = (Wire.endTransmission() == 0);
	if (success) Serial.println("success.");
	else Serial.println("FAILED!");
	return(success);
}

bool DeviceDisplay::checkPresent()
{
	if (!present)
	{
		Serial.println("ERROR: lcd not present, restart device with LCD properly connected.");
	}
	return (present);
}

void DeviceDisplay::setTempTextShowTime(uint8_t show_time)
{
	temp_text_show_time = show_time * 1000L;
	Serial.printf("Info: setting LCD temporary text timer to %d seconds (%d ms)\n", show_time, temp_text_show_time);
}

void DeviceDisplay::moveToPos(uint8_t line, uint8_t col)
{
	if (checkPresent() && (line_now != line || col_now != col))
	{
		line = (col > cols) ? line + 1 : line; // jump to next line if cols overflow
		col = (col > cols) ? col - cols : col; // jump to next line if cols overflow
		line = (line > lines) ? 1 : line;	  // start at beginning of screen if lines overflow
		line_now = line;
		col_now = col;
		setCursor(col - 1L, line - 1L);
	}
}

uint16_t DeviceDisplay::getPos(uint8_t line, uint8_t col)
{
	return ((line - 1) * cols + col - 1);
}
uint16_t DeviceDisplay::getPos()
{
	return (getPos(line_now, col_now));
}

void DeviceDisplay::goToLine(uint8_t line)
{
	if (checkPresent())
	{
		if (line > lines)
		{
			Serial.printf("ERROR: requested move to line %d. Display only has %d lines.\n", line, lines);
		}
		else
		{
			moveToPos(line, 1);
		}
	}
}

void DeviceDisplay::print(const char c[], bool temp)
{

	if (checkPresent())
	{

		// determine text length to maximally fill the line
		uint8_t length = (strlen(c) > (cols - col_now + 1)) ? cols - col_now + 1 : strlen(c);

		// position information
		uint8_t col_init = col_now;
		uint16_t pos_now = getPos();

		// update actual LCD text (but only parts that are necessary, to avoid slow i2c communication)
		char update[length + 1];
		int needs_update = -1;
		for (uint8_t i = 0; i <= length; i++)
		{
			if (needs_update > -1 && (i == length || buffer[pos_now + i] == c[i]))
			{
				// either at the end OR buffer the same as new text but prior text has needs_update flag on -> write text
				strncpy(update, c + needs_update, i - needs_update);
				update[i - needs_update] = 0; // make sure it's 0-pointer terminated

#ifdef LCD_DEBUG_ON
				Serial.printf(" - part '%s' (%d to %d, length %d)  sent to lcd on line %d, col %d\n",
							  update, needs_update + 1, i, length, line_now, col_init + needs_update);
#endif

				// update lcd
				moveToPos(line_now, col_init + needs_update);
				LiquidCrystal_I2C::print(update);

				// store new text in buffer
				strncpy(buffer + pos_now + needs_update, update, i - needs_update);

				needs_update = -1; // reset
			}
			else if (needs_update == -1 && i < length && (temp || !temp_pos[pos_now + i]) && buffer[pos_now + i] != c[i])
			{
				// either a new temp or NOT overwriting a temp position + buffer not the same as new text (and not at end yet)
				needs_update = i; // mark beginning of update
			}
		}

		// update final position
		moveToPos(line_now, col_init + length);

		// update memory information
		if (temp)
		{
			// temporary message --> start counter and store memory info
			temp_text = true;
			temp_text_show_start = millis();
#ifdef LCD_DEBUG_ON
			Serial.printf(" - flagging positions %d to %d as TEMPORARY\n", pos_now, pos_now + length - 1);
#endif
			for (uint8_t i = pos_now; i < pos_now + length; i++)
			{
				temp_pos[i] = true;
			}
		}
		else
		{
			// non temporary message --> store in memory (don't include null pointer)
			strncpy(memory + pos_now, c, length);
		}

#ifdef LCD_DEBUG_ON
		Serial.printf(" - finished (new cursor location = line %d, col %d), text buffer:\n[1]%s[%d]\n", line_now, col_now, buffer, strlen(buffer));
#endif
	}
}

void DeviceDisplay::printLine(uint8_t line, const char text[], uint8_t start, uint8_t end, uint8_t align, bool temp)
{

	if (checkPresent())
	{

		// safety check
		if (line > lines)
		{
			Serial.printf("ERROR: requested print on line %d. Display only has %d lines.\n", line, lines);
			return;
		}

		// move to correct position
		moveToPos(line, start);

		// ensure legitemate start and end points
		end = (end == LCD_LINE_END || end > cols) ? cols : end;
		start = (start > end) ? end : start; // essentially leads to NO print
		uint8_t length = end - start + 1;

		// assemble print text (pad the start/end according to align)
		char full_text[length + 1] = "";
		uint8_t space_start, space_end = 0;
		if (align == LCD_ALIGN_LEFT)
		{
			space_start = strlen(text);
			space_end = length;
			strncpy(full_text, text, length);
		}
		else if (align == LCD_ALIGN_RIGHT)
		{
			space_start = 0;
			space_end = (strlen(text) < length) ? length - strlen(text) : 0;
			strncpy(full_text + space_end, text, length - space_end);
		}
		else
		{
			Serial.println("ERROR: unsupported alignment");
		}

		// spaces
		for (int i = space_start; i < space_end; i++)
		{
			full_text[i] = ' ';
		}

		// make sure 0 pointer at the end
		full_text[length] = 0;

#ifdef LCD_DEBUG_ON
		if (align == LCD_ALIGN_LEFT)
			Serial.printf("Info @ %Lu: printing%s '%s' LEFT on line %u (%u to %u)\n",
						  millis(), (temp ? " TEMPORARY" : ""), full_text, line, start, end);
		else if (align == LCD_ALIGN_RIGHT)
			Serial.printf("Info @ %Lu: printing%s '%s' RIGHT on line %u (%u to %u)\n",
						  millis(), (temp ? " TEMPORARY" : ""), full_text, line, start, end);
#endif

		// send to print
		print(full_text, temp);
	}
}

void DeviceDisplay::printLine(uint8_t line, const char text[], uint8_t length, uint8_t start)
{
	printLine(line, text, start, start + (length == LCD_LINE_LENGTH ? cols : length) - 1, LCD_ALIGN_LEFT, false);
}

void DeviceDisplay::printLineRight(uint8_t line, const char text[], uint8_t length, uint8_t end)
{
	end = (end == LCD_LINE_END || end > cols) ? cols : end;
	uint8_t start = length == LCD_LINE_LENGTH ? 1 : (length <= end ? end - length + 1 : 1);
	printLine(line, text, start, end, LCD_ALIGN_RIGHT, false);
}

void DeviceDisplay::printLineTemp(uint8_t line, const char text[], uint8_t length, uint8_t start)
{
	printLine(line, text, start, start + (length == LCD_LINE_LENGTH ? cols : length) - 1, LCD_ALIGN_LEFT, true);
}

void DeviceDisplay::printLineTempRight(uint8_t line, const char text[], uint8_t length, uint8_t end)
{
	end = (end == LCD_LINE_END || end > cols) ? cols : end;
	uint8_t start = length == LCD_LINE_LENGTH ? 1 : (length <= end ? end - length + 1 : 1);
	printLine(line, text, start, end, LCD_ALIGN_RIGHT, true);
}

void DeviceDisplay::clearLine(uint8_t line, uint8_t start, uint8_t end)
{
	printLine(line, "", start, end);
}

void DeviceDisplay::clearScreen(uint8_t start_line)
{
	if (checkPresent())
	{
		for (uint8_t i = start_line; i <= lines; i++)
		{
			clearLine(i);
		}
	}
}

void DeviceDisplay::clearTempText()
{

	// buffers
	char revert[cols];
	int needs_revert = -1;
	uint16_t pos, i;

#ifdef LCD_DEBUG_ON
	Serial.printf("Info @ %Lu: clearing temp messages...\n", millis());
	for (uint8_t line = 1; line <= lines; line++)
	{
		for (uint8_t col = 1; col <= cols; col++)
		{
			pos = getPos(line, col);
			if (col == 1)
				Serial.printf("[%2d]", pos + 1);
			Serial.printf("%s", (temp_pos[pos]) ? "T" : "F");
			if (col == cols)
				Serial.printf("[%2d]\n", pos + 1);
		}
	}
#endif

	// find temp text on each row
	for (uint8_t line = 1; line <= lines; line++)
	{
		for (uint8_t col = 1; col <= cols + 1; col++)
		{
			pos = getPos(line, col);
			if (needs_revert > -1 && (col > cols || !temp_pos[pos]))
			{
				// either at end of a temp section or end of the line with temp text to revert
				strncpy(revert, memory + needs_revert, pos - needs_revert);
				revert[pos - needs_revert] = 0; // make sure it's 0-pointer terminated

#ifdef LCD_DEBUG_ON
				Serial.printf(" - reverting line %d, col %d to %d to '%s'\n",
							  line, col - (pos - needs_revert), col - 1, revert);
#endif

				// reset affected temp text
				for (i = needs_revert; i < pos; i++)
					temp_pos[i] = false;
				moveToPos(line, col - (pos - needs_revert));
				print(revert, false);
				needs_revert = -1;
			}
			else if (needs_revert == -1 && col <= cols && temp_pos[pos])
			{
				// found the beginning of a temp section
				needs_revert = pos; // mark beginning of revert
			}
		}
	}

	// flag temp text as false and reset all temp fields
	temp_text = false;
}

void DeviceDisplay::update()
{
	if (temp_text && (millis() - temp_text_show_start) > temp_text_show_time)
	{
		clearTempText();
	}
}

// default instances and LCD finder
DeviceDisplay LCD_16x2(0x3f, 16, 2);
DeviceDisplay LCD_20x4(0x27, 20, 4);
const int n_LCDs = 2;
DeviceDisplay *LCDs[] = { &LCD_16x2, &LCD_20x4 };
DeviceDisplay* find_LCD() {
	Serial.println("Info: searching through default LCD configurations...");
	bool success = false;
	for (int i = 0; i < n_LCDs; i++) {
		success = LCDs[i]->checkAddress();
		if (success) {
			Serial.printf("Info: found LCD present for default LCD configuration %d, proceeding with this one...\n", i + 1);
			return(LCDs[i]);
		}
	}
	// no suitable LCD found, return the first one in the array
	Serial.println("WARNING: no LCD in the default configurations seems to be present, reverting to first default.");
	return(LCDs[0]);
}


// ********************** //
// implementation example //
// ********************** //

DeviceDisplay *lcd = 0;

int last_second = 0;
int last_message = 0;

char device_name[20];
char device_public_ip[20];
byte macAddress[6];

#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)
unsigned long last_sync = millis();

void info_handler(const char *topic, const char *data)
{
	if (strcmp(topic, "spark/device/name") == 0)
	{
		Serial.println("Device name: " + String(data));
		strncpy(device_name, data, sizeof(device_name));
		lcd->printLine(1, "Name: " + String(device_name));
	}
	else if (strcmp(topic, "spark/device/ip") == 0)
	{
		Serial.println("Device public IP: " + String(data));
		strncpy(device_public_ip, data, sizeof(device_public_ip));
		lcd->printLine(3, "IP address: ");
		lcd->printLine(4, device_public_ip);
	}
	// unsubscribe from events
	if (strlen(device_name) > 0 && strlen(device_public_ip) > 0)
	{
		Particle.unsubscribe();
	}
}

void setup(void)
{
	Serial.begin(9600);
	Time.zone(-6); // to get the time correctly
	Serial.println("Starting up...");

	// LCD screen
	lcd = find_LCD();
	lcd->init();
	lcd->setTempTextShowTime(3); // 3 seconds temporary text show time
	lcd->printLine(1, "Starting up...");
	lcd->printLine(2, "extra long super testing");

	// device info
	Serial.println("Requesting device info...");
	Particle.subscribe("spark/", info_handler);
	Particle.publish("spark/device/name");
	Particle.publish("spark/device/ip");
	Serial.println("Startup complete");
}

int test = 0;

void loop(void)
{

	// Time sync (once a day)
	if (millis() - last_sync > ONE_DAY_MILLIS)
	{
		// Request time synchronization from the Particle Cloud
		Particle.syncTime();
		last_sync = millis();
	}

	// Time update
	if (Time.second() != last_second)
	{
		Serial.println(Time.format(Time.now(), "%H:%M:%S %d.%m.%Y %Z"));
		lcd->printLine(2, Time.format(Time.now(), "%H:%M:%S %d.%m.%Y %Z"));
		last_second = Time.second();
	}

	// Other test messages
	if (millis() - last_message > 5000)
	{

		if (test == 0)
		{
			lcd->printLineTemp(1, "line 1 temp msg extra long");
		}
		else if (test == 1)
		{
			lcd->printLineTemp(2, "full left temp");
		}
		else if (test == 2)
		{
			lcd->printLineTempRight(2, "right temp", 11);
		}
		else if (test == 3)
		{
			lcd->printLineTempRight(1, "2 line temp", 12);
			lcd->printLineTempRight(2, "2 line temp", 12);
		}
		else if (test == 4)
		{
			lcd->printLineRight(1, "right perm", 11);
		}
		else if (test == 5)
		{
			lcd->printLineTempRight(1, "TEMP", 4);
		}
		else if (test == 6)
		{
			lcd->printLine(1, " add part left ", 10, 4);
		}
		else if (test == 7)
		{
			lcd->goToLine(1);
			lcd->print("1 ");
			lcd->print("2 ");
			lcd->print("3...");
		}
		else if (test == 8)
		{
			lcd->printLine(1, "Name: " + String(device_name));
		}

		test++;
		last_message = millis();
	}

	lcd->update();
}
