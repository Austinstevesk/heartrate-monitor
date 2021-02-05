#include <SoftwareSerial.h>
#define DEBUG true
SoftwareSerial gprsSerial(9, 10); //Rx, Tx -> For GSM module
#include <LiquidCrystal.h>
#include <stdlib.h>
LiquidCrystal lcd(2,3,4,5,6,7); //RS, E, D4, D5, D6, D7
 

String number = "+254712746036";
 
//Variables
float temp;
int hum;
String tempC;
int error;
int pulsePin = A1; // Pulse Sensor connected to analog pin
int buzzer = 13; // pin to send sound to buzzer
int fadePin = 5;
int faderate = 0;
bool alert = false;
 
// Volatile Variables, used in the interrupt service routine!
volatile int BPM; // int that holds raw Analog in 0. updated every 2mS
volatile int Signal; // holds the incoming raw data
volatile int IBI = 600; // int that holds the time interval between beats! Must be seeded!
volatile boolean Pulse = false; // "True" when heartbeat is detected. "False" when not a "live beat".
volatile boolean QS = false; // becomes true when Arduino finds a beat.
 
// Regards Serial OutPut -- Set This Up to your needs
static boolean serialVisual = true; // Set to 'false' by Default.
volatile int rate[10]; // array to hold last ten IBI values
volatile unsigned long sampleCounter = 0; // used to determine pulse timing
volatile unsigned long lastBeatTime = 0; // used to find IBI
volatile int P =512; // used to find peak in pulse wave
volatile int T = 512; // used to find trough in pulse wave
volatile int thresh = 525; // used to find instant moment of heart beat
volatile int amp = 100; // used to hold amplitude of pulse waveform
volatile boolean firstBeat = true; // used to seed rate array
volatile boolean secondBeat = false; // used to seed rate array

 
void setup()
{      
pinMode(fadePin,OUTPUT);                     
interruptSetup();
lcd.begin(16, 2);
lcd.print("Connecting...");
Serial.begin(9600);
gprsSerial.begin(9600);
pinMode(buzzer, OUTPUT);
Serial.println("AT");
interruptSetup();
}

 
void loop(){
  if(QS){
lcd.clear();
start:
error=0;
lcd.setCursor(0, 0);
lcd.print("BPM = ");
lcd.print(BPM);
lcd.setCursor(0, 1);
lcd.print("Normal BPM");
Serial.print("BPM = ");
Serial.println(BPM);
delay(1000);

if(alert == false){
   if(BPM>100){
      digitalWrite(buzzer, HIGH);
      lcd.clear();
      lcd.print("BPM = ");
      lcd.print(BPM);
      lcd.setCursor(0,1);
      lcd.print("BPM Too High");
      bpmTooHigh();
      alert = true;
}
if(BPM<35){
  digitalWrite(buzzer, HIGH);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("BPM = ");
  lcd.print(BPM);
  lcd.setCursor(0,1);
  lcd.print("BPM too Low");
  bpmTooLow();
  alert = true;
  
}
}
if(alert){
  if(BPM<100 && BPM >35){
  digitalWrite(buzzer, LOW);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("BPM = ");
  lcd.print(BPM);
  lcd.setCursor(0,1);
  lcd.print("Normal BPM");
  alert = false;
  }
}

updatebeat();
if (error==1){
goto start; //go to label "start"
}
QS = false;
  }
  else{
    digitalWrite(buzzer, LOW);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("No heartbeat:");
    lcd.setCursor(0,1);
    lcd.print("Finger removed");
    //Serial.println("Sensor disconnected ");
   

  }

delay(100);
}
 
void updatebeat(){
if (gprsSerial.available()){
    Serial.write(gprsSerial.read());
 
  gprsSerial.println("AT");
 
  gprsSerial.println("AT+CPIN?");
  delay(1000);
 
  gprsSerial.println("AT+CREG?");
  delay(1000);
 
  gprsSerial.println("AT+CGATT?");
  delay(1000);
 
  gprsSerial.println("AT+CIPSHUT");
  delay(1000);
 
  gprsSerial.println("AT+CIPSTATUS");
  delay(2000);
 
  gprsSerial.println("AT+CIPMUX=0");
  delay(2000);
 
  ShowSerialData();
 
  gprsSerial.println("AT+CSTT=\"safaricom\"");//start task and setting the APN,
  delay(1000);
 
  ShowSerialData();
 
  gprsSerial.println("AT+CIICR");//bring up wireless connection
  delay(3000);
 
  ShowSerialData();
 
  gprsSerial.println("AT+CIFSR");//get local IP adress
  delay(2000);
 
  ShowSerialData();
 
  gprsSerial.println("AT+CIPSPRT=0");
  delay(3000);
 
  ShowSerialData();
  
  gprsSerial.println("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",\"80\"");//start up the connection
  delay(6000);
 
  ShowSerialData();
 
  gprsSerial.println("AT+CIPSEND");//begin send data to remote server
  delay(4000);
  ShowSerialData();
  
  String str="GET https://api.thingspeak.com/update?api_key=MD868VXVCJSZEJ1Y&field1=" + String(BPM);
  Serial.println(str);
  gprsSerial.println(str);//begin send data to remote server
  
  ShowSerialData();
 
  gprsSerial.println((char)26);//sending
  delay(5000);//waiting for reply, important! the time is base on the condition of internet 
  gprsSerial.println();
 
  ShowSerialData();
 
  gprsSerial.println("AT+CIPSHUT");//close the connection
  delay(100);
  ShowSerialData();
}
else{
  Serial.println("Unable to send data");
}
}

