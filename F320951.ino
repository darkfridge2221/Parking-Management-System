#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
#include <TimeLib.h>

//prototypes
void checkReg();
void addVehicle(const char* reg, char t, const char* loc, const char* payment, const char* entry, const char* exit);
void sync();
void paymentStatus(const char* message);
void changeType(const char* message);
void changeLocation(const char* message);
int findReg(const char* reg);
void printAllVehicles();
int freeMemory();

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield(); //define lcd
int currentlyDisplaying = 1; //shows which item in the array is being displayed
bool upButtonPressed = false; //flag for up button
bool downButtonPressed = false; //flag for down button
bool selectButtonPressed = false; //flag for select button
unsigned long selectButtonPressedTime = 0; //timer for select button


enum State : byte { //the available states
  SYNC,
  CHECK_REG,
  ADD,
  PAYMENT_STATUS,
  CHANGE_TYPE,
  CHANGE_LOCATION,
  REMOVE
};
State currentState = SYNC; //starting state

struct Vehicle { //defines the struct and info
  String regNumber;
  char type;
  String location;
  String paymentStatus;
  tmElements_t entryTime;
  tmElements_t exitTime;

  //default constructor
  Vehicle() {}

  //parameterized constructor
  Vehicle(String reg, char t, String loc, String payment, tmElements_t entry, tmElements_t exit) {
    regNumber = reg;
    type = t;
    location = loc;
    paymentStatus = payment;
    entryTime = entry;
    exitTime = exit;
  }
};

const int maxVehicles = 10; //maximum number of vehicles
int currentVehicles = 0; //counter to track the number of vehicles
Vehicle vehicles[maxVehicles]; //array that stores info for each vehicle


void setup(){
  // put your setup code here, to run once:
  Serial.begin(9600); 
  lcd.begin(16, 2); 

  byte upArrow[8] = {
    0b00100,
    0b01110,
    0b11111,
    0b01110,
    0b01110,
    0b01110,
    0b01110,
    0b01110
  }; //up arrow
  byte downArrow[8] = {
    0b01110,
    0b01110,
    0b01110,
    0b01110,
    0b01110,
    0b11111,
    0b01110,
    0b00100
  }; //down arrow 
  lcd.createChar(0, upArrow); lcd.createChar(1, downArrow); //create special characters
}

void loop(){
  // put your main code here, to run repeatedly:
  switch(currentState){
    case SYNC:
      sync();
      break;

    case CHECK_REG:
      checkReg();
      break;

    case ADD:
      break;

    case PAYMENT_STATUS:
      break;

    case CHANGE_TYPE:
      break;

    case CHANGE_LOCATION:
      break;

    case REMOVE:
      break;
  }
}

void addVehicle(String reg, char t, String loc, String payment){
  if (currentVehicles >= maxVehicles){ //checks if maximum amount of vehicles reached
    Serial.println(F("ERROR: Maximum number of vehicles reached"));
    return;
  }
  tmElements_t currentTime; //create a tmElements_t struct to represent the current time
  breakTime(now(), currentTime);

  //assign vehicle information to the array at the current index
  vehicles[currentVehicles].regNumber = reg;
  vehicles[currentVehicles].type = t;
  vehicles[currentVehicles].location = loc;
  vehicles[currentVehicles].paymentStatus = "NPD"; //set to default not paid
  vehicles[currentVehicles].entryTime = currentTime;  //set entry time to the current time
  vehicles[currentVehicles].exitTime = currentTime;   //set exit time to the current time
}

void sync() {
  lcd.setBacklight(5); //set backlight to purple
  static bool runSync = true;
  while (runSync) {
    delay(1000);  //wait 1 second before printing Q
    Serial.print(F("Q"));

    if (Serial.available() > 0) { //check for incoming data on serial
      char received = Serial.read();
      if (received == 'X'){
        runSync = false;
      }
    }
  }
  Serial.println(F("UDCHARS,FREERAM"));
  lcd.setBacklight(7); //set backlight to white and prind the extension tasks completed
  currentState = CHECK_REG;
}

