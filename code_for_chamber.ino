
/* Things to do */
// Omission count not working yet
// to add stim generator for training sessions.


/* Some commonly used functions
  Serial2.println("xyz")  --> Print the statement or variable values to the serial monitor
  myFile.print            --> Print the statement or variable values to the text file in SD card
  millis()                --> Returns the number of milliseconds passed since the Arduino board began running the current program.
  digitalWrite( pin, HIGH)--> Write a HIGH or a LOW value to a digital output pin.
  digitalRead(pin)        --> Reads the value from a specified digital pin.
*/

  /*          PINS            */
#include <Servo.h>            // import library for Servo Control
#include <SPI.h>              // import library for SPI communication with SD card Module 
#include <SD.h>               // import library to read and write on SD card.    
File myFile;

Servo foodServo;
int foodServoPin = 10;        // Food dispenser servo
int IRSensorLeft = 21;        // IR Sensor at the leftmost hole
int IRSensorRight = 20;       // IR Sensor at the rightmost hole
int IRSensorFood = 19;        // IR Sensor at the food dispenser hole
int chamberLED = 22;          // Chamber LEDs
int foodDispenserLED = 23;    // Food tray LED
int holesLED = 24;            // LEDs in the Holes


/* Varibles to count number of pokes using interrupts */
int NoOfTrials = 0;           
int countL = 0;               // Total count of Left hole pokes
int countR = 0;               // Total count of Right hole pokes
int countF = 0;               // Total count of Food Dispenser hole pokes
int countCorrectL = 0;        // count of correct pokes in left hole
int countCorrectR = 0;        // count of correct pokes in right hole
int countIncorrectL = 0;      // count of incorrect pokes in left hole
int countIncorrectR = 0;      // count of incorrect pokes in right hole
int countPrematureL = 0;      // count of Premature(before the stimulus was sent) pokes in left hole
int countPrematureR = 0;      // count of Premature(before the stimulus was sent) pokes in right hole
int countPrematureF = 0;      // count of Premature(before the poking in the correct hole) pokes in Food Dispenser hole
unsigned long pokeTime = 0;   // stores the time when hole was poked
int countOmission = 0;        // count of Omissions: when rat does nothing after the stimulus is sent.
bool stimSent = false;        // boolean to keep track of the period before and after sending stimulus
bool foodTray = false;        // boolean to keep track of the period during which it is supposed to collect food.
bool Omission = false;        // boolean to keep track of Omission: any kind of activity after stimulus is sent.

/* The ratio of stimulus to be sent */
int random_ratio = 50;


/* Start Time Variables */
unsigned long startTime1;
unsigned long startTime2;
unsigned long startTime3;
/* Defining the Dealy times */

/* Delay Variables in milliseconds */
const unsigned long HolesLightsON_delay = 3000;
const unsigned long FoodTrayLightsON_delay = 3000;
const unsigned long IncorrectResponseTimeOut_delay = 3000;
const unsigned long InertialInterval_delay = 3000;
const unsigned long StimulusDuration_delay = 500;

int stim; //stores the values of stimulus, 0=left, 1=right


void setup() {

  Serial2.begin(9600);                 //Initializing Serial communication on Pins TX2,RX2 (16,17)
  while (!Serial2) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  /* checking if SD card is connected */
  Serial2.print("Initializing SD card...");

  if (!SD.begin(53)) {
    Serial2.println("initialization failed!");
    while (1);
  }
  Serial2.println("initialization done.");
  
  sdWriteHead();                     

  //Defining mode of pins as input/output
  pinMode (IRSensorLeft, INPUT);
  pinMode (IRSensorRight, INPUT);
  pinMode (IRSensorFood, INPUT);
  pinMode (chamberLED, OUTPUT);
  pinMode (foodDispenserLED, OUTPUT);
  pinMode (holesLED, OUTPUT);
  foodServo.attach(foodServoPin);
  foodServo.write(0);

  Serial2.println("started");            

  // Interrupts to count the number of pokes(correct, incorrect and premature pokes)
  attachInterrupt(digitalPinToInterrupt(IRSensorLeft), LeftHole, RISING);
  attachInterrupt(digitalPinToInterrupt(IRSensorRight), RightHole, RISING);
  attachInterrupt(digitalPinToInterrupt(IRSensorFood), FoodHole, RISING);
}


void loop() {
  NoOfTrials++;                        //increasing the number of trial no. at the start of each trial 
  digitalWrite(chamberLED, HIGH);
  digitalWrite(foodDispenserLED, LOW);
  digitalWrite(holesLED, LOW);
  customDelay(InertialInterval_delay); //added the inertial delay here.
  stim = generateRand();               //  0 = O-stim = left, 1 = S-stim = right

  Serial2.print("stimulus sent = ");
  Serial2.println(stim);               //send this out to the other arduino to deliver stimulus.
  
  Omission = true;
  stimSent=true;
  customDelay(StimulusDuration_delay); //added delay for duration of stimulous if required.

  digitalWrite(chamberLED, LOW);
  digitalWrite(holesLED, HIGH);

  startTime1 = millis();
  while (millis() - startTime1 <= HolesLightsON_delay) {
    if (stim) Right();                 //When stimulus corresponding to the right hole is provided
    else Left();                       //When stimulus corresponding to the left hole is provided
  }
  Serial2.println(" end ");
  
  if(Omission) countOmission++;
  stimSent=false;
}

/* Functions Left() and Right() contain all the checks corresponding to the signal that has been sent. */

