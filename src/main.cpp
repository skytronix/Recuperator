#include <Arduino.h>
#include <DallasTemperature.h>
//#include <OneWire.h>
//#include <SPI.h>
#include <Ethernet2.h>
#include <PubSubClient.h>
#include <Servo.h>

//-------------------------------------------------------------------------------------------------
//---------------------------Определения и установки для датчиков DS18B20--------------------------
//-------------------------------------------------------------------------------------------------
//Цифровой контакт на которую подключены датчики
#define ONE_WIRE_BUS 2
// Установка разрешающей способности датчика
// 9 bit - 0.5 градуса
// 10 bit - 0.25 градуса
// 11 bit - 0,125 градуса
// 12 bit - 0,0625 градуса
#define TEMPERATURE_PRECISION 11

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

int numberOfDevices; // Количество найденных температурных датчиков
  unsigned char* Addr0 = new unsigned char[50];
  String Str0 = "";

DeviceAddress tempDeviceAddress; // Переменная для хранения адреса найденного датчика
//-------------------------------------------------------------------------------------------------

int flag = 0;

#define vent_in_high 0     //Вентилятор притока максимальная скорость
#define vent_in_low 1      //Вентилятор притока половинная скорость
#define vent_out_high 7    //Вентилятор вытяжки максимальная скорость
#define vent_out_low 8     //Вентилятор вытяжки половинная скорость
#define valve_1 5          //пин к которому подключена первая заслонка
#define valve_2 6          //пин к которому подключена вторая заслонка
#define pressure_1 A0      //датчик перепада давления 1
#define pressure_2 A1      //датчик перепада давления 2
#define pressure_3 A2      //датчик перепада давления 3

//Время через которое отправлять показания
#define TimeSend 10000 //Каждые 10 секунд

#define ID_CONNECT "Recuperator"

byte mac[]    = { 0xE8, 0x40, 0xF2, 0x72, 0xF1, 0xB2 };
byte server[] = { 10, 1, 20, 10 };
byte ip[]     = { 10, 1, 20, 199 };

char tmp1[10]; //Буффер для передачи переменных значений по сети
char buf[10];

//Переменные для счета времени
unsigned long currentTime = 0;
unsigned long loopTime = 0;

EthernetClient ethClient;
Servo Servo_Valve_1; //Объявляем переменную Servo_Valve_1 типа Servo для задвижки №1
Servo Servo_Valve_2; //Объявляем переменную Servo_Valve_1 типа Servo для задвижки №2
//PubSubClient client(server, 1883, callback, ethClient);

//**************************************************************
//******** Процедура обработки входящих топиков*****************
//**************************************************************
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String strTopic = String(topic);
  String strPayload = String((char*)payload);

  //обработка принятых сообщений
  if (strTopic == "Recuperator/valve_1")
      {
      Serial.println("Input valve_1 = " + strPayload);
      Servo_Valve_1.write(map(strPayload.toInt(),0,100,0,180));
      }

  if (strTopic == "Recuperator/valve_2")
    {
      Serial.println("Input valve_2 = " + strPayload);
      Servo_Valve_2.write(map(strPayload.toInt(),0,100,0,180));
    }

  if (strTopic == "Recuperator/valve_all")
    {
      Serial.println("Input valve_all = " + strPayload);
      Servo_Valve_1.write(map(strPayload.toInt(),0,100,0,180));
      Servo_Valve_2.write(map(strPayload.toInt(),0,100,0,180));
    }

   if (strTopic == "Recuperator/vent_in")
    {
      if (strPayload == "off")
        {
          digitalWrite(vent_in_high, LOW);
          digitalWrite(vent_in_low, LOW);
        }
      if (strPayload == "low")
        {
          digitalWrite(vent_in_high, LOW);
          digitalWrite(vent_in_low, HIGH);
        }
      if (strPayload == "high")
        {
          digitalWrite(vent_in_low, LOW);
          digitalWrite(vent_in_high, HIGH);
        }
    }

   if (strTopic == "Recuperator/vent_out")
    {
      if (strPayload == "off")
        {
          digitalWrite(vent_out_high, LOW);
          digitalWrite(vent_out_low, LOW);
        }
      if (strPayload == "low")
        {
          digitalWrite(vent_out_high, LOW);
          digitalWrite(vent_out_low, HIGH);
        }
      if (strPayload == "high")
        {
          digitalWrite(vent_out_low, LOW);
          digitalWrite(vent_out_high, HIGH);
        }
    }
}
//**************************************************************

PubSubClient client(server, 1883, callback, ethClient);

//Процедура переподключения
void reconnect() {
  while (!client.connected()) {
    if (client.connect(ID_CONNECT)) {
      client.subscribe("Recuperator/#");
    } else {
      Serial.println("SERVER CONNECTION IS LOST!!!");
      delay(5000);
    }
  }
}
//***************************************************************
// Функция возвращает адрес датчика в виде строки
//***************************************************************
const char* printAddress(DeviceAddress deviceAddress)
{
  Str0 = "";
  for (uint8_t i = 0; i < 8; i++)
  {
    //if (deviceAddress[i] < 16) String Addr = String("0");
    //Serial.print(deviceAddress[i], HEX);
    Str0 = Str0 + String(deviceAddress[i], HEX);
  }
  Str0 = "Recuperator/Temp/" + Str0;
  Str0.getBytes(Addr0,50,0);
  return Addr0;
}
//***************************************************************