void checkReg(){
  while (Serial.available() > 0){
    Serial.read(); //clear the serial buffer
  }
  while (Serial.available() == 0){
    checkButtons(); //check for incoming data while allowing for buttons to be used
  }
  String message = Serial.readString(); //read input 

  //validate format if the operation character is valid or if the regNumber is valid
  char firstCharacter = message.charAt(0);
  if (firstCharacter == 'R'){ 
    currentState = REMOVE; //if first character is R then change state to REMOVE
    remove(message);
  }
  else if(!(firstCharacter == 'A' || firstCharacter == 'S' || 
            firstCharacter == 'T' || firstCharacter == 'L' || 
            firstCharacter == 'R')){ //if the first character is not a valid operation
    Serial.println(F("ERROR: Invalid operation"));
    currentState = CHECK_REG;
  }
  else if (!isupper(message.charAt(2)) || !isupper(message.charAt(3)) || 
           !isdigit(message.charAt(4)) || !isdigit(message.charAt(5)) || 
           !isupper(message.charAt(6)) || !isupper(message.charAt(7)) || 
           !isupper(message.charAt(8))){ //if the regnumber is not a combination of upper case and numbers
    Serial.println(F("ERROR: Invalid registrration number"));
    currentState = CHECK_REG;
  }
  else if (message.charAt(1) != '-' || message.charAt(9) != '-'){ //if dashes are not in correct position
    Serial.println(F("ERROR: Dash not in correct position"));
    currentState = CHECK_REG;
  }
  else{ //run appropriate function depending on the operation character
    if (firstCharacter == 'A'){
      currentState = ADD;
      add(message);
    }
    if (firstCharacter == 'S'){
      currentState = PAYMENT_STATUS;
      paymentStatus(message);
    }
    if (firstCharacter == 'T'){ 
      currentState = CHANGE_TYPE;
      changeType(message);
    }
    if (firstCharacter == 'L'){
      currentState = CHANGE_LOCATION;
      changeLocation(message);
    }
  }
}

void add(String message) {
  //substring and extract the necessary info
  String regNumber = message.substring(2, 9);
  char type = message.charAt(10);
  String location = message.substring(12);
  char lastCharacter = message.charAt(10); 
  int index = findReg(message);
  if(!(lastCharacter == 'C' || lastCharacter == 'M' || //validating if the vehicle type is correct
       lastCharacter == 'V' || lastCharacter == 'L' ||
       lastCharacter == 'B')){
    Serial.println(F("ERROR: Incorrect vehicle type"));
    currentState = CHECK_REG;
  }
  else if (message.charAt(11) != '-'){ //checks if dash is in correct position
    Serial.println(F("ERROR: Dash not in correct position"));
    currentState = CHECK_REG;
  }
  else if (location.length() > 13 || location.length() <= 2){ //checks if the location string is the correct length
    Serial.println(F("ERROR: Invalid location string length"));
    currentState = CHECK_REG;
  }
  else if(vehicles[index].regNumber == regNumber || (vehicles[index].regNumber == regNumber || 
          vehicles[index].location == location || vehicles[index].type == type)){ //checks if vehicle already exists
    Serial.println(F("ERROR: Vehicle already exists"));
    currentState = CHECK_REG;
  }
  else{
    tmElements_t currentTime; //get the current time
    breakTime(now(), currentTime);
    addVehicle(regNumber, type, location, ""); //call the addVehicle function
    vehicles[index].entryTime = currentTime; //sets time to current time using timeLib
    currentVehicles++; //increments counter of vehicles
    displayInfo(); //display to LCD
    currentState = CHECK_REG; //change state 
  }
}

void paymentStatus(String message){
  String regNumber = message.substring(2, 9); //extract necessary information
  String status = message.substring(10, 12);
  int index = findReg(regNumber);
  if (index == -1){ //checks if vehicle reg number exists
    Serial.println(F("ERROR: Vehicle does not exist"));
    currentState = CHECK_REG;
  }
  else if (status.equals("PD")){  //check payment status and updates array
    tmElements_t currentTime;
    breakTime(now(), currentTime);
    vehicles[index].exitTime = currentTime;
    vehicles[index].paymentStatus = status;
    displayInfo();
    currentState = CHECK_REG;
  }
  else if (status.equals("NPD")){ //checks if they want to change status to NPD
    vehicles[index].paymentStatus = status;
    displayInfo();
    currentState = CHECK_REG;
  }
  else{ //display error and change state
    Serial.println(F("ERROR: Invalid payment status"));
    currentState = CHECK_REG;
  }
}

