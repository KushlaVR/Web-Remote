# Web-Remote


# Поки ти читаєш цей текст - українці гинуть від російських ракет.
Ти можеш перерахувати будь-яку суму на спеціальний рахунок Національного Банку України для допомоги збройним силам україни у протистоянні російському окупанту.
Навть незначна сума може врятувати чиєсь життя!

### Для зарахування коштів у національній валюті:

Банк: Національний банк України

МФО 300001

Рахунок № UA843000010000000047330992708

код ЄДРПОУ 00032106

Отримувач: Національний банк України 

### Для зарахування коштів у USD: 

BENEFICIARY: National Bank of Ukraine

BENEFICIARY BIC: NBUA UA UX

BENEFICIARY ADDRESS: 9 Instytutska St, Kyiv, 01601, Ukraine

ACCOUNT NUMBER: 804790258

BENEFICIARY BANK NAME: JP MORGAN CHASE BANK, New York

BENEFICIARY BANK BIC: CHASUS33

ABA 0210 0002 1

BENEFICIARY BANK ADDRESS: 383 Madison Avenue, New York, NY 10017, USA

PURPOSE OF PAYMENT: for ac 47330992708 

### Для зарахування коштів у EUR: 

BENEFICIARY: National Bank of Ukraine

IBAN DE05504000005040040066

PURPOSE OF PAYMENT: for ac 47330992708

BENEFICIARY BANK NAME: DEUTSCHE  BUNDESBANK, Frankfurt

BENEFICIARY BANK BIC: MARKDEFF

BENEFICIARY BANK ADDRESS: Wilhelm-Epstein-Strasse 14, 60431 Frankfurt Am Main, Germany

### Для зарахування коштів у GBP: 

BENEFICIARY/RECIPIENT NAME: National Bank of Ukraine

ACCOUNT NUMBER: 80033041

IBAN GB52CHAS60924280033041

BENEFICIARY ADDRESS: 9 Instytutska St, Kyiv, 01601, Ukraine

BENEFICIARY BANK NAME: JP MORGAN CHASE BANK NA, London

BENEFICIARY BANK BIC: CHASGB2L

SORT CODE: 60-92-42 

BENEFICIARY BANK ADDRESS: 125 London Wall, London EC2Y 5AJ, UK

PURPOSE OF PAYMENT: for ac 47330992708

# Початкові налаштування
1. Підєднатись до WIFI точки доступу Wemos**AABBCCDDEEFF** (AABBCCDDEEFF - серійний номер вашої плати)
2. Перейти в Google Chrome на сторінку 192.168.4.1
3. У меню Google Chrome вибрати пункт - **додати на початковий екран**. На робочому столі створиться ярлик для швидкого запуску додатка.


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

