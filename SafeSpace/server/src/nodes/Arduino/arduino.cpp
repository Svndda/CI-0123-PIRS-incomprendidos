// Arduino code to read temperature and humidity from analog pins A0 and A1
// and output the readings in JSON format via Serial.
// No need to be compile here, just upload to your Arduino board.

void setup() {
  Serial.begin(9600);
}

void loop() {
  int a0 = analogRead(A0);
  int a1 = analogRead(A1);

  float temp = a0 * (5.0 / 1023.0) * 100.0;
  float hum  = a1 * (5.0 / 1023.0) * 100.0;

  Serial.print("{\"temp\":");
  Serial.print(temp, 2);
  Serial.print(",\"humidity\":");
  Serial.print(hum, 2);
  Serial.println("}");

  delay(2000);
}