void changeType(String message){
  String regNumber = message.substring(2, 9); //extract and substring info
  char type = message.charAt(10);
  int index = findReg(regNumber);
  if (index == -1){ //checks if vehicle exists
    Serial.println(F("ERROR: Vehicle does not exist"));
    currentState = CHECK_REG; 
  }
  else if (vehicles[index].paymentStatus == "NPD"){ //checks if vehicle paid for
    Serial.println(F("ERROR: Vehicle not paid for"));
    currentState = CHECK_REG;
  }
  else{ //changes array and state
    vehicles[index].type = type;
    displayInfo();
    currentState = CHECK_REG; //display error message and change state
  }
}

void changeLocation(String message){
  String regNumber = message.substring(2, 9); //extract and substring info
  String location = message.substring(10);
  if (findReg(regNumber) == -1){ //checks if vehicle exists
    Serial.println(F("ERROR: Vehicle does not exist"));
    currentState = CHECK_REG; //changes state
  }
  else if (vehicles[findReg(regNumber)].paymentStatus == "NPD" || //checks if paid for or if assigning to same location
           vehicles[findReg(regNumber)].location == location){
    Serial.println(F("ERROR: Vehicle not paid for or assigning to same location"));
    currentState = CHECK_REG;
  }
  else{
    vehicles[findReg(regNumber)].location = location; //update array
    displayInfo();
    currentState = CHECK_REG; //display to LCD and change state
  }
}

void remove(String message){ 
  String regNumber = message.substring(2, 9); //extract information
  int index = findReg(regNumber);
  if (index == -1){ //checks if vehicle exists
    Serial.println(F("ERROR: Vehicle does not exist"));
    currentState = CHECK_REG;
  }
  else if (vehicles[index].paymentStatus == "NPD"){ //checks if vehicle is paid for
    Serial.println(F("ERROR: Vehicle not paid for"));
    currentState = CHECK_REG;
  }
  else{
    for (int i = index; i < currentVehicles; ++i) { //shift all items down by one 
      vehicles[i].regNumber = vehicles[i + 1].regNumber;
      vehicles[i].type = vehicles[i + 1].type;
      vehicles[i].location = vehicles[i + 1].location;
      vehicles[i].paymentStatus = vehicles[i + 1].paymentStatus;
      vehicles[i].entryTime = vehicles[i + 1].entryTime;
      vehicles[i].exitTime = vehicles[i + 1].exitTime;
    }
    currentVehicles--; currentlyDisplaying--;
    displayInfo();
    currentState = CHECK_REG;
  }
} 

int findReg (String reg){ //looks through vehicles array to find a matched regNumber then returns its index
  for (int i = 0; i < currentVehicles; i++) {
    if (vehicles[i].regNumber == reg) {
      return i;
    }
  }
  //return -1 if the regNumber not found
  return -1;
}

void displayInfo (){
  if (currentVehicles >= 2){ //if more than one vehicle, display only down arrow
    lcd.setCursor(0, 0); lcd.print(" "); 
    lcd.setCursor(0, 1); lcd.write((uint8_t)1);
  }  
  lcd.setCursor(1, 0);
  lcd.print(vehicles[0].regNumber); //display reg number
  if (vehicles[0].paymentStatus == "PD"){ //if vehicle is paid for
    lcd.setBacklight(2); //set colour to green
    lcd.setCursor(4, 1); lcd.print(vehicles[0].paymentStatus); //display payment status
    lcd.setCursor(12, 1); lcd.print(formatTime(vehicles[0].exitTime)); //display exit time
  }
  if (vehicles[0].paymentStatus == "NPD"){ //if vehicle not paid for
    lcd.setBacklight(3); //set colour to yello
    lcd.setCursor(3, 1); lcd.print(vehicles[0].paymentStatus); //display payment status
  }
  lcd.setCursor(7, 1); lcd.print(formatTime(vehicles[0].entryTime));  //display the entry time

  lcd.setCursor(1, 1); lcd.print(vehicles[0].type); //display the vehicle type
  lcd.print(" "); lcd.setCursor(9, 0); //print a space to separate vehicle type and 
  lcd.print(vehicles[0].location.substring(0, 7)); //display the vehicle location
  //scroll(vehicles[0].location); this is where scroll wouldve been if i managed to implement it
  currentlyDisplaying = 1; //update the currentlyDisplaying index
}

