#pragma once
#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif


class BenchMark
{
	ulong m = 0;
	ulong length = 0;
	ulong lastMeasure = 0;

public:
	//Інтервал між вимірами
	ulong inteval = 300;
	//Обчислена позиція в градусах відповідно до вхідного діапазону
	ulong pos;
	//Виміряна довжина імпульса
	ulong ImpulsLength = 0;
	bool isChanged;

	//Мінімальне допустиме вхідне значення (все що менше - приводиться до мін)
	ulong IN_min = 550;
	//Позиція центра
	ulong IN_center = 1475;
	//Максимальне допустиме значення (все що більше - приводиться до максимума)
	ulong IN_max = 2400;

	//Градуси при мінімальному вхідному значенню
	int OUT_min = 0;
	//Градуси при максимальному вхідному значенню
	int OUT_max = 180;

	BenchMark();
	~BenchMark();
	void SetFakeValue(ulong v);
	void ICACHE_RAM_ATTR Start();
	void ICACHE_RAM_ATTR Stop();
	//Чи актуальні результати вимірів
	bool isActual();

	void loop();
	int convert(ulong value);
	bool isValid();

};