void setup() {

  pinMode(vent_in_high, OUTPUT);
  pinMode(vent_in_low, OUTPUT);
  pinMode(vent_out_high, OUTPUT);
  pinMode(vent_out_low, OUTPUT);

  Serial.begin(57600);
  Servo_Valve_1.attach(valve_1); //привязываем первую заслонку к типу серво
  Servo_Valve_2.attach(valve_2); //привязываем вторую заслонку к типу серво
  Ethernet.begin(mac, ip); //Запускаем сетевой модуль W5500

  if (client.connect(ID_CONNECT)) {
    client.publish("Recuperator/open1", "false");
    client.publish("Recuperator/open2", "false");
    client.publish("Recuperator/mon1", "false");
    client.publish("Recuperator/ring1", "false");
    client.subscribe("Recuperator/#");
  }
  currentTime = millis();// считываем время, прошедшее с момента запуска программы
  loopTime = currentTime;//и присваиваем значение циклу времени

//--------------------------------------------------------------------------------
  // Запуск библиотеки датчиков
  sensors.begin();

  // Определяем общее количество датчиков
  numberOfDevices = sensors.getDeviceCount();

  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" sensor.");

  // Печать информации о паразитном питании
  Serial.print("Parasite power is: ");
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

  // Цикл печати информации о найденных датчиках
  for (int i = 0; i < numberOfDevices; i++)
  {
    // Search the wire for address
    if (sensors.getAddress(tempDeviceAddress, i))
    {
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      Serial.print(printAddress(tempDeviceAddress));
      Serial.println();

      Serial.print("Setting resolution to ");
      Serial.println(TEMPERATURE_PRECISION, DEC);

      // Установка разрешающей способности датчика
      // 9 bit - 0.5 градуса
      // 10 bit - 0.25 градуса
      // 11 bit - 0,125 градуса
      // 12 bit - 0,0625 градуса
      sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);

      Serial.print("Resolution actually set to: ");
      Serial.print(sensors.getResolution(tempDeviceAddress), DEC);
      Serial.println();
    } else {
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }
  }
//--------------------------------------------------------------------------------
}


void loop() {
  if (!client.connected()) {
    reconnect();
    client.subscribe("Recuperator/#");
  }

  client.loop();//Цикличная обработка соединения с сервером
  //Отслеживаем звонок в домофон
//  if (digitalRead(ring1_pin) == LOW && flag == 0) {
//    client.publish("Recuperator/ring1", "true");
//    flag = 1;//выставляем  переменную flag в единицу
//  }
//  else if (digitalRead(ring1_pin) == HIGH && flag == 1) {
//    client.publish("Recuperator/ring1", "false");
//    flag = 0; //обнуляем переменную flag
//  }

//***********************************************************************************************
//************Выполнение происходит каждые TimeSend миллисекунд без остановки *******************
//***********************************************************************************************
  currentTime = millis();                        // считываем время, прошедшее с момента запуска программы
  if(currentTime >= (loopTime + TimeSend))       // сравниваем текущий таймер с переменной loopTime + 1 секунда
  {
    //Условие выполняется каждые TimeSend миллисекунд
    sensors.requestTemperatures(); // Команда отправляет запрос на считывание температуры
    // Цикл выполняет печать температуры со всех датчиков
    for (int i = 0; i < numberOfDevices; i++)
      {
         // Поиск на шине всех датчиков
       if (sensors.getAddress(tempDeviceAddress, i))
        {
            float tempC = sensors.getTempC(tempDeviceAddress); //Получаем температуру
            dtostrf(tempC, -2, 2, tmp1); //Преобразуем в массив

           client.publish(printAddress(tempDeviceAddress), tmp1); //Отправляем на сервер значения
           Serial.print(printAddress(tempDeviceAddress));// Вывод в консоль серийного номера датчика
           // Печатаем температуру с найденного датчика
           Serial.print("     ");
           Serial.print(tmp1);
           Serial.println();
        }
    //else ghost device! Check your power requirements and cabling
    //delay(1000);
         loopTime = currentTime;
      }
    itoa(analogRead(pressure_1),buf,10);
    client.publish("Recuperator/pressure_1", buf);
    itoa(analogRead(pressure_2),buf,10);
    client.publish("Recuperator/pressure_2", buf);
    itoa(analogRead(pressure_3),buf,10);
    client.publish("Recuperator/pressure_3", buf);
//    loopTime = currentTime;                     // в loopTime записываем новое значение
  }

//***********************************************************************************************
}

//Функция возвращает температуры от датчиков
//String printTemperature(DeviceAddress deviceAddress)
//{
  // method 2 - faster
  //float tempC = sensors.getTempC(deviceAddress);
//  String val = String(sensors.getTempC(deviceAddress));
//  return val;
  //Serial.print(" Temp ");
  //Serial.print(tempC);
//}