void ShowSerialData()
{
  while(gprsSerial.available()!=0)
  Serial.write(gprsSerial.read());
  Serial.println("GPRS Available");
  delay(5000); 
  
}
 

void interruptSetup(){
TCCR2A = 0x02; // DISABLE PWM ON DIGITAL PINS 3 AND 11, AND GO INTO CTC MODE
TCCR2B = 0x06; // DON'T FORCE COMPARE, 256 PRESCALER
OCR2A = 0X7C; // SET THE TOP OF THE COUNT TO 124 FOR 500Hz SAMPLE RATE
TIMSK2 = 0x02; // ENABLE INTERRUPT ON MATCH BETWEEN TIMER2 AND OCR2A
sei(); // MAKE SURE GLOBAL INTERRUPTS ARE ENABLED
}
 
ISR(TIMER2_COMPA_vect){ // triggered when Timer2 counts to 124
cli(); // disable interrupts while we do this
Signal = analogRead(pulsePin); // read the Pulse Sensor
sampleCounter += 2; // keep track of the time in mS
int N = sampleCounter - lastBeatTime; // monitor the time since the last beat to avoid noise
 
// find the peak and trough of the pulse wave
if(Signal < thresh && N > (IBI/5)*3){ // avoid dichrotic noise by waiting 3/5 of last IBI
if (Signal < T){ // T is the trough
T = Signal; // keep track of lowest point in pulse wave
}
}
 
if(Signal > thresh && Signal > P){ // thresh condition helps avoid noise
P = Signal; // P is the peak
} // keep track of highest point in pulse wave
 
if (N > 250){ // avoid high frequency noise
if ( (Signal > thresh) && (Pulse == false) && (N > (IBI/5)*3) ){
Pulse = true; // set the Pulse flag when there is a pulse

IBI = sampleCounter - lastBeatTime; // time between beats in mS
lastBeatTime = sampleCounter; // keep track of time for next pulse 
if(secondBeat){ // if this is the second beat
secondBeat = false; // clear secondBeat flag
for(int i=0; i<=9; i++){ // seed the running total to get a realistic BPM at startup
rate[i] = IBI;
}
}
 
if(firstBeat){ // if it's the first time beat is found
firstBeat = false; // clear firstBeat flag
secondBeat = true; // set the second beat flag
sei(); // enable interrupts again
return; // IBI value is unreliable so discard it
}
word runningTotal = 0; // clear the runningTotal variable
 
for(int i=0; i<=8; i++){ // shift data in the rate array
rate[i] = rate[i+1]; // and drop the oldest IBI value
runningTotal += rate[i]; // add up the 9 oldest IBI values
}
 
rate[9] = IBI; // add the latest IBI to the rate array
runningTotal += rate[9]; // add the latest IBI to runningTotal
runningTotal /= 10; // average the last 10 IBI values
BPM = 60000/runningTotal; // how many beats can fit into a minute? that's BPM!
QS = true; // set Quantified Self flag
}
}
 
if (Signal < thresh && Pulse == true){ // when the values are going down, the beat is over
Pulse = false; // reset the Pulse flag so we can do it again
amp = P - T; // get amplitude of the pulse wave
thresh = amp/2 + T; // set thresh at 50% of the amplitude
P = thresh; // reset these for next time
T = thresh;
}
 
if (N > 2500){ // if 2.5 seconds go by without a beat
thresh = 512; // set thresh default
P = 512; // set P default
T = 512; // set T default
lastBeatTime = sampleCounter; // bring the lastBeatTime up to date
firstBeat = true; // set these to avoid noise
secondBeat = false; // when we get the heartbeat back
}
sei();
}
   void bpmTooHigh()
    {
      Serial.println ("Sending Message........\n");
      gprsSerial.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
      delay(1000);
      //Serial.println ("Set SMS Number"); nb
      gprsSerial.println("AT+CMGS=\"" + number + "\"\r"); //Mobile phone number to send message
      delay(1000);
      //char value[5];
      //itoa(h,value,10); //convert integer to char array      
      String SMS = "Warning! BPM too high!";
      gprsSerial.println(SMS);
      Serial.println(SMS);
      delay(100);
      gprsSerial.println((char)26);// ASCII code of CTRL+Z
      Serial.println("Sms sent");
      delay(1000);
      //_buffer = _readSerial();
    }


   
    void bpmTooLow()
    {
      Serial.println ("Sending Message..........\n");
      gprsSerial.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
      delay(1000);
      //Serial.println ("Set SMS Number");
      gprsSerial.println("AT+CMGS=\"" + number + "\"\r"); //Mobile phone number to send message
      delay(1000);
      //char value[5];
      //itoa(h,value,10); //convert integer to char array      
      String SMS = "Warning! BPM too low!";
      gprsSerial.println(SMS);
      Serial.println(SMS);
      delay(100);
      gprsSerial.println((char)26);// ASCII code of CTRL+Z
      Serial.println("Sms sent");
      delay(1000);
      //_buffer = _readSerial();
    }
