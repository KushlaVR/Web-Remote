# Web-Remote
Web-remote це прошивка для розширення можливостей приймача радокерованих моделей
Для прошивки розроблено спеціальну плату, яка демонструє всі її можливості.
![Налаштування точки доступу](/Schematic/photo-top.png){ height="300px" }
![Налаштування точки доступу](/Schematic/photo-bottom.png){ height="300px" }



# Початкові налаштування
1. Підєднатись до WIFI точки доступу Wemos**AABBCCDDEEFF** (AABBCCDDEEFF - серійний номер вашої плати)
2. Перейти в Google Chrome на сторінку 192.168.4.1
3. У меню Google Chrome вибрати пункт - **додати на початковий екран**. На робочому столі створиться ярлик для швидкого запуску додатка.

# Що це і навіщо?

Дана прошивка розроблена для модуля ESP12 (і всіх його модифікацій - наприклад Wemos D1 mini, lolin, NodeMCU  і т.п.). 
Вона може значно розширити можливості звичайної 4-ох канальної радіоапаратури. 

Підключивши плату до апаратури у моделі повляються наступні опції:

1. Додається 8 додаткових каналів для підключення емітації освітлення

	* Лампи заднього ходу
	* Лампи стопсигналів
	* Лампи поворотів (правий/лівий борт)
	* Лампи ближнього світла
	* Лампи дальноього світла
	* Лампи протитуманок
	* Лампи стоянкових вогнів

2. Додається альтернативний канал для підключення сервоприводу селектора коробки передач
3. Додається альтернативний канал для керування регулятором

# Як це працює?

Плата підключається до радіоприймача апаратури керування моделлю. 
Прошивка аналізує сигнали від апаратури і на їх основі синхронно з діями оператора включає освіталення та керує механізмом вибору передач КПП.

При повороті коліс у праву чи ліву сторону (на основі сигналів каналу X) автоматично включаються відповідні сигнали поворотів. 

Під час руху заднім ходом (на основі сигналів каналу Y) - автоматично включається лампи заднього ходу.

Під час скидання швидкості більше ніж на 5% (а також при повній зупинці моделі) відбувається автоматичне вмикання стопсигналів.

Канал 3 (кнопка на радіоапаратурі) використовується для керування освітленням. Кожне натискання кнопки перемика освітлювальні прибори моделі 
по колу (вимкнено -> стоянкові вогні -> бллижнє світло -> дальнє світло -> вимкнено). Подвійний клік вмикає/вимикає протитуманки. 
Утримання кнопки вмикає/вимикає аварійну світлову сигналізацію.

Канал 4 (3-ох позиціний перемикач) використовується для керування селектором КПП. Прошивка може керувати моделлю в двох режимах - ручний та автоматичний.

# Пояснення налаштувань

## Налаштування WIFI
![Налаштування точки доступу](/tools/img/1-config.jpg)
* SSID - назва танка в мережах WIFI
* Password - пароль для підключення до данка

## Налаштування входів

Вхідні сигнали апаратури можуть бути налаштовані на апаратурі (якщо є підтримка такої опції), 
або параметри прошивки можуть бути змінені відповідно до вашої апаратури (якщо немає можливості підлаштувати апаратуру)

Кожен із 4-хох входів має 3 основних параметра (min, max, center)

![Налаштування входів](/tools/img/2-input-monitor.jpg)

min, max - відображають крайні (максимально можливі) положення органу керування. Параметр center - обчислюється автоматично і є суто інформативним.

Як дізнатись ці крайні положення? Для цього необхідно авктивувати монітор (кнопка "Monitor"). 
Після цього у полі "current" почнуть відображатись поточні значення з радіоапаратури. В цей момент необхідно перевести орган керування у одне із крайніх положень
і запамятати значення відповідного каналу. Коли визначено min та max показник - їх необхідно вписати у поля для каналу і записати зміни (кнопка "Save" внизу сторінки налаштувань)

