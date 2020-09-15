#include "Joypad.h"

int Joypad_id = 0;
int tran = 0;

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
	//Serial.print("json=");	Serial.println(*json);

	int _tran = json->getInt("tran");
	if (_tran >= tran) tran = _tran;
	//Serial.print("tran=");	Serial.println(_tran);

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
				Joypadfield* f = (Joypadfield*)(fields->get(i++));
				if (f != nullptr) {
					f->value = str.substring(fieldsIndex, endIndex).toFloat();
					//f->sent = f->value;
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
	unsigned long m;
	//m = millis(); Serial.printf("*1 %i\n", m);
	if (this->client.connected()) {

		JsonString ret = "event: event\n\ndata: ";
		ret.beginObject();
		ret.AddValue("tran", String(tran));
		ret.beginArray("values");
		if (fields == nullptr) return true;
		Joypadfield* f = (Joypadfield*)(fields->getFirst());
		while (f != nullptr) {
			ret.appendComa();
			int v = f->value;
			ret += "\"" + String(v) + "\"";
			f->sent = f->value;
			f = (Joypadfield*)(f->next);
		}
		ret.endArray();
		ret.endObject();
		ret += "\n\n";
		//m = millis(); Serial.printf("*2 %i\n", m);
		//Serial.printf("Client %i send values\n", id);
		this->client.print(ret);
		//m = millis(); Serial.printf("*3 %i\n", m);
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

bool Joypad::changed()
{
	if (fields == nullptr) return false;
	Joypadfield* f = (Joypadfield*)(fields->getFirst());
	while (f != nullptr) {
		if (f->changed()) return true;
		f = (Joypadfield*)(f->next);
	}
	return false;
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

void JoypadCollection::updateValuesFrom(Joypad* source)
{
	if (fields == nullptr) {
		fields = new Collection();
	}
	Joypadfield* j = (Joypadfield*)(source->fields->getFirst());
	while (j != nullptr) {
		setValue(j->name, j->value);
		j = (Joypadfield*)(j->next);
	}
}



void JoypadCollection::setValue(String name, double value)
{
	if (fields == nullptr) {
		fields = new Collection();
	}
	//Проставляємо поточні значення
	if (!JoypadCollection::setValue(fields, name, value)) {
		//не знайшли, додаємо
		Joypadfield* jf = new Joypadfield(name);
		jf->value = value;
		fields->add(jf);
	};

	//Поновляємо значення у всіх клієнтах
	Joypad* j = (Joypad*)getFirst();
	while (j != nullptr) {
		JoypadCollection::setValue(j->fields, name, value);
		j = (Joypad*)(j->next);
	}
}

double JoypadCollection::getValue(String name)
{
	if (fields == nullptr) return 0;
	Joypadfield* j = (Joypadfield*)(fields->getFirst());
	while (j != nullptr) {
		if (name == j->name) {//Знайшли
			return j->value;
		}
		j = (Joypadfield*)(j->next);
	}
	return 0;
}

bool JoypadCollection::setValue(Collection* fields, String name, double value)
{
	if (fields == nullptr) return false;

	Joypadfield* j = (Joypadfield*)(fields->getFirst());
	while (j != nullptr) {
		if (name == j->name) {//Знайшли
			j->value = value;
			return true;
		}
		j = (Joypadfield*)(j->next);
	}
	return false;
}

void JoypadCollection::loop()
{
	unsigned long m = millis();

	Joypad* j = (Joypad*)getFirst();
	while (j != nullptr) {
		if (j->changed()) {
			if ((m - j->report) > reportAliveInterval) {

				//Serial.print("id=");
				//Serial.print(j->id);
				//Serial.print("m=");
				//Serial.println(m);

				if (!j->sendValues()) {
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
		else if ((m - j->report) > keepAliveInterval) {
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