void Left() {
  delay(10);
  if (digitalRead(IRSensorLeft)) {         //checking for left hole poke
    correctResponse();
  }
  else if (digitalRead(IRSensorRight)) {   //checking for right hole poke
    incorrectResponse();
  }
}

void Right() {
  delay(10);
  if (digitalRead(IRSensorRight)) {
    correctResponse();
  }
  else if (digitalRead(IRSensorLeft)) {
    incorrectResponse();
  }
}

void correctResponse() {
  foodTray =true;                      //the rat is now eligible to poke in food tray
  if(stim ==0) countCorrectL++;        //increasing the count for correct response corresponding to the stimulus sent.
  else countCorrectR++;
  
  Serial2.println("correct response");
  digitalWrite(holesLED, LOW);
  digitalWrite(foodDispenserLED, HIGH);
  
  startTime2 = millis();
  while (millis() - startTime2 <= FoodTrayLightsON_delay) {
    if (digitalRead(IRSensorFood)
    ) {
      servoControl();                   // send off the signal to food dispenser servo;
      digitalWrite(foodDispenserLED, LOW);
      break;
    }
  }
  startTime1 = startTime1 + HolesLightsON_delay; //to end the loop.
  foodTray = false;                     //the rat is now not eligible to poke in food tray
}

void incorrectResponse() {
  if(stim ==0) countIncorrectR++;                 //increasing the count for incorrect response corresponding to the stimulus sent.
  else countIncorrectL++;
  Serial2.println("incorrect response");
  digitalWrite(holesLED, LOW);
  customDelay(IncorrectResponseTimeOut_delay);
  startTime1 = startTime1 + HolesLightsON_delay;  //to end the wait of response and start a new loop.
}

void LeftHole() {
  pokeTime=millis();  //storing the exact poke Time
  countL++;
  printCounts();
  if(stimSent == false) countPrematureL++; //increasing the number of premature counts
  Omission = false;
}
void RightHole() {
  pokeTime=millis();
  countR++;
  printCounts();
  if(stimSent == false) countPrematureR++;
  Omission = false;
}
void FoodHole() {
  pokeTime=millis();
  countF++;
  printCounts();
  if(foodTray == false) countPrematureF++;
  Omission = false;
}

void printCounts() {      //function for printing all the counts to serial monitor
  
  sdWrite();              //writing the data to the SD Card
  //Serial2.println("Printed data to SD Card");
  Serial2.print("Trial No. = ");
  Serial2.print(NoOfTrials);
  Serial2.print(" count L = ");
  Serial2.print(countL);
  Serial2.print(" R = ");
  Serial2.print(countR);
  Serial2.print(" F = ");
  Serial2.println(countF);
//  Serial2.print(" CorrectL ");
//  Serial2.print(countCorrectL);
//  Serial2.print(" CorrectR ");
//  Serial2.print(countCorrectR);
//  Serial2.print(" IncorrectL ");
//  Serial2.print(countIncorrectL);
//  Serial2.print(" IncorrectR ");
//  Serial2.print(countIncorrectR);
//  Serial2.print(" PrematureL ");
//  Serial2.print(countPrematureL);
//  Serial2.print(" PrematureR ");
//  Serial2.print(countPrematureR);
//  Serial2.print(" PrematureF ");
//  Serial2.print(countPrematureF);
//  Serial2.print(" Omission ");
//  Serial2.print(countOmission);
//  Serial2.print(" pokeTime ");
//  Serial2.print(pokeTime);
//  Serial2.println(" ms ");
}

int generateRand() {     //generates a random number 0 or 1 according to the ratio
  int x = random(0, 100);
  if (x > random_ratio)
    return 1;
  else return 0;
}

void servoControl() {    // function to control servo
  int angle = foodServo.read();
  foodServo.write((angle + 60) % 180);
  Serial2.println("food delivered");
}

void customDelay(int delayTime) {
  startTime3 = millis();
  while (millis() - startTime3 <= delayTime) {
  }
}

void sdWriteHead() {      //Writes the heading of all the data to the sd card seprated with comma
  
  myFile = SD.open("test7.csv", FILE_WRITE);   //insert name of the file here
  
  // if the file opened okay, write to it:
  if (myFile) {
    myFile.println("Trial_Number,countL,countR,countF,countCorrectL,countCorrectR,countIncorrectL,countIncorrectR,countPrematureL,countPrematureR,countPrematureF,countOmission,pokeTime");
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial2.println("error opening the text file");
  }

}

void sdWrite() {         //Writes all the data to the sd card seprated with comma
  
  myFile = SD.open("test7.csv", FILE_WRITE);       //insert name of the file here
  
  // if the file opened okay, write to it:
  if (myFile) {
    myFile.print(NoOfTrials);
    myFile.print(",");
    myFile.print(countL);
    myFile.print(",");
    myFile.print(countR);
    myFile.print(",");
    myFile.print(countF);
    myFile.print(",");
    myFile.print(countCorrectL);
    myFile.print(",");
    myFile.print(countCorrectR);
    myFile.print(",");
    myFile.print(countIncorrectL);
    myFile.print(",");
    myFile.print(countIncorrectR);
    myFile.print(",");
    myFile.print(countPrematureL);
    myFile.print(",");
    myFile.print(countPrematureR);
    myFile.print(",");
    myFile.print(countPrematureF);
    myFile.print(",");
    myFile.print(countOmission);
    myFile.print(",");
    myFile.print(pokeTime);
    myFile.println(" ms");
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial2.println("error opening the text file");
  }
}