## Налаштування світла
![Налаштування світла](/tools/img/3-light-settings.jpg)

* Onboard port address - адреса мікросхеми PCF8574, що відповідає за освітлення.
* Stop light duration - проміжок часу, на який включається стоп-сигнал після зупинки моделі
* Back light timeout - проміжок часу, через який вимикається світло заднього ходу після зупинки моделі
* Reverce limit - % швидкості заднього ходу після якого автоматично включається сигнал заднього ходу (тільки в ручному режимі)
* Turn light limit - % поворуту коліс після якого автоматично включається відповідний синал повору


## Налаштування КПП
![Налаштування КПП](/tools/img/4-gearbox-settings.jpg)
* Mode - режим роботи
* Neutral - положення селектора КПП для нейтральної передачі
* Gear 1 - положення селектора КПП для першої передачі
* Gear 2 - положення селектора КПП для другої передачі

## АКПП 

Наступні налаштування мають значення лише для роботи в автоматичному режимі.
Канал 4 має 3 робочі положення (1 - крайнє ліве , 2 - середнє, 3 - крайнє праве). 

В ручному режиі ці положеня переводять селектор КПП у положення:

* 1 передача - (1) крайнє ліве
* Нейтралка  - (2) середнє
* 2 передача - (3) крайнє праве

В автоматичному режимі ці положеня переводять АКПП у стан:

* Parking - (1) крайнє ліве
* Neutral  - (2) середнє
* Drive - (3) крайнє праве

### АКПП - налаштування стратегії вибору передач
![Налаштування селектора КПП](/tools/img/5-gear-selector.jpg)

* Acceleration from 0 to 100 - тривалість розгону моделі від 0 до 100%. Цей параметр обмежує максимальне прискорення моделі, додаючи копійності руху. 
	Для прикладу: Типове значення цього параметра 5000 (це відповідає 5 секундам). Це означає, що зажавши курок до максимума модель буде плавно розганятись до 
	максимальної швидкості на протязі 5-ти секунд (як і звичайний автомобіль розганяється до швидкості 50..60км/год).
	Також цей параметр впливає і на гальмування. Тривалість гальмівного шляху від 100% до 0 складає 1/4 від цього параметра.
	Тобто відпустивши курок на максимальній швидкості модель скине до 0 за 1,2 секунди.

* Gear actuator trigger gap - зазор спрацьовання механізму перемикання передач (вимірюється у процентах він швидкості). 
	Цей параметр визначає як поводить себе КПП у зоні перемикання передач (в мент переходу між 1-ю та 2-ю передачами).
	При досягненні максимальної швидкості першої передачі - коробка перейде на другу. 
	Але людина не взмозі точно утримувати курок у заданій позиції. На граничній зоні можливі незначні рухи курком (наприклад +/-2%)
	Щоб під час цих стрибків знову не повертатись на першу передачу у прошивку введено невеликий зазор. 
	Тобто повернення на першу передачу буде здійснене з невеликим запізненням.
	Це виключає непотрібні перемикання коробки і додає копійності руху моделі.


Розгін моделі в автоматичному режимі відбувається не синхронно із положенням курка. Це необхідно врахувати при керуванні.
В автоматичному режимі - положення курка задає швидкість моделі (швидкість на колесах). 
При цьому одинакові оберти коліс можуть відповідати різним обертам мотора. Це залежить від включеної передачі.

Приклад: перевівши курок в положення 50% - модель розганятиметься до цієї швидкості на першій передачі відповідно до налаштувань. 
Досягнувши 50% швидкості на колесах - оберти мотора будуть 90%. З цього моменту прискорення припинеться і модель продовжить рух із сталою швидкістю.

Якщо потім перевести курок у положення 70% - модель знову продовжить розгін до максимальних обертів першої передачі, 
перейде на другу передачу, скине оберти мотора і знову почне набір обертів для досягнення цільової швидкості на колесах.

