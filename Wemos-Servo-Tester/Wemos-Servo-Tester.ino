/*
 Name:		Wemos_Servo_Tester.ino
 Created:	4/4/2021 8:21:06 PM
 Author:	Віталік
*/

#include <Servo.h>

Servo servo1 = Servo();
Servo servo2 = Servo();
Servo servo3 = Servo();
Servo servo4 = Servo();
Servo servo5 = Servo();
Servo servo6 = Servo();
Servo servo7 = Servo();
Servo servo8 = Servo();
Servo servo9 = Servo();

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	pinMode(D0, OUTPUT);
	pinMode(D1, OUTPUT);
	pinMode(D2, OUTPUT);
	pinMode(D3, OUTPUT);
	pinMode(D4, OUTPUT);
	pinMode(D5, OUTPUT);
	pinMode(D6, OUTPUT);
	pinMode(D7, OUTPUT);
	pinMode(D8, OUTPUT);

	servo1.attach(D1);
	servo2.attach(D2);
	servo3.attach(D3);
	servo4.attach(D4);
	servo5.attach(D8);
	servo6.attach(D7);
	servo7.attach(D6);
	servo8.attach(D5);
	servo9.attach(D0);

}

// the loop function runs over and over again until power down or reset
void loop() {
	servo1.write(90-50);
	servo2.write(90-30);
	servo3.write(90-20);
	servo4.write(90-10);
	servo5.write(90);
	servo6.write(90+10);
	servo7.write(90+20);
	servo8.write(90+30);
	servo9.write(90+50);
}
