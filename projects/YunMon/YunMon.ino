/*
Arduino Yun - Power and temperature measurement

1) Interrupt-driven light pulse detection on IRQ 0 and 1
- PV Produktion: pin3 (Yun) = IRQ 0
- Zweirichtungszaehler: pin2 (Yun) = IRQ 1
- calculate power based on ppwh pulses per Wh

2) OneWire temperature measurement via Dallas DS18B20

3) Plot averaged powers to plot.ly as streaming plot

4) Log data to /mnt/sda1/datalogs: timestamp, production [W], abs(production - consumption) [W], temperature [°C]


Possible improvements / backlog:
- check power calculations for pathological cases (zero difference etc)


History:

141229 change of dev env to Win158, and moved to git

140927 Merged with DS18B20 temperature measurement - on port 4, writing to col 4

140921 Move from appending to one single datafile datalog.txt to daily datalog files

140912  timing approach adjusted: timestamp with 1s precision is logged, looping until measurementinterval with
ms precision has passed by. Should be ok for intervals >= 10s. Not ok for sub-second measurements.

140911: data logging to SD card added
*/

// plot.ly streaming tokens, gitignored, same directory
#include "mytokens.h"

#include <Bridge.h>
#include <Console.h>

#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 4 on the Arduino
#define ONE_WIRE_BUS 4

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// Data logging and time stamping from http://arduino.cc/en/Tutorial/YunDatalogger
#include <FileIO.h>
unsigned long millis_current;
String fileName, timeStamp, dataString;
char __fileName[34] = "/mnt/sda1/datalogs/YYYY-MM-DD.txt";  // value as pattern only, to get size right

#include <PlotlyYun.h>
#include <YunMessenger.h>

// Initialize plotly "plotters" with your stream tokens.
// Find your stream tokens in your plotly account here: https://plot.ly/settings
// Initialize as many plotters as you want! Each plotter will send data to the same plot as a separate line.
// Make sure that you place these exact-same stream tokens in your `config.json` file on the Linino.

plotly plotter1(PLOTLY_TOKEN_1); plotly plotter2(PLOTLY_TOKEN_2); // plot.ly tokens

// Measurement interval in milliseconds
int measurementInterval = 10000;

//Number of pulses per wh - found or set on the meter. 10000 pulses/kwh = 10 pulses per wh
int ppwh0 = 10; int ppwh1 = 10;

// Number of pulses, incl. averaged pulsed, to measure energy
long pulseCount0 = 0; long pulseCount1 = 0; long averageCount0 = 0; long averageCount1 = 0;

//Used to measure power
unsigned long pulseTime0, accumulatedPulseTime0, lastTime0, deltaTime0, pulseTime1, accumulatedPulseTime1, lastTime1, deltaTime1;

//power and energy
double power0, averagedPower0, power1, averagedPower1;

void setup()
{

    // start-up the bridge
    Bridge.begin();
    FileSystem.begin();

    delay(2000);
    Console.begin();
    while (!Console)
    {
        ; // wait for Console port to connect.
    }

    Console.buffer(64);
    delay(2000);

    // attach interrupt routines
    attachInterrupt(0, onPulse0, FALLING); attachInterrupt(1, onPulse1, FALLING);

    // Start up the OneWire library
    sensors.begin();

    plotter1.timezone  = "Europe/Berlin"; plotter2.timezone = "Europe/Berlin";
    millis_current = millis();

}

void loop()
{
    if (millis() - millis_current > measurementInterval)
    {

        // reset timer
        millis_current = millis();

        sensors.requestTemperatures(); // Send the command to get temperatures

        // Target: YY-MM-DD hh:mm:ss, elapsed Millis, analogValues
        dataString = getTimeStamp() + ",";
        dataString += String(averagedPower0); dataString += ",";
        dataString += String(averagedPower1); dataString += ",";
        dataString += String(sensors.getTempCByIndex(0));

        // open the file. note that only one file can be open at a time,
        // so you have to close this one before opening another.
        // The FileSystem card is mounted at the following "/mnt/sda1"
        fileName = "/mnt/sda1/datalogs/" + getDate() + ".txt";
        // convert fileName string to char array
        fileName.toCharArray(__fileName, sizeof(__fileName));
        File dataFile = FileSystem.open(__fileName, FILE_APPEND);

        // if the file is available, write to it:
        if (dataFile)
        {
            dataFile.println(dataString);
            dataFile.close();
            // print to the serial port too:
            Console.println(dataString);
        }
        // if the file isn't open, pop up an error:
        else
        {
            Console.println("error opening datalog.txt");
        }

        //Plot the value
        plotter1.plot(averagedPower0); // Solar production
        plotter2.plot(averagedPower1); // Zweirichtungszaehler

        //Reset averaging
        averageCount0 = 0; accumulatedPulseTime0 = 0;
        averageCount1 = 0; accumulatedPulseTime1 = 0;
    }
}

//Interrupt routine 0 - PV production
void onPulse0()
{
    //pulseCounters
    pulseCount0++; averageCount0++;

    //used to measure time between pulses.
    lastTime0 = pulseTime0;
    pulseTime0 = micros();
    deltaTime0 = pulseTime0 - lastTime0;
    accumulatedPulseTime0 += deltaTime0;

    //Calculate power
    power0 = 3600000000.0 / (deltaTime0 * ppwh0);
    averagedPower0 = (3600000000.0 * averageCount0) / (accumulatedPulseTime0 * ppwh0);
    //  }
}

//Interrupt routine 1 - Zweirichtungszaehler
void onPulse1()
{
    //pulseCounters
    pulseCount1++; averageCount1++;

    //used to measure time between pulses.
    lastTime1 = pulseTime1;
    pulseTime1 = micros();
    deltaTime1 = pulseTime1 - lastTime1;
    accumulatedPulseTime1 += deltaTime1;

    //Calculate power
    power1 = 3600000000.0 / (deltaTime1 * ppwh1);
    averagedPower1 = (3600000000.0 * averageCount1) / (accumulatedPulseTime1 * ppwh1);
    //  }
}

// This function returns a string with the current date
String getDate()
{
    String result;
    Process time;
    // date is a command line utility to get the date and the time
    // in different formats depending on the additional parameter
    time.begin("date");
    time.addParameter("+%Y-%m-%d");  // parameters: D for the complete date mm/dd/yy, T for the time hh:mm:ss
    time.run();  // run the command

    // read the output of the command
    while (time.available() > 0)
    {
        char c = time.read();
        if (c != '\n')
            result += c;
    }

    return result;
}

// This function returns a string with the time stamp
String getTimeStamp()
{
    String result;
    Process time;
    // date is a command line utility to get the date and the time
    // in different formats depending on the additional parameter
    time.begin("date");
    time.addParameter("+%Y-%m-%d %T");  // parameters: D for the complete date mm/dd/yy, T for the time hh:mm:ss
    time.run();  // run the command

    // read the output of the command
    while (time.available() > 0)
    {
        char c = time.read();
        if (c != '\n')
            result += c;
    }

    return result;
}

