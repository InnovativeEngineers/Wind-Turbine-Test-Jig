// Libraries
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>

// Debug Flag
#define DEBUG

// Pins
const byte MASTER_START_PIN = 7;
const byte SD_CHIPSELECT_PIN = 10;

// Configs
const int MENU_DELAY = 2000; // delay when cycling through menus
const int TEST_DELAY = 1000; // delay between test samples
const int HANG_DELAY = 1000; // delay before calling hung transmission
const int SERIAL_DELAY = 5; // delay for serial transmission
const int UPDATE_DELAY = 1000; // delay for updating display
const int BUTTON_DELAY = 100; // delay for buttons within while loops

// SD
File DATAFILE;
String FILENAME;
bool FILE_CREATED = false;
char ALPHANUMERIC[37] = "_abcdefghijklmnopqrstuvwxyz0123456789";

// LCD
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
byte BUTTONS = 0;

// Serial Communication
const char WIND = 'A';
const char FORCE = 'L';
const char RPM = 'H';
const char DATA = 'D';
const char UP = 'u';
const char DOWN = 'd';
const char LEFT = 'l';
const char RIGHT = 'r';
const char CAL = 'C';

// Setup Function
void setup() {
  
  Serial.begin(9600);
  while(!Serial);

  init_lcd();
}

// Init Functions
void init_lcd(void){
  lcd.begin(16,2);
  lcd.setBacklight(0x7);
  lcd.clear();
  lcd.setCursor(0,0);
}

// Main Loop
void loop(){

  lcd_display("MAIN MENU", "S: START TEST");
  lcd_display("U: LC, D: AN", "L: HS, R: SD");

  while(1) {

    delay(BUTTON_DELAY);
    
    BUTTONS = lcd.readButtons();
    if (BUTTONS & BUTTON_UP) {load_cell(); break;}
    else if (BUTTONS & BUTTON_DOWN) {anemometer(); break;}
    else if (BUTTONS & BUTTON_LEFT) {hall_sensor(); break;}
    else if (BUTTONS & BUTTON_RIGHT) {sd_card(); break;}
    else if (BUTTONS & BUTTON_SELECT) {run_test(); break;}

    if(digitalRead(MASTER_START_PIN)){run_test(); break;}
  }
}

// Load Cell Menu
void load_cell(){

  #ifndef DEBUG
  lcd_display("LOAD CELL", "FORCE: ");
  #endif

  #ifdef DEBUG
  lcd.clear();
  lcd.setCursor(0,0);
  #endif

  unsigned long last_update = millis();

  while(1) {

    delay(BUTTON_DELAY);

    if(millis()-last_update > UPDATE_DELAY){
      
      #ifndef DEBUG
      lcd.setCursor(7,1);
      lcd.print(get_data(FORCE));
      lcd.print(" g  ");
      #endif
      
      #ifdef DEBUG
      last_update = millis();
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(get_data(FORCE));
      lcd.setCursor(0,1);
      lcd.print(get_data(CAL));
      #endif

      last_update = millis();
    }
    
    
    BUTTONS = lcd.readButtons();
    if(BUTTONS & BUTTON_UP) {Serial.print(UP);}
    else if(BUTTONS & BUTTON_DOWN) {Serial.print(DOWN);}
    else if(BUTTONS & BUTTON_LEFT) {Serial.print(LEFT);}
    else if(BUTTONS & BUTTON_RIGHT) {Serial.print(RIGHT);}
    else if(BUTTONS & BUTTON_SELECT) {return;}
  }
}

// Anemometer Menu
void anemometer() {

  #ifndef DEBUG
  lcd_display("ANEMOMETER", "SPEED: ");
  #endif

  #ifdef DEBUG
  lcd.clear();
  lcd.setCursor(0,0);
  #endif

  unsigned long last_update = millis();
    
  while(1) {
    
    delay(BUTTON_DELAY);

    if(millis()-last_update > UPDATE_DELAY){
      
      #ifndef DEBUG
      lcd.setCursor(7,1);
      lcd.print(get_data(WIND));
      lcd.print(" m/s ");
      #endif

      #ifdef DEBUG
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(get_data(WIND));
      #endif

      last_update = millis();
    }
    
    BUTTONS = lcd.readButtons();
    if(BUTTONS & BUTTON_UP) {return;}
    else if(BUTTONS & BUTTON_DOWN) {return;}
    else if(BUTTONS & BUTTON_LEFT) {return;}
    else if(BUTTONS & BUTTON_RIGHT) {return;}
    else if(BUTTONS & BUTTON_SELECT) {return;}
  }
}

