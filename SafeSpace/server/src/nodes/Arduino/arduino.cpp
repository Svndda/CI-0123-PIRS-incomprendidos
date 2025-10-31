// // Arduino code to read temperature and humidity from analog pins A0 and A1
// // and output the readings in JSON format via Serial.
// // No need to be compile here, just upload to your Arduino board.
/*
#include <Wire.h>
#include <Adafruit_BMP085.h>
#define seaLevelPressure_hPa 1013.25
// Incluimos librería
#include <DHT.h>
 
// Definimos el pin digital donde se conecta el sensor
#define DHTPIN 2
// Dependiendo del tipo de sensor
#define DHTTYPE DHT11
 
// Inicializamos el sensor DHT11
DHT dht(DHTPIN, DHTTYPE);

Adafruit_BMP085 bmp;

const int trigPin = 9;  
const int echoPin = 10; 

float duration, distance;  

void setup() {

  pinMode(trigPin, OUTPUT);  
	pinMode(echoPin, INPUT);  

  Serial.begin(9600);
  dht.begin();
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    while (1) {}
  }
}

void loop() {

  digitalWrite(trigPin, LOW);  
	delayMicroseconds(2);  
	digitalWrite(trigPin, HIGH);  
	delayMicroseconds(10);  
	digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);  
  distance = (duration*.0343)/2;  

  Serial.print("Distance: ");  
	Serial.println(distance);

  // Leemos la humedad relativa
  float h = dht.readHumidity();
 
  // Comprobamos si ha habido algún error en la lectura
  if (isnan(h)) {
    Serial.println("Error obteniendo los datos del sensor DHT11");
    return;
  }

  Serial.print("Humedad: ");
  Serial.println(h);


  Serial.print("Temperature = ");
  Serial.print(bmp.readTemperature());
  Serial.println(" *C");

  Serial.print("Pressure = ");
  Serial.print(bmp.readPressure());
  Serial.println(" Pa");

  Serial.print("Altitude = ");
  Serial.print(bmp.readAltitude());
  Serial.println(" meters");

  Serial.print("Pressure at sealevel (calculated) = ");
  Serial.print(bmp.readSealevelPressure());
  Serial.println(" Pa");

  Serial.print("Real altitude = ");
  Serial.print(bmp.readAltitude(seaLevelPressure_hPa * 100));
  Serial.println(" meters");

  Serial.println();
  delay(500);
}
  */