#include "mbed.h"
#include "EthernetInterface.h"
#include "Adafruit_SSD1306.h"
#include "ESP8266Interface.h"
#include "DHT.h"
#include "http_request.h"
#include <ctime>

// ETH
EthernetInterface ethernetInterface;

// OLED DISPLAY
I2C _i2c(I2C_SDA, I2C_SCL);
Adafruit_SSD1306_I2c oledDisplay(_i2c, D5, D4); // display() Pins for Display (SDA, SCL, RES, DC, CS)
Adafruit_SSD1306_I2c *handlerGetDisplay()
{
    return &oledDisplay;
}

// Sensor Controller
DHT dht; // Sensor DHT (D2, DHT22 or DHT11)

// Control Fragment (button, speaker)
InterruptIn buttonTime;          // buttonTime(); Pin for Button
InterruptIn buttonTriggerAlarm;  // buttonTriggerAlarm(); Pin for Button
InterruptIn buttonAlarmSettings; // buttonAlarmSettings(); Pin for Button
InterruptIn buttonRepeatAlarm;   // buttonRepeatAlarm(); Pin for Button
InterruptIn buttonMuteAlarm;     // buttonMuteAlarm(); Pin for Button
InterruptIn buttonTriggerSensor; // buttonTriggerSensor(); Pin for Button
InterruptIn buttonControlAlarm;  // buttonTriggerSensor(); Pin for Button
DigitalOut speaker;              // speaker() Pins for Digital Out
// Serial pc(USBTX, USBRX);      pc() Pins for Serial

// Important variables for controller
bool alarmEnabled = false;
int alarmHour = 7;
int alarmMinute = 30;

const char *locationAPI = "http://ip-api.com/json";
struct LocationData
{
    time_t unixTime;
    double latiTude, longiTude;
    string city;
};

int handlerGetTime(int chooseTimeType)
{
    int returnedTime = 0;
    time_t timeNow = time(NULL);
    struct tm *infoOfTime = localtime(&timeNow);

    switch (chooseTimeType)
    {
    case 1:
        returnedTime = infoOfTime->tm_hour;
        break;

    case 2:
        returnedTime = infoOfTime->tm_min;
        break;

    case 3:
        returnedTime = infoOfTime->tm_sec;
        break;

    default:
        return 0;
        break;
    }

    return returnedTime;
}

void handlerEnableAlarm()
{
    alarmEnabled = true;
    speaker = 1;
}

void handlerDisableAlarm()
{
    alarmEnabled = false;
    speaker = 0;
}

void handlerUpdateDisplay()
{
    oledDisplay.clearDisplay();
    oledDisplay.printf("%02d:%02d", alarmHour, alarmMinute);
    oledDisplay.display();
}

void handlerLocationDisplay(const char *unixTime, const char *latiTude, const char *longiTude, const char *city)
{
    oledDisplay.clearDisplay();

    oledDisplay.setTextCursor(0, 0);
    oledDisplay.printf("UNIX Time: %s", unixTime);
    wait_us(2000000);

    oledDisplay.clearDisplay();
    oledDisplay.setTextCursor(0, 0);
    oledDisplay.printf("Latitude: %s", latiTude);
    oledDisplay.setTextCursor(0, 2);
    oledDisplay.printf("Longitude: %s", longiTude);
    wait_us(2000000);

    oledDisplay.setTextCursor(0, 0);
    oledDisplay.printf("City: %s", city);
    wait_us(2000000);

    oledDisplay.clearDisplay();
    oledDisplay.display();
}

