#include <DHT.h>
#define DHTPIN 2
#define DHTTYPE DHT11

#include <CurieBLE.h>
#include <CurieTimerOne.h>
#include <SPI.h>
#include <SD.h>

// Bluetooth Low Energy
BLEPeripheral blePeripheral;
BLEService sensorService("181A");
BLEUnsignedLongCharacteristic sensorOutput("2A7A", BLERead | BLENotify);

// SD Card
File file;

int output = 0;
int startMillis = 0;
int prevMillis = 0;
int outputArray[72];

// Pins
int photocell = 0;
int photocell2 = 2;
int temp = 1;
int water = 3;
int soil = 4;
int soilV = 8;
int waterV = 7;

int count;
int avgCount;
int topCount;

bool isMeasuring;

DHT dht(DHTPIN, DHTTYPE);

void setup() 
{
  Serial.begin(9600);

  pinMode(soilV, OUTPUT);
  
  blePeripheral.setLocalName("Plantr");
  blePeripheral.setAdvertisedServiceUuid(sensorService.uuid());
  
  blePeripheral.addAttribute(sensorService);
  blePeripheral.addAttribute(sensorOutput);

  sensorOutput.setValue(output);
  
  blePeripheral.begin();

  if (!SD.begin(4))
  {
    return;
  }
  
  getSDData();

  dht.begin();

  count = 0;

  isMeasuring = false;

  measureSensor();
  CurieTimerOne.start(3600000, &measureSensor);
}

void loop() 
{
  BLECentral central = blePeripheral.central();

  if (central) 
  {
    startMillis = millis();
    while (central.connected()) 
    {
      if (millis() - startMillis >= 2000) 
      {
        while(isMeasuring){}
        readSensorOutputs();
      }
    }
  }
}

int photocellArray[10];
int humidArray[10];
int tempArray[10];
int waterArray[10];
int soilArray[10];

int photocellAvgArray[10];
int humidAvgArray[10];
int tempAvgArray[10];
int waterAvgArray[10];
int soilAvgArray[10];

int photocellTopArray[10];
int humidTopArray[10];
int tempTopArray[10];
int waterTopArray[10];
int soilTopArray[10];

void measureSensor() 
{
  isMeasuring = true;
  
  if (count == 10) 
  {
    count = 0;

    if (avgCount == 10) 
    {
      avgCount = 0;

      if (topCount == 10)
      {
        topCount = 0;
        
        output = getSoil(average(soilTopArray))
                  + getWater(average(waterTopArray))
                  + getTemp(average(tempTopArray))
                  + getHumid(average(humidTopArray))
                  + getLight(average(photocellTopArray));
        
        shiftArray(output);
        writeSensorOutput(output);
        
        Serial.println(" - OUTPUT - ");
        Serial.println(output);
        Serial.println(" - ");
        Serial.print("Light: ");
        Serial.println(average(photocellTopArray));
        Serial.print("Humidity: ");
        Serial.println(average(humidTopArray));
        Serial.print("Temperature: ");
        Serial.println(average(tempTopArray));
        Serial.print("Water: ");
        Serial.println(average(waterTopArray));
        Serial.print("Soil: ");
        Serial.println(average(soilTopArray));
        Serial.println("----------------");
      }
      
      photocellTopArray[topCount] = average(photocellAvgArray);
      humidTopArray[topCount] = average(humidAvgArray);
      tempTopArray[topCount] = average(tempAvgArray);
      waterTopArray[topCount] = average(waterAvgArray);
      soilTopArray[topCount] = average(soilAvgArray);

      Serial.print(" - AVERAGE << #");
      Serial.print(topCount);
      Serial.println(" - ");
      Serial.print("Light: ");
      Serial.println(photocellTopArray[topCount]);
      Serial.print("Humidity: ");
      Serial.println(humidTopArray[topCount]);
      Serial.print("Temperature: ");
      Serial.println(tempTopArray[topCount]);
      Serial.print("Water: ");
      Serial.println(waterTopArray[topCount]);
      Serial.print("Soil: ");
      Serial.println(soilTopArray[topCount]);
      Serial.println("----------------");
            
      topCount++;
    } 
    
    photocellAvgArray[avgCount] = average(photocellArray);
    humidAvgArray[avgCount] = average(humidArray);
    tempAvgArray[avgCount] = average(tempArray);
    waterAvgArray[avgCount] = average(waterArray);
    soilAvgArray[avgCount] = average(soilArray);

    /*
    Serial.print(" - AVERAGE #");
    Serial.print(avgCount);
    Serial.println(" - ");
    Serial.print("Light: ");
    Serial.println(photocellAvgArray[avgCount]);
    Serial.print("Humidity: ");
    Serial.println(humidAvgArray[avgCount]);
    Serial.print("Temperature: ");
    Serial.println(tempAvgArray[avgCount]);
    Serial.print("Water: ");
    Serial.println(waterAvgArray[avgCount]);
    Serial.print("Soil: ");
    Serial.println(soilAvgArray[avgCount]);
    Serial.println("----------------");
    */

    avgCount++;
  }

  photocellArray[count] = (analogRead(photocell)+analogRead(photocell2))/2;
  humidArray[count] = dht.readHumidity();
  int dhtTempReading = dht.readTemperature();
  int lmTempReading = (analogRead(temp)*330)/1024;
  tempArray[count] = (dhtTempReading+lmTempReading)/2;
  digitalWrite(soilV, HIGH);
  digitalWrite(waterV, HIGH);
  delay(100);
  soilArray[count] = analogRead(soil);
  waterArray[count] = analogRead(water);
  digitalWrite(soilV, LOW);
  digitalWrite(waterV, LOW);

  /*
  Serial.print(" - MEASURE #");
  Serial.print(count);
  Serial.println(" - ");
  Serial.print("Light: ");
  Serial.println(photocellArray[count]);
  Serial.print("Humidity: ");
  Serial.println(humidArray[count]);
  Serial.print("Temperature: ");
  Serial.println(tempArray[count]);
  Serial.print("Water: ");
  Serial.println(waterArray[count]);
  Serial.print("Soil: ");
  Serial.println(soilArray[count]);
  Serial.println("----------------");
  */
  
  count++;

  isMeasuring = false;
}

