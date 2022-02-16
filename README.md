# Web-Remote

# Початкові налаштування
1. Підєднатись до WIFI точки доступу Wemos**AABBCCDDEEFF** (AABBCCDDEEFF - серійний номер вашої плати)
2. Перейти в Google Chrome на сторінку 192.168.4.1
3. У меню Google Chrome вибрати пункт - **додати на початковий екран**. На робочому столі створиться ярлик для швидкого запуску додатка.

# Що це і навіщо?

Дана прошивка розроблена для того, щоб розширити можливості звичайної 4-зо канальної радіоапаратури. 

Підключивши плату до апараткри у моделі повляються наступні опції:

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
Прошивка отримує сигнали від апаратури і на їх основі синхронно з діями оператора включає освіталення.

При повороті коліс у праву чи ліву сторону (на основі сигналів каналу X) автоматично включається відповідні сигнали повороту. 

Під час руху заднів моходом (на основі сигналів каналу Y) - автоматично включається лампи заднього ходу.
Під час скидання швидкості більше ніж на 5% (а також при повній зупинці моделі) відбувається автоматичне вимикання стопсигналів.

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
Onboard port address - адресамікросхеми PCF8574, що відповідає за освітлення.
Stop light duration - проміжок часу, на який включається стоп-сигнал після зупинки моделі
Back light timeout - проміжок часу, через який вимикається світло заднього ходу після зупинки моделі
Reverce limit - % швидкості заднього ходу після якого автоматично включається сигнал заднього ходу (тільки в ручному режимі)
Turn light limit - % поворуту коліс після якого автоматично включається відповідний синал повору


## Налаштування КПП
![Налаштування КПП](/tools/img/4-gearbox-settings.jpg)

## Налаштування селектора КПП
![Налаштування селектора КПП](/tools/img/5-gear-selector.jpg)

## АКПП - стратегія розгону
![АКПП - стратегія розгону](/tools/img/6-gear-selector-yellow-line.jpg)

## АКПП - Стратегія гальмування
![АКПП - Стратегія гальмування](/tools/img/7-gear-selector-orange-line.jpg)

## [КПП - обмеження передачі заднього ходу
![КПП - обмеження передачі заднього ходу](/tools/img/8-gear-selector-reverce.jpg)


# Для розробників
## Структура проекту
1. **HTML-Remote** - C# MVC проект для відлагодки HTML інтерфейсу та випробування різного роду технологій, які потім мігрують в ESP. Проект повністю повторяє функціонал ESP прошивки і розширює його (тільк в плані тестового іункціоналу)
  * З допомогою **bundleconfig.json** налаштована склейка та мініфікація скриптів, стилів та HTML файлів у підпапку **data**
  * З підпапки **data** файли копіюються (у WebRemote/data) вручну (можливо потім автоматизую цей процес....)
2. **WebRemote** - C++ проект прошивки ESP8266. Директорія data - містить HTML мініфіковіні файли
## Events API
### Протокол комунікації
1. Викликаємо GET api/EventSourceName. Метод повертає Route (адресу джерела івентів) по якому транслюється інформація з сервера на web-UI. Одним із параметів є DI клієнта
2. POST в контроллер **api** - Надсилаємо пакет з форматом даних **format:{fields:[field1, field2, field, ... fieldN]}\n**. Цього формату має притримуватись додаток при комунікації з приладом, і в такому ж форматі має відровідати прилад. (це для того, щоб мінімізувати трафік і одночасно забезпечити підтримку стрих версій додатку у яких може бути інша конфігурація елементів керування)
4. По Event source каналу отримуємо від прилада масив значень і розставляємо їх по елементах керування у відповідності до обговореного формату
5. Якщо користувач змінив якийсь параметр - надсилаємо приладу пакет у обговореному наперед форматі. (Повертаємось до п4)
6. Якщо звязок втрачено - гасимо лампочку **Connected**. Запускаємо зворотній відлік на 3 секнди і пробуємо відновити звязок (Повертаємось до п1)
### Формат пакету
```javascript
{
  client:"random string id of client"
  format:[field1, field2, field, ... fieldN],
  values:['v1', 'v2', 'v3', .. 'vN']
};
```