// Ex 2, Ex 7
void handlerButtonTimeDisplay()
{
    int month, day, hour, minute, weekDay;

    while (1)
    {
        time_t timeNow = time(NULL);
        struct tm *infoOfTime = localtime(&timeNow);

        month = infoOfTime->tm_mon + 1;
        day = infoOfTime->tm_mday;
        hour = infoOfTime->tm_hour;
        minute = infoOfTime->tm_min;
        weekDay = infoOfTime->tm_wday;

        oledDisplay.clearDisplay();

        oledDisplay.setTextSize(2);
        oledDisplay.setTextColor(WHITE);
        oledDisplay.setTextCursor(0, 0);
        oledDisplay.printf("%02d-%d-%02d", day, weekDay, month);
        oledDisplay.setTextCursor(0, 20);
        if (alarmEnabled)
        {
            oledDisplay.printf("%02d:%02d Alarm ON", hour, minute);
        }
        else
        {
            oledDisplay.printf("%02d:%02d Alarm OFF", hour, minute);
        }
        oledDisplay.setTextCursor(0, 40);

        oledDisplay.display();

        wait_us(1000);
    }
}

// Ex 3
void handlerButtonSettingsAlarm()
{
    oledDisplay.clearDisplay();
    oledDisplay.printf("Set time for Alarm\n");
    oledDisplay.printf("Press button for control and set alarm time!\n");
    oledDisplay.display();

    while (buttonAlarmSettings)
    {
        wait_us(50000);
    }

    while (!buttonAlarmSettings)
    {
        alarmHour = 24 % (alarmHour + 1);
        handlerUpdateDisplay();
        wait_us(200000);
    }
    wait_us(50000);

    while (buttonAlarmSettings)
    {
        wait_us(50000);
    }

    while (!buttonAlarmSettings)
    {
        alarmMinute = 60 % (alarmMinute + 1);
        handlerUpdateDisplay();
        wait_us(200000);
    }
    wait_us(50000);

    oledDisplay.clearDisplay();
    oledDisplay.printf("Time for alarm is set!");
    oledDisplay.display();

    while (buttonAlarmSettings)
        ;
}

// Ex 4
void handlerAlarmSignal()
{
    Timer timerAlarm;
    timerAlarm.start();
    handlerEnableAlarm();

    while (timerAlarm.read_ms() < 600000)
    {
    }

    handlerDisableAlarm();
    timerAlarm.stop();
    timerAlarm.reset();
}

// Ex 5
void handlerRepeatAlarmSpeaker()
{
    handlerDisableAlarm();

    Timer timerAlarm;
    timerAlarm.start();

    while (timerAlarm.read_ms() < 300000)
    {
    }

    handlerAlarmSignal();
    timerAlarm.stop();
    timerAlarm.reset();
}

// Ex 6, Ex 7
void handlerControlAlarm()
{
    if (alarmEnabled)
    {
        handlerDisableAlarm();
    }
}

// Ex 8
void handlerSensorOutput()
{
    float sensorTemperature, sensorHumidity;

    while (1)
    {
        sensorHumidity = dht.getHumidity();
        sensorTemperature = dht.getTemperature();

        oledDisplay.clearDisplay();
        oledDisplay.setTextSize(2);
        oledDisplay.setTextColor(WHITE);
        oledDisplay.setTextCursor(0, 0);
        oledDisplay.printf("T: %.1fC", sensorTemperature);
        oledDisplay.setTextCursor(0, 30);
        oledDisplay.printf("H: %.1f%%", sensorHumidity);
        oledDisplay.display();

        wait_us(1000);
    }
}

// Ex 10

int main()
{
    oledDisplay.begin();
    oledDisplay.clearDisplay();
    oledDisplay.display();

    while (1)
    {
        buttonTime.fall(&handlerButtonTimeDisplay);
        buttonTriggerAlarm.fall(&handlerButtonSettingsAlarm);
        buttonRepeatAlarm.fall(&handlerRepeatAlarmSpeaker);
        buttonMuteAlarm.fall(&handlerControlAlarm);
        buttonTriggerSensor.fall(&handlerSensorOutput);
        buttonControlAlarm.fall(&handlerControlAlarm);

        if (handlerGetTime(1) == alarmHour && handlerGetTime(2) == alarmMinute && alarmEnabled)
        {
            handlerAlarmSignal();
            handlerUpdateDisplay();
            wait_us(50000);
        }

        wait_us(1000);
    }
}