int average(int arr[]){
  int total = 0;
  for (int i = 0; i < 10; i++) 
  {
    total += arr[i];
  }
  return total / 10;
}

void readSensorOutputs() 
{
  for (int i = 0; i < 72; i++) 
  {
    sensorOutput.setValue(outputArray[i]);
  }
}

void getSDData(){
  file = SD.open("sensor.log");
  if (file)
  {
    int fileSize = file.size();
    if (fileSize > 360) {
      file.seek(fileSize - 360);
    }
    while (file.available()) {
      char c[5];
      c[0] = file.read();
      c[1] = file.read();
      c[2] = file.read();
      c[3] = file.read();
      c[4] = file.read();
      shiftArray(atoi(c));
    }
  } else {
    Serial.println("Could not read SD");
  }
  file.close();
}

void writeSensorOutput(int in) 
{
  file = SD.open("sensor.log", FILE_WRITE);
  if (file) 
  {
    file.print(in);
  }
  file.close();
}

void shiftArray(int in) 
{
  for (int i = 72; i > 0; i--)
  {
    outputArray[i] = outputArray[i-1];
  }
  outputArray[0] = in;
}

int getSoil(int soil) {
  if (soil < 150) {
    return 10000; // extreme wet
  } else if (soil < 200) {
    return 20000; // wet
  } else if (soil < 300) {
    return 30000; // normal wet
  } else if (soil < 400) {
    return 40000; // normal
  } else if (soil < 575) {
    return 50000; // normal dry
  } else if (soil < 650) {
    return 60000; // light dry
  } else if (soil < 750) {
    return 70000; // dry
  } else if (soil < 850) {
    return 80000; // very dry
  } else {
    return 90000; // extreme dry
  }
}

int getWater(int water) {
  if (water < 20) {
    return 1000;
  } else if (water < 100) {
    return 2000;
  } else {
    return 9000;
  }
}

int getTemp(int temp) {
  if (temp < 1) {
    return 100; // freezing
  } else if (temp < 5) {
    return 200; // very cold
  } else if (temp < 10) {
    return 300; // cold
  } else if (temp < 20) {
    return 400; // okay
  } else if (temp < 29) {
    return 500; // nice
  } else if (temp < 35) {
    return 600; // hot
  } else {
    return 700; // burning
  }
}

int getHumid(int humid) {
  if (humid < 10) {
    return 10; // very dry
  } else if (humid < 25) {
    return 20; // dry
  } else if (humid < 40) {
    return 30; // normal
  } else if (humid < 65) {
    return 40; // great
  } else if (humid < 80) {
    return 50; // wet
  } else if (humid < 90) {
    return 60; // very wet
  } else {
    return 70; // extreme wet
  }
}

int getLight(int light) {
  if (light < 100) {
    return 1; // dark
  } else if (light < 300) {
    return 2; // dim
  } else if (light < 600) {
    return 3; // light
  } else if (light < 800) {
    return 4; // bright
  } else {
    return 5; // very bright
  }
}


