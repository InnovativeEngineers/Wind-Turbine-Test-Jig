/*  
 *  This Arduino is being used to read the values from our three sensors and store them locally. 
 *  
 *  When prompted by the other Arduino, the stored data will be configured and sent via Serial.
 *  
 *  The Anemometer functions as a linear voltage sensor.
 *  
 *  The Load Cell functions as a serial data sensor.
 *  
 *  The Hall Sensor is currently being used as an analog scan sensor.
 *    - However, figuring out how to used digital interrupts would work much better.
 */

// LIBRARIES //

// Load Cell
#include <HX711_ADC.h>

// DEBUG FLAG //

//#define DEBUG

// PINS //

// Anemometer: analog input, linear range, wind-speed[m/s] = 20*input[V] - 7.6
const byte ANEMOMETER_PIN = A1; 

// Load Cell Dout: digital pin used to read shifted data from Load Cell
const byte LOAD_CELL_DOUT_PIN = 4; 

// Load Cell Sck: digital serial clock pin used to shift data from Load Cell
const byte LOAD_CELL_SCK_PIN = 5;

// Hall Sensor: analog pin scanned constantly to time rpm of shaft
const byte HALL_SENSOR_PIN = A3;

// CONFIGS //

// Anemometer Buffer Size: power of 2 for easier pointer/average computation
const byte ANEMOMETER_BUFFER_SIZE = 8;

// Load Cell Buffer Size: power of 2 for easeir pointer/average computation
const byte LOAD_CELL_BUFFER_SIZE = 8;

// Hall Sensor Buffer Size: power of 2 for easier pointer/average computation
const byte HALL_SENSOR_BUFFER_SIZE = 8;

// ANEMOMETER DATA //

// Anemometer Buffer: rolling buffer to store ADC values from anemometer (int)
unsigned int ANEMOMETER_BUFFER[ANEMOMETER_BUFFER_SIZE];

// Anemometer Index: running index to store next ADC value
byte ANEMOMETER_INDEX = 0;

// LOAD CELL DATA //

// Load Cell Stabilizing Time: stabalizing time during begin
const long LOAD_CELL_STABALIZING_TIME = 2000;

// Load Cell Cal Factor: calibration factor of Load Cell
float LOAD_CELL_CAL_FACTOR = 18.75;

// Load Cell Cal Power: power of 10's place to increase/decrease
byte LOAD_CELL_CAL_POWER = 0;

// Load Cell: class istance for the Load Cell
HX711_ADC LoadCell(LOAD_CELL_DOUT_PIN, LOAD_CELL_SCK_PIN);

// Load Cell Buffer: rolling buffer to store Load Cell force (in grams?)
float LOAD_CELL_BUFFER[LOAD_CELL_BUFFER_SIZE];

// Load Cell Index: running index to store next force value
byte LOAD_CELL_INDEX = 0;

// HALL SENSOR DATA //

// Hall Sensor On: current state of the Hall Sensor
bool HALL_SENSOR_ON = false;

// Hall Sensor Last Read: time (in ms) of last Hall Sensor trigger
unsigned long HALL_SENSOR_LAST_READ = 0;

// Hall Sensor Buffer: rolling buffer to store time between triggers (in ms)
unsigned long HALL_SENSOR_BUFFER[HALL_SENSOR_BUFFER_SIZE];

// Hall Sensor Index: running index to store next time value
byte HALL_SENSOR_INDEX = 0;

// Hall Sensor Off Value: analog value to warrant OFF status of Hall Sensor
unsigned long HALL_SENSOR_OFF_VALUE = 200;

// SERIAL CONSTANTS //
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
  
  pinMode(ANEMOMETER_PIN, INPUT);
  pinMode(HALL_SENSOR_PIN, INPUT);

  init_load_cell();
  init_int_buffer(ANEMOMETER_BUFFER, ANEMOMETER_BUFFER_SIZE);
  init_float_buffer(LOAD_CELL_BUFFER, LOAD_CELL_BUFFER_SIZE);
  init_long_buffer(HALL_SENSOR_BUFFER, HALL_SENSOR_BUFFER_SIZE);

  Serial.begin(9600);
  while(!Serial);
}

// Init Functions
void init_load_cell(){
  LoadCell.begin();
  LoadCell.start(LOAD_CELL_STABALIZING_TIME);
  LoadCell.setCalFactor(LOAD_CELL_CAL_FACTOR);
  LoadCell.tare();
}
void init_int_buffer(int buff[], int buff_size){
  for(int i = 0; i < buff_size; i++){
    buff[i] = 0;
  }
}
void init_float_buffer(float buff[], int buff_size){
  for(int i = 0; i < buff_size; i++){
    buff[i] = 0.0;
  }
}
void init_long_buffer(long buff[], int buff_size){
  for(int i = 0; i < buff_size; i++){
    buff[i] = 0;
  }
}

