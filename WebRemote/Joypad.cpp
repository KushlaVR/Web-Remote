#include "Joypad.h"

int Joypad_id = 0;

Joypad::Joypad()
{
	Joypad_id++;
	id = Joypad_id;
}

bool Joypad::keepAlive()
{
	if (this->client.connected()) {
		Serial.printf_P(PSTR("Client %i is still connected\n"), id);
		this->client.println(F("event: event\ndata: { \"TYPE\":\"KEEP-ALIVE\" }\n"));   // Extra newline required by SSE standard
		report = millis();
		return true;
	}
	else {
		Serial.printf_P(PSTR("Client %i diconnected;\n"), id);
		this->client.flush();
		this->client.stop();
		this->clientIP = INADDR_NONE;
		return false;
	}
}

bool Joypad::processParcel(JsonString* json)
{
	int fieldsIndex = json->indexOf("fields");
	if (fieldsIndex >= 0) {
		String str = json->substring(fieldsIndex + 8);
		fields = new Collection();
		fieldsIndex = str.indexOf("[");
		if (fieldsIndex >= 0) {
			str = str.substring(fieldsIndex + 1);
			fieldsIndex = str.indexOf("\"");
			while (fieldsIndex >= 0) {
				fieldsIndex++;
				int endIndex = str.indexOf("\"", fieldsIndex);
				Joypadfield* f = new Joypadfield(str.substring(fieldsIndex, endIndex));
				fields->add(f);
				fieldsIndex = endIndex + 1;
				fieldsIndex = str.indexOf("\"", endIndex + 1);
				Serial.println(f->name);
			}
			return true;
		}
	}
	fieldsIndex = json->indexOf("values");
	if (fieldsIndex >= 0) {
		String str = json->substring(fieldsIndex + 8);
		fieldsIndex = str.indexOf("[");
		if (fieldsIndex >= 0) {
			str = str.substring(fieldsIndex + 1);
			fieldsIndex = str.indexOf("\"");
			int i = 0;
			while (fieldsIndex >= 0) {
				fieldsIndex++;
				int endIndex = str.indexOf("\"", fieldsIndex);
				Joypadfield* f = (Joypadfield *)(fields->get(i));
				if (f != nullptr) {
					f->value = str.substring(fieldsIndex, endIndex).toDouble();
				}
				fieldsIndex = endIndex + 1;
				fieldsIndex = str.indexOf("\"", endIndex + 1);
			}
			return true;
		}
	}
	return false;
}

bool Joypad::sendValues()
{
	JsonString ret = "";
	ret.beginObject();
	ret.beginArray("values");
	if (fields == nullptr) return true;
	Joypadfield* f = (Joypadfield*)(fields->getFirst());
	while (f != nullptr) {
		ret.appendComa();
		ret += "\"" + String(f->value) + "\"";
		f->sent = f->value;
		f = (Joypadfield*)(f->next);
	}
	ret.endArray();
	ret.endObject();

	if (this->client.connected()) {
		Serial.printf_P(PSTR("Client %i is still connected\n"), id);
		this->client.println(F("event: event\n"));
		this->client.print(F("data: "));
		this->client.print(ret);
		this->client.print(F("\n\n"));
		report = millis();
		return true;
	}
	else {
		Serial.printf_P(PSTR("Client %i diconnected;\n"), id);
		this->client.flush();
		this->client.stop();
		this->clientIP = INADDR_NONE;
		return false;
	}
}

JoypadCollection::JoypadCollection()
{


}

Joypad* JoypadCollection::getById(int id)
{
	Joypad* j = (Joypad*)getFirst();
	while (j != nullptr) {
		if (j->id == id) return j;
		j = (Joypad*)(j->next);
	}
	return nullptr;
}

void JoypadCollection::keepAlive()
{
	unsigned long m = millis();

	Joypad* j = (Joypad*)getFirst();
	while (j != nullptr) {
		if (m - j->report > 10000) {
			if (!j->keepAlive()) {
				//Немає звязку
				Joypad* next = (Joypad*)(j->next);
				remove(j);
				delete j;
				j = next;
			}
			else {
				j = (Joypad*)(j->next);
			}
		}
		else {
			j = (Joypad*)(j->next);
		}
	}

}

Joypadfield::Joypadfield(String name)
{
	this->name = name;
}
