int data;

void setup() {
  Serial.begin(9600);
  pinMode(13,OUTPUT);
}

void loop() {
  if(Serial.available())
  {
    data = Serial.read();
  }
  if(data == 1)
  {
    Serial.println("김병찬");
  }
  else if(data ==2)
  {
    Serial.println("일론머스크"); 
  }
}