// Main Loop
void loop(){
  
  // Update Load Cell (tell on-board ADC to read new value)
  LoadCell.update();

  // Read Anemometer voltage, store in rolling buffer.
  anemometer_read();
  
  // Scan Hall Sensor, update RPM in rolling buffer if cycle detected.
  hall_sensor_read();

  // Shift out Load Cell value, store in rolling buffer.
  load_cell_read();
}


void serialEvent() {
  
  char inChar = char(Serial.read());
  String dataString = "";
    
  switch(inChar) {
    
    case WIND: // Anemometer Value
      Serial.print(wind_speed());
      break;

    case FORCE: // Load Cell Value
      Serial.print(force());
      break;

    case LEFT: // Increase Cal Factor power
      LOAD_CELL_CAL_POWER += 1;
      break;

    case RIGHT: // Decrease Cal Factor power
      LOAD_CELL_CAL_POWER -= 1;
      break;

    case UP: // Increase Cal Factor
      LOAD_CELL_CAL_FACTOR += pow(10, LOAD_CELL_CAL_POWER);
      LoadCell.setCalFactor(LOAD_CELL_CAL_FACTOR);
      break;

    case DOWN: // Decrease Cal Factor
      LOAD_CELL_CAL_FACTOR -= pow(10, LOAD_CELL_CAL_POWER);
      LoadCell.setCalFactor(LOAD_CELL_CAL_FACTOR);
      break;

    case CAL: // Cal Factor
      Serial.print(LOAD_CELL_CAL_FACTOR);
      break;

    case RPM: // Hall Sensor Value
      Serial.print(rpm());
      break;

    case DATA: // CSV Data String
      dataString += String(wind_speed());
      dataString += ",";
      dataString += String(force());
      dataString += ",";
      dataString += String(rpm());
      Serial.print(dataString);
      break;

    default: // Error in Comms
      Serial.print("N/A");
      break;
  }
}

void anemometer_read(void) {
  ANEMOMETER_BUFFER[ANEMOMETER_INDEX++] = analogRead(ANEMOMETER_PIN);
  ANEMOMETER_INDEX %= (ANEMOMETER_BUFFER_SIZE);
}

float anemometer_average(void) {
  float average = 0;
  for(byte i = 0; i < ANEMOMETER_BUFFER_SIZE; i++) {
    average += ANEMOMETER_BUFFER[i];
  }
  average /= ANEMOMETER_BUFFER_SIZE;
  return average;
}

float anemometer_voltage(void) {
  return ((5*anemometer_average())/1024);
}

float wind_speed(void){
  return ((20*anemometer_voltage())-7.6);
}

void load_cell_read(void) {
  LOAD_CELL_BUFFER[LOAD_CELL_INDEX] = LoadCell.getData();
  LOAD_CELL_INDEX %= (LOAD_CELL_BUFFER_SIZE);
}

float load_cell_average(void) {
  float average = 0;
  for(byte i = 0; i < LOAD_CELL_BUFFER_SIZE; i++) {
    average += LOAD_CELL_BUFFER[i];
  }
  average /= LOAD_CELL_BUFFER_SIZE;
  return average;
}

float force() {
  return load_cell_average();
}

void hall_sensor_read(void) {
  unsigned int hall_value = analogRead(HALL_SENSOR_PIN);
  if(HALL_SENSOR_ON) {
    if(hall_value < HALL_SENSOR_OFF_VALUE) {
      HALL_SENSOR_ON = false;
    }
  }
  else { // Hall Sensor Off
    if(hall_value > HALL_SENSOR_OFF_VALUE) {
      unsigned long new_time = millis();
      HALL_SENSOR_BUFFER[HALL_SENSOR_INDEX++] = new_time - HALL_SENSOR_LAST_READ;
      HALL_SENSOR_LAST_READ = new_time;
      HALL_SENSOR_INDEX %= (HALL_SENSOR_BUFFER_SIZE);
      HALL_SENSOR_ON = true;
    }
  }
}

float hall_sensor_average(void) {
  float average = 0;
  for(byte i = 0; i < HALL_SENSOR_BUFFER_SIZE; i++) {
    average += HALL_SENSOR_BUFFER[i];
  }
  average /= HALL_SENSOR_BUFFER_SIZE;
  return average;
}

float rpm(void) {
  return 60000.0/hall_sensor_average(); // 60.0/(millis/1000)
}
