#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <FreeRTOS_AVR.h>
#include <Servo.h>

Servo myservo;
SoftwareSerial mySerial(2, 3);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

void setup()  
{
    Serial.begin(9600);
    while (!Serial);  // for Leonardo/Micro/Zero
  
    finger.begin(57600);
    Serial.println("esp 시작");
    pinMode (6, OUTPUT); // servopin = 6
    myservo.attach(6);
  
    xTaskCreate(servo, "servo", 512, NULL, 1, NULL);
    xTaskCreate(fingerprintTask, "Fingerprint", 512, NULL, 2, NULL);

    vTaskStartScheduler(); // 스케줄러 시작
}

int Usercode;

void servo(void *pvParameters){
  while(1){
    if(Usercode == 1){
      Serial.println("1번 모터 작동");
      Serial.write(Usercode);
      myservo.write(30);
      vTaskDelay(4000 / portTICK_PERIOD_MS); // 주기적 실행을 위한 딜레이
    }
    else if(Usercode == 2){
      Serial.println("2번 모터 작동");
      Serial.write(Usercode);
      myservo.write(150);
      vTaskDelay(4000 / portTICK_PERIOD_MS); // 주기적 실행을 위한 딜레이
    }
    else{
    }
    // Serial.println("hello");
    // vTaskDelay(4000 / portTICK_PERIOD_MS);
  }
}

void fingerprintTask(void *pvParameters) {
  while(1) {
    int id = finger.getImage();
    if (id == FINGERPRINT_OK) {
      id = finger.image2Tz();
      if (id == FINGERPRINT_OK) {
        id = finger.fingerFastSearch();
        if (id == FINGERPRINT_OK) {
          Usercode = (int)finger.fingerID;
          Serial.println((int)finger.fingerID);
          vTaskDelay(4000 / portTICK_PERIOD_MS); // 인식 결과 4초 유지
        } else {
        }
      } else {
      }
    } else {
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS); // 주기적 실행을 위한 딜레이
  }
}

void loop()                     
{
//    int id = finger.getImage();
//    if (id == FINGERPRINT_OK) {
//      id = finger.image2Tz();
//      if (id == FINGERPRINT_OK) {
//        id = finger.fingerFastSearch();
//        if (id == FINGERPRINT_OK) {
//          UserCode = (String)finger.fingerID
//          Serial.println(UserCode);
//          /vTaskDelay(4000 / portTICK_PERIOD_MS); // 인식 결과 4초 유지
//        } else {
//        }
//      } else {
//      }
//    } else {
//    }
}