// Hall Sensor Menu
void hall_sensor() {

  #ifndef DEBUG
  lcd_display("HALL SENSOR", "RPM: ");
  #endif

  #ifdef DEBUG
  lcd.clear();
  lcd.setCursor(0,0);
  #endif

  unsigned long last_update = millis();

  while(1) {
    
    delay(BUTTON_DELAY);

    if(millis()-last_update > UPDATE_DELAY){
      #ifndef DEBUG
      lcd.setCursor(5,1);
      lcd.print(get_data(RPM));
      lcd.print("     ");
      #endif
  
      #ifdef DEBUG
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(get_data(RPM));
      #endif

      last_update = millis();
    }
    
    BUTTONS = lcd.readButtons();
    if(BUTTONS & BUTTON_UP) {return;}
    else if(BUTTONS & BUTTON_DOWN) {return;}
    else if(BUTTONS & BUTTON_LEFT) {return;}
    else if(BUTTONS & BUTTON_RIGHT) {return;}
    else if(BUTTONS & BUTTON_SELECT) {return;}
  }
}

// SD Card Menu
void sd_card() {

  lcd_display("SD CARD", "PIN: "+String(SD_CHIPSELECT_PIN));

  if(!SD.begin(SD_CHIPSELECT_PIN)){
    lcd_display("ERROR OPENING SD","");
    return;
  }

  if(FILE_CREATED){
    lcd_display("CREATED: ", FILENAME);
    return;
  }

  lcd_display("SD CARD FOUND!", "");
  
  lcd_display("LEADING \"_\"", "WILL BE DELETED");

  FILENAME = find_open_filename();

  lcd_display("FILENAME: ", FILENAME);

  byte cursor_index = 0;
  byte alphanumeric_value[8]; // test_001 (default)
  alphanumeric_value[0] = 21; // t
  alphanumeric_value[1] = 6;  // e
  alphanumeric_value[2] = 20; // s
  alphanumeric_value[3] = 21; // t
  alphanumeric_value[4] = 0;  // _
  alphanumeric_value[5] = int(27+FILENAME.charAt(5)); // 0 (default)
  alphanumeric_value[6] = int(27+FILENAME.charAt(6)); // 0 (default)
  alphanumeric_value[7] = int(27+FILENAME.charAt(7)); // 1 (default)
  lcd_cursor(cursor_index, 1);

  while(1) {
    
    BUTTONS = lcd.readButtons();

    if (BUTTONS & BUTTON_DOWN) {
      BUTTONS = 0;
      alphanumeric_value[cursor_index] = (alphanumeric_value[cursor_index]+36)%37;
      FILENAME.setCharAt(cursor_index, ALPHANUMERIC[alphanumeric_value[cursor_index]]);
      update_char(cursor_index, 1, FILENAME.charAt(cursor_index));
      lcd_cursor(cursor_index, 1);
      delay(BUTTON_DELAY);
    }
    else if (BUTTONS & BUTTON_UP) {
      BUTTONS = 0;
      alphanumeric_value[cursor_index] = (alphanumeric_value[cursor_index]+1)%37;
      FILENAME.setCharAt(cursor_index, ALPHANUMERIC[alphanumeric_value[cursor_index]]);
      update_char(cursor_index, 1, FILENAME.charAt(cursor_index));
      lcd_cursor(cursor_index, 1);
      delay(BUTTON_DELAY);
    }
    else if (BUTTONS & BUTTON_LEFT) {
      BUTTONS = 0;
      cursor_index = (cursor_index+7)&7; // keeps index between 0-7
      lcd_cursor(cursor_index, 1);
      delay(BUTTON_DELAY);
    }
    else if (BUTTONS & BUTTON_RIGHT) {
      BUTTONS = 0;
      cursor_index = (cursor_index+1)&7; // keeps index between 0-7
      lcd_cursor(cursor_index, 1);
      delay(BUTTON_DELAY);
    }
    else if (BUTTONS & BUTTON_SELECT) {
      while(FILENAME.charAt(0) == '_') {
        FILENAME.remove(0,1);
      }
      if(SD.exists(FILENAME)){
        lcd_display("ERROR: FILE", "ALREADY EXISTS");     
        lcd.noCursor();
        return;
      }
      else {
        DATAFILE = SD.open(FILENAME, FILE_WRITE);    
        if (DATAFILE){
          FILE_CREATED = true;
          lcd_display(FILENAME, "CREATED!");       
          lcd.noCursor();
          return;
        }
        else {
          FILE_CREATED = false;     
          lcd_display("ERROR: FILE", "NOT CREATED"); 
          lcd.noCursor();
          return;
        }
      }
    }
  } 
}

