#include <Sim800l.h>
#include <SoftwareSerial.h> //is necesary for the library!! 

Sim800l Sim800l;  //to declare the library
String text;     // to storage the text of the sms
uint8_t index;   // to indicate the message to read.


void setup(){
	Serial.begin(9600); // only for debug the results . 
	Sim800l.begin(); // initializate the library. 
	index=1; // first position in the prefered memory storage. 
	text=Sim800l.readSms(index);   	
	Serial.println(text);

}

void loop(){
	//do nothing
}