Відповідно відпустивши курок (наприклад до 40%) модель скине оберти мотора до мінімальних для другої передачі, включить першу передачу, 
підвищить оберти мотора так, щоб зберегти плавність руху (забезпечити незмінну швидкість на колесах в момент переключення), 
і далі продовжить скидати обертів мотора до досягнення 40% на колесах.


### Стратегія розгону

![АКПП - стратегія розгону](/tools/img/6-gear-selector-yellow-line.jpg)

* Gear 1 minimum motor speed - швидкість з якої розпочинається рух моделі на першій передачі.
* Gear 1 maximum motor speed - максимальні оберти мотора для першої передачі (точка А на графіку). При досягенні цієї швидкості відбудеться перехід на 2-гу передачу
* Gear 2 start motor speed - оберти мотора під час включення 2-гої передачі (точка B на графіку). При переході з першої на другу передачу коробка спочатку переходить в нейтральне положення, 
	і скидає оберти мотора, щоб при включенні другої передачу модель не смикалась. Даний показник необхідно обчислити на основі характеристик КПП, або підібрати експерементальним шляхом.

### Стратегія гальмування
![АКПП - Стратегія гальмування](/tools/img/7-gear-selector-orange-line.jpg)
* Gear 2 maximum motor speed - максимальні оберти мотора для руху на другій передачі.
* Gear 2 minimum motor speed - мінімальні оберти мотора для руху на другій передачі (точка C на графіку). Придосягненні цієї швидкості відбудеться перехід на першу передачу.
* Gear 1 start motor speed - оберти мотора під час включення 1-ї передачі (точка D на графіку). При переході з другої передачі на першу
	коробка спочатку переходить в нейтральне положення, 
	і додає оберти мотора, щоб при включенні першої передачу модель не смикалась. Даний показник необхідно обчислити на основі характеристик КПП, або підібрати експерементальним шляхом.

### Обмеження передачі заднього ходу
![КПП - обмеження передачі заднього ходу](/tools/img/8-gear-selector-reverce.jpg)
* Gear R maximum motor speed - максимальні оберти мотора для руху заднім ходом.
* Gear R minimum motor speed - мінімальні оберти мотора для руху заднім ходом.

### Блокування

Для уникнення надмірних перевантажень коробки і мехінзмів моделі у прошивці реалізовані наступні блокування 

* У режимі P - примусово вмикається 1 передача і модель не реагує на положення курка
* У режимі N - примусово вмикається лише нейтралка і модель реагує на положення курка. Можна газувати на місці.
* У режимі N - перехіду у режим D чи P відбувається тільки при повній зупинці мотора незалежно від селектора режиму роботи АКПП.
* У режимі D - перехід у P, N чи задній хід відбувається тільки при повній зупинці мотора незалежно від селектора режиму роботи АКПП.
* У режимі R - перехід у P, N чи передній хід відбувається тільки при повній зупинці мотора незалежно від селектора режиму роботи АКПП.

Проте варто зауважити, що на моторі не встановлено ніяких датчиків, моментом зупинки вважається команда стоп на регуляторі. 
Тому не варто зловживати і надмірно надіятись на блокування. 
Керування відбувається на розсуд оператора і за наслідки також несе відповідальність виключно оператор.

## Купити автору каву

Цей проект я роблю абсолютно без оплатно, вкладаючи туди свій вільний час. 

Якщо тебе зацікавив проект - ти можеш його використати абсолютно безкоштовно. 
Не забуть поставити мені зірочку і поширити посилання серед своїх друзів. 

Також твоя вдячність може бути у вигляді чашки кави. 

Купити мені каву можна з допомогою патреона

https://www.patreon.com/KushlaVR

А ще краще - оформити мені підписку, щоб я пив каву регулярно, меньше спав і розвивав цей проект :-)

## Про мене

https://kushlavr.github.io

Дякую за увагу. 

Тримай фоста пістолєтом.


