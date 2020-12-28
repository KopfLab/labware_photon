#include "application.h"
#include "LoggerDisplay.h"

void LoggerDisplay::debug() {
	debug_display = true;
}

void LoggerDisplay::init()
{

	// checking that device available at the supposed address
	if (!exists) Serial.println("INFO: logger does not use an LCD screen.");
	else if (checkAddress()) present = true;
	else Serial.println("WARNING: no I2C LCD fouund, LCD functionality disabled.");

	if (present)
	{
		// create the lcd object and initialize it
		lcd = new LiquidCrystal_I2C(lcd_addr, cols, lines);
		lcd->init();
		//LiquidCrystal_I2C::init();
		//backlight();
		//clear();
		lcd->backlight();
		lcd->clear();
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

bool LoggerDisplay::checkAddress()
{
	// checking that device available at the supposed address
	Wire.begin();
	bool success = 0;
	for (int i=0; i < sizeof(i2c_addrs)/sizeof(i2c_addrs[0]); i++) {
		Serial.printf("INFO: checking for lcd at I2C address 0x%02X... ", i2c_addrs[i]);
		Wire.beginTransmission(i2c_addrs[i]);
		success = (Wire.endTransmission() == 0);
		if (success) {
			Serial.println("found -> use.");
			lcd_addr = i2c_addrs[i];
			break;
		} else {
			Serial.println("NOT found.");
		}
	}
	return(success);
}

bool LoggerDisplay::checkPresent()
{
	if (exists && !present)
	{
		Serial.println("ERROR: lcd not present, restart device with LCD properly connected.");
	}
	return (exists && present);
}

void LoggerDisplay::setTempTextShowTime(uint8_t show_time)
{
	temp_text_show_time = show_time * 1000L;
	Serial.printf("INFO: setting LCD temporary text timer to %d seconds (%d ms)\n", show_time, temp_text_show_time);
}

void LoggerDisplay::moveToPos(uint8_t line, uint8_t col)
{
	if (checkPresent() && (line_now != line || col_now != col))
	{
		line = (col > cols) ? line + 1 : line; // jump to next line if cols overflow
		col = (col > cols) ? col - cols : col; // jump to next line if cols overflow
		line = (line > lines) ? 1 : line;	  // start at beginning of screen if lines overflow
		line_now = line;
		col_now = col;
		lcd->setCursor(col - 1L, line - 1L);
	}
}

uint16_t LoggerDisplay::getPos(uint8_t line, uint8_t col)
{
	return ((line - 1) * cols + col - 1);
}
uint16_t LoggerDisplay::getPos()
{
	return (getPos(line_now, col_now));
}

void LoggerDisplay::goToLine(uint8_t line)
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

void LoggerDisplay::print(const char c[], bool temp)
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

				if (debug_display) {
					Serial.printf(
						" - part '%s' (%d to %d, length %d)  sent to lcd on line %d, col %d\n",
						update, needs_update + 1, i, length, line_now, col_init + needs_update);
				}

				// update lcd
				moveToPos(line_now, col_init + needs_update);
				lcd->print(update);
				//LiquidCrystal_I2C::print(update);

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
			if (debug_display) {
				Serial.printf(" - flagging positions %d to %d as TEMPORARY\n", pos_now, pos_now + length - 1);
			}

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

		if (debug_display) {
			Serial.printf(" - finished (new cursor location = line %d, col %d), text buffer:\n[1]%s[%d]\n", line_now, col_now, buffer, strlen(buffer));
		}
	}
}

void LoggerDisplay::printLine(uint8_t line, const char text[], uint8_t start, uint8_t end, uint8_t align, bool temp)
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

		if (debug_display) {
			if (align == LCD_ALIGN_LEFT)
				Serial.printf("Info @ %Lu: printing%s '%s' LEFT on line %u (%u to %u)\n",
							millis(), (temp ? " TEMPORARY" : ""), full_text, line, start, end);
			else if (align == LCD_ALIGN_RIGHT)
				Serial.printf("Info @ %Lu: printing%s '%s' RIGHT on line %u (%u to %u)\n",
							millis(), (temp ? " TEMPORARY" : ""), full_text, line, start, end);
		}

		// send to print
		print(full_text, temp);
	}
}

void LoggerDisplay::printLine(uint8_t line, const char text[], uint8_t length, uint8_t start)
{
	printLine(line, text, start, start + (length == LCD_LINE_LENGTH ? cols : length) - 1, LCD_ALIGN_LEFT, false);
}

void LoggerDisplay::printLineRight(uint8_t line, const char text[], uint8_t length, uint8_t end)
{
	end = (end == LCD_LINE_END || end > cols) ? cols : end;
	uint8_t start = length == LCD_LINE_LENGTH ? 1 : (length <= end ? end - length + 1 : 1);
	printLine(line, text, start, end, LCD_ALIGN_RIGHT, false);
}

void LoggerDisplay::printLineTemp(uint8_t line, const char text[], uint8_t length, uint8_t start)
{
	printLine(line, text, start, start + (length == LCD_LINE_LENGTH ? cols : length) - 1, LCD_ALIGN_LEFT, true);
}

void LoggerDisplay::printLineTempRight(uint8_t line, const char text[], uint8_t length, uint8_t end)
{
	end = (end == LCD_LINE_END || end > cols) ? cols : end;
	uint8_t start = length == LCD_LINE_LENGTH ? 1 : (length <= end ? end - length + 1 : 1);
	printLine(line, text, start, end, LCD_ALIGN_RIGHT, true);
}

void LoggerDisplay::clearLine(uint8_t line, uint8_t start, uint8_t end)
{
	printLine(line, "", start, end);
}

void LoggerDisplay::clearScreen(uint8_t start_line)
{
	if (checkPresent())
	{
		for (uint8_t i = start_line; i <= lines; i++)
		{
			clearLine(i);
		}
	}
}

void LoggerDisplay::clearTempText()
{

	// buffers
	char revert[cols];
	int needs_revert = -1;
	uint16_t pos, i;

	if (debug_display) {
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
	}

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

				if (debug_display) {
					Serial.printf(" - reverting line %d, col %d to %d to '%s'\n",
								line, col - (pos - needs_revert), col - 1, revert);
				}

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

void LoggerDisplay::update()
{
	if (present && temp_text && (millis() - temp_text_show_start) > temp_text_show_time)
	{
		clearTempText();
	}
}