void printTime(tmElements_t time) { //print time in HHMM format
  lcd.print(String(time.Hour, DEC));
  lcd.print(String(time.Minute, DEC));
}

void checkButtons(){
  uint8_t buttons = lcd.readButtons(); //read state of buttons on LCD 

  if (buttons & BUTTON_UP) { //check if up button pressed
    if (!upButtonPressed) {
      up();
      upButtonPressed = true;
      downButtonPressed = false; //reset downButtonPressed
    }
  } else if (buttons & BUTTON_DOWN) { //check if down button pressed
    if (!downButtonPressed) {
      down();
      downButtonPressed = true;
      upButtonPressed = false; //reset upButtonPressed
    }
  } else { //reset button flags when no button is pressed
    upButtonPressed = false;
    downButtonPressed = false;
  }
   if (buttons & BUTTON_SELECT){ //check if select button is pressed
    if (!selectButtonPressed){
      // SELECT button is pressed
      selectButtonPressedTime = millis();
      selectButtonPressed = true;
    } else { //select button is held
      unsigned long currentTime = millis();
      if (currentTime - selectButtonPressedTime >= 1000) { //clear screen, set LCD to purple, and display student ID and bytes free
        lcd.clear();
        lcd.setBacklight(5); //purple backlight
        lcd.setCursor(0, 0); lcd.print(" F320951"); //display id number
        lcd.setCursor(1, 1); lcd.print(freeMemory()); lcd.print(" bytes    "); //display amount of free bytes
      }
    }
  } 
  else {//select button is released
    if (selectButtonPressed) { //restore the display to its previous state
      if (currentVehicles == 0){
        lcd.clear();
        lcd.setBacklight(7); //white backlight
        selectButtonPressed = false;
      }
      else{
        displayInfo();
        selectButtonPressed = false;
      }
    }
  }
}


int up(){ 
  if (currentVehicles <= 1){ //if less than or equal to one vehicle, do not do anything
    return;
  }
  if (currentlyDisplaying == 2){ //if displaying second vehicle, display only down arrow
    lcd.setCursor(0, 0); lcd.print(" ");
    lcd.setCursor(0, 1); lcd.write((uint8_t)1); //display down arrow
  }
  else if (currentlyDisplaying <= 1){ //if displaying first vehicle, only display down arrow
    lcd.setCursor(0, 1); lcd.print(" ");
    lcd.setCursor(0, 1); lcd.write((uint8_t)1); //display down arrow
    return;
  }
  else{
    lcd.setCursor(0, 1); lcd.write((uint8_t)1); // Display down arrow
  }
  currentlyDisplaying--;

  lcd.setCursor(1, 0); lcd.print(vehicles[currentlyDisplaying - 1].regNumber); //display the reg number
  lcd.setCursor(9, 0); lcd.print(vehicles[currentlyDisplaying - 1].location.substring(0, 7)); //display the location
  //scroll(vehicles[currentlyDisplaying - 1].location); this is where scroll wouldve been if i found hwo to implement it
  lcd.setCursor(1, 1); lcd.print(vehicles[currentlyDisplaying - 1].type); //display vehicle type
  lcd.print(" ");
  if (vehicles[currentlyDisplaying - 1].paymentStatus == "PD"){ //display payment status with correct colour
    lcd.print(" "); lcd.print(vehicles[currentlyDisplaying - 1].paymentStatus); //display payment status
    lcd.setBacklight(2); //set to green
  }
  else{
    lcd.print(vehicles[currentlyDisplaying - 1].paymentStatus); //display payment status
    lcd.setBacklight(3); //set to yellow
    lcd.setCursor(12, 1); lcd.print("    "); //clear this section of display
  }
  lcd.print(" "); lcd.print(formatTime(vehicles[currentlyDisplaying - 1].entryTime)); //display entry time
  if (vehicles[currentlyDisplaying - 1].paymentStatus == "PD"){ //display exit time if paid 
    lcd.setCursor(12, 1); lcd.print(formatTime(vehicles[currentlyDisplaying - 1].exitTime));
  }
  return currentlyDisplaying;
}