// Function to find open filename on SD Card
String find_open_filename() {
  String filename = "test_001.txt";
  unsigned int file_number = 1;
  while(SD.exists(filename)) {
    file_number++;
    if(file_number == 1000){
      return "_______.txt"; 
    }
    filename.setCharAt(5, '0'+(file_number/100) % 10);
    filename.setCharAt(6, '0'+(file_number/10) % 10);
    filename.setCharAt(7, '0'+(file_number) % 10);
  }
  return filename;
}

// Test Function
void run_test() {
  
  if(!FILE_CREATED){

    if(!SD.begin(SD_CHIPSELECT_PIN)){
      lcd_display("ERROR OPENING SD","");
      return;
    }
    else{
      lcd_display("SD CARD FOUND!","FINDING FILE...");
    }
    
    FILENAME = find_open_filename();
    if(SD.exists(FILENAME)){
      randomSeed(analogRead(0));
      long rand_num = random(1000);
      FILENAME.setCharAt(5, (rand_num/100)%10);
      FILENAME.setCharAt(6, (rand_num/10)%10);
      FILENAME.setCharAt(7, (rand_num)%10);
    }
    lcd_display("CREATING FILE: ", FILENAME);    
    DATAFILE = SD.open(FILENAME, FILE_WRITE);
        
    if (DATAFILE){
      FILE_CREATED = true;
      lcd_display("CREATED: ", FILENAME);
    }
    else{
      FILE_CREATED = false;
      lcd_display("ERROR: FILE", "NOT CREATED");
      return;
    }
  }
  
  #ifndef DEBUG
  lcd_display("SPEED:","F:        R: ");
  #endif

  DATAFILE.print("Initial Force: ");
  DATAFILE.println(get_data(FORCE));
  
  while(1) {
 
    delay(TEST_DELAY);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("W: ");
    lcd.print(get_data(WIND));
    lcd.setCursor(8,0);
    lcd.print("R: ");
    lcd.print(get_data(RPM));
    lcd.setCursor(0,1);
    lcd.print("F: ");
    lcd.print(get_data(FORCE));
    DATAFILE.print(get_data(WIND));
    DATAFILE.print(", ");
    DATAFILE.print(get_data(FORCE));
    DATAFILE.print(", ");
    DATAFILE.print(get_data(RPM));
    DATAFILE.print(", ");
    DATAFILE.println(millis());
    
    BUTTONS = lcd.readButtons();
    if(BUTTONS & BUTTON_UP) {break;}
    else if(BUTTONS & BUTTON_DOWN) {break;}
    else if(BUTTONS & BUTTON_LEFT) {break;}
    else if(BUTTONS & BUTTON_RIGHT) {break;}
    else if(BUTTONS & BUTTON_SELECT) {break;}

    if(digitalRead(MASTER_START_PIN)){break;}
  }
  DATAFILE.close();
  return;
}

// Prints line_1 and line_2 to LCD
void lcd_display(String line_1, String line_2){
  BUTTONS = 0;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(line_1);
  lcd.setCursor(0,1);
  lcd.print(line_2);
  delay(MENU_DELAY);
}

// Moves and turns on cursor at (x,y)
void lcd_cursor(int x, int y){
  lcd.setCursor(x,y);
  lcd.cursor();
}

// Update single char at (x,y) on LCD
void update_char(int x, int y, char c){
  lcd.setCursor(x, y);
  lcd.print(c);
}

// Sends char to other Arduino, returns response
String get_data(char cmd){
  String str = "";
  Serial.print(cmd);
  unsigned long start_time = millis();
  while(!Serial.available()){
    if(millis()-start_time > HANG_DELAY){
      return "HANG";
    }
  }
  delay(SERIAL_DELAY);
  while(Serial.available()) {
    str += char(Serial.read());
  }
  return str;
}
