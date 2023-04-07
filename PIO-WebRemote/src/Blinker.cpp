#include "Blinker.h"



Blinker::Blinker(String name)
{
	first = nullptr;
	current = nullptr;
	this->name = name;
}


Blinker::~Blinker()
{
}

void Blinker::loop()
{
	if (startTime == 0) return;
	unsigned long offset = millis() - startTime;//������ ���� ������� �� �������
	if (current == nullptr) return;//���� ���� �������� �� ���� - ����� �� ������
	while (true) {
		BlinkerItem* item = current;
		if (offset < item->offset) return;//���� ��� �� �� ������ - ��������
		write(item->pin, item->value);//��� ������
		if (debug) {
			console.print(name);
			console.printf(": %i->%i\n", item->pin, item->value);
		}
		current = item->next;//���������� �� ���������� ��������

		if (current == nullptr)//����� ������
		{
			if (repeat) {//�������� � �������
				current = first;
				startTime = millis();
				if (debug) {
					console.print(name);
					console.println(": ...");
				}
				return;
			}
			end();
			return;
		}
	}
}

Blinker* Blinker::Add(int pin, unsigned long offset, int value)
{
	pinMode(pin, OUTPUT);
	BlinkerItem* item = new BlinkerItem();
	item->pin = pin;
	item->offset = offset;
	item->value = value;
	if (first == nullptr) {
		first = item;
		last = item;
	}
	else {
		last->next = item;
		last = item;
	}
	return this;
}

Blinker* Blinker::end()
{
	current = nullptr;
	startTime = 0;
	BlinkerItem* item = first;
	while (item != nullptr)
	{
		write(item->pin, startupState);
		item = item->next;
	}
	return this;
}

void Blinker::write(int pin, int value)
{
	if (value == 0)
		digitalWrite(pin, LOW);
	else if (value == 255)
		digitalWrite(pin, HIGH);
	else
		analogWrite(pin, value);
}

void Blinker::printValues()
{
	console.print(name);
	console.println(":");

	int i = 0;
	BlinkerItem* item = first;
	while (item != nullptr)
	{
		console.print("    ");
		console.print(i);
		console.print(" ");
		console.printf(": %i->%i;\n", item->pin, item->value);
		item = item->next;
		i++;
	}
	console.println("");

}

BlinkerItem* Blinker::item(int index)
{
	int i = 0;
	BlinkerItem* item = first;
	while (item != nullptr)
	{
		if (i == index) return item;
		item = item->next;
		i++;
	}
	return nullptr;
}

BlinkerItem::BlinkerItem()
{
	next = nullptr;
}

void Beeper::write(int pin, int value)
{
	if (value == 0)
		noTone(pin);
	else {
		//analogWriteFreq(value);
		tone(pin, value);
	}
}


void VirtualBlinker::write(int pin, int value) {
	if (this->writeMethode == nullptr) return;
	this->writeMethode(pin, value);
}