int down(){
  if (currentVehicles <= 1){ //if less than one vehicle, dont do anything
    return;
  }
  if (currentlyDisplaying >= currentVehicles - 1){ //if displaying not the first vehicle, display only and up arrow
    lcd.setCursor(0, 1); lcd.print(" ");
    lcd.setCursor(0, 0); lcd.write((uint8_t)0); // Display up arrow
  }
  else{
    lcd.setCursor(0, 0); lcd.write((uint8_t)0); // Display up arrow
  }
  if (currentlyDisplaying >= currentVehicles){
    return;
  }
  if (vehicles[currentlyDisplaying].paymentStatus == "PD"){
    lcd.setBacklight(2); //set colour to green
  }
  if (vehicles[currentlyDisplaying].paymentStatus == "NPD"){
    lcd.setBacklight(3); //set colour to yellow
  }

  lcd.setCursor(1, 0); lcd.print(vehicles[currentlyDisplaying].regNumber); //display the reg number
  lcd.setCursor(9, 0); lcd.print(vehicles[currentlyDisplaying].location.substring(0, 7)); //display the location
  if (vehicles[currentlyDisplaying].paymentStatus == "PD"){ //display payment status with correct colour 
    lcd.setBacklight(2); //set colour to green
    lcd.setCursor(12, 1); lcd.print(formatTime(vehicles[currentlyDisplaying].exitTime)); //display the exit time
    lcd.setCursor(3, 1); lcd.print(" "); lcd.print(vehicles[currentlyDisplaying].paymentStatus); //display the payment status
  }
  if (vehicles[currentlyDisplaying].paymentStatus == "NPD"){
    lcd.setBacklight(3); //set colour to yellow
    lcd.setCursor(3, 1); lcd.print(vehicles[currentlyDisplaying].paymentStatus); //display the payment status
    lcd.setCursor(12, 1); lcd.print("    "); //clear this section of the display
  }
  lcd.setCursor(1, 1); lcd.print(vehicles[currentlyDisplaying].type); //display the vehicle type
  lcd.setCursor(7, 1); lcd.print(formatTime(vehicles[currentlyDisplaying].entryTime)); //display the entry time
  currentlyDisplaying++; return currentlyDisplaying; //go to next item
}

String formatTime(tmElements_t timeElements) {
  char buffer[20]; //formats time in HHMM format
  sprintf(buffer, "%02d%02d", timeElements.Hour, timeElements.Minute);
  return String(buffer);
}

int freeMemory() {
  int size = 2048; //calculate and return free memory
  byte *buf;
  while ((buf = (byte *)malloc(--size)) == NULL);
  free(buf);
  return size;
}

/*
void printAllVehicles() { //print array information for debugging
  for (int i = 0; i < currentVehicles; ++i) {
    Serial.print(F("Vehicle "));
    Serial.print(i + 1);
    Serial.print(F(": RegNumber="));
    Serial.print(vehicles[i].regNumber);
    Serial.print(F(", Type="));
    Serial.print(vehicles[i].type);
    Serial.print(F(", Location="));
    Serial.print(vehicles[i].location);
    Serial.print(F(", PaymentStatus="));
    Serial.print(vehicles[i].paymentStatus);
    Serial.print(F(", EntryTime="));
    Serial.print(formatTime(vehicles[i].entryTime));
    Serial.print(F(", ExitTime="));
    Serial.print(formatTime(vehicles[i].exitTime));
  }
}

void scroll(String location){
  if (location.length() > 7){
    for (int i = 0; i <= location.length(); i++){
      if (i >= location.length() - 7){
        i = -1;
      }
      else{
        lcd.setCursor(9, 0);
        lcd.print(location.substring(i, 7+i));
        delay(500);
      }
    }
  }
  else{
    lcd.setCursor(9, 0);
    lcd.print(location);
  }
}
*/