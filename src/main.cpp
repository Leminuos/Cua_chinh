/*
Arduino Uno R3 (Cửa chính):
- LCD ngoài + keypad 4x4 + buzzer
- Cửa chính: 1 servo + 1 button
*/

// Khai báo thư viện
#include <Arduino.h>
#include <Keypad.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

// Khai báo các chân
const byte pinBuzzer = 5;      // Cấu hình chân cho buzzer
const byte pinServo = 4;       // Cấu hình chân cho servo
const byte buttonDoor = 3;     // Cấu hình chân cho nút nhấn mở cửa

// Khai báo các biến
byte cursorCollums = 0;  // Đặt con trỏ lcd
byte input_Index=0;     //Chỉ số vị trí của ký tự trong mật khẩu khi nhập 
byte pass_Index=0;     // Chỉ số vị trí của ký tự trong mật khẩu đúng
const byte limitIncorrect = 3;  // Giới hạn nhập sai mật khẩu
byte countIncorrect = 0;  // Đếm số lần nhập sai mật khẩu
unsigned long currentTime, previousTime = 0;  // Khai báo thời gian hiện tại và thời gian trước đó
byte countDown = 60;    // Hiện thị thời gian đếm ngược được phép nhập 
byte pos = 0;          // Lưu giá trị góc của servo
char data_receive = 0;     // Dữ liệu nhận 
char data_send = 0;        // Dữ liệu gửi

// Khai báo các biến trạng thái
boolean isNotAllow = false; // Trả về false thì được phép nhập, true thì không được phép
boolean isEnable = false;
boolean state_Door = 0; // Trạng thái cửa chính

//Mật khẩu
char pass[4];           // Mảng tạm dùng để lưu trữ ký tự khi nhập
char initial_pass[4];   // Mật khẩu ban đầu được lưu trong EEPROM
char current_pass[4];   // Mật khẩu dùng lưu trữ để thay đổi mật khẩu

//Keypad
const byte rows = 4;  // Khai báo hàng
const byte cols = 4;  // Khai báo cột
char keys[rows][cols]=    // Mảng hai chiều lưu trữ các ký tự trên keypad 4x4
{
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};                      
byte rowsPin[rows]={6, 7, 8, 9};       // Cấu hình chân cho các hàng keypad 4x4
byte colsPin[cols]={10, 11, 12, 13};    // Cấu hình chân cho các cột keypad 4x4

LiquidCrystal_I2C lcd(0x27, 16, 2);                                        // Khởi tạo lcd kết nối với module i2c
Keypad keypad = Keypad(makeKeymap(keys), rowsPin, colsPin, rows, cols);    // Khởi tạo keypad
Servo myServo;

void beep(unsigned long t);
void readPasswordSetup();

void setup(){
  Serial.begin(9600);
  Serial.flush();
  // Đặt mật khẩu lưu vào EEPROM
  //for(int j=0;j<4;j++){
  //EEPROM.write(j, j+49);}
  lcd.begin(16, 2);
  lcd.backlight();  
  readPasswordSetup();
  lcd.print(" Enter Password:");
  pinMode(pinBuzzer, OUTPUT);  // Cấu hình đầu ra cho chân buzzer
  digitalWrite(pinBuzzer, LOW); // Đặt đầu ra này là mức 0
  myServo.attach(pinServo);
  myServo.write(pos);
}

void loop() {
  // if(Serial.available()){
  //   data_receive = Serial.read();
  //   switch(data_receive){
  //     case '0': // Đóng cửa
  //       state_Door = 0;
  //       isEnable = 1;
  //       break;
  //     case '1':
  //       state_Door = 1;
  //       isEnable = 1;
  //       break;
  //   }
  // }
  char key = keypad.getKey(); // Ký tự nhập vào sẽ gán cho biến Key
  if (key && isNotAllow == false){
    beep(50);
    pass[input_Index]=key;
    cursorCollums = input_Index + 6;
    lcd.setCursor(cursorCollums, 1);
    lcd.print("*");
    input_Index++;  // Tăng chỉ số nhập vào từ bàn phím lên 1
    if(key == 'A'){
      lcd.clear();
      lcd.print(" Enter Password:");
      input_Index=0;  // Thiết lập lại chỉ số về 0
    }
  }
  if (input_Index == 4){
    if (strncmp(pass, initial_pass, 4) == 0) // Nếu mảng mật khẩu nhập từ bàn phím bằng mật khẩu ban đầu
    {
      state_Door = 1;
      isEnable = 1;
      input_Index = 0;
      isNotAllow = true; // Không cho phép nhập từ bàn phím
      data_send = '1';
      Serial.write(data_send);
    }else{
      lcd.clear();
      beep(500);
      delay(50);
      beep(500);
      lcd.print(" Enter Password");
      input_Index = 0;
      countIncorrect++;
      isNotAllow = false; // Cho phép nhập từ bàn phím
      currentTime = millis(); // Lưu thời gian hiện tại
      if (currentTime - previousTime >= 30000) { // Nếu đã trôi qua 30 giây
        previousTime = currentTime; // Lưu lại thời gian cũ
        countIncorrect = 0; // Nếu sau 30 giây không nhập, reset số lần nhập sai về mặc định là 0
      }
    }
  }
  if(countIncorrect == limitIncorrect){
    isNotAllow = true;
    lcd.clear();
    lcd.print(" Remaining time:");
    lcd.setCursor(7, 1);
    lcd.print(countDown);
    delay(1000);
    countDown--;
    if (countDown == 0){
      lcd.clear();
      delay(50);
      lcd.print(" Enter Password:");
      countDown = 60; // Nếu thời gian đếm ngược đạt đến 0, thì reset lại về 30s
      countIncorrect = 0; // Nếu thời gian đếm ngược đạt đến 0, cho phép nhập tiếp mật khẩu
      isNotAllow = false;
    }
  }
  if(isNotAllow == true && key == '#') // Nếu nhấn phím '#' thì đóng cửa
  { 
    state_Door = 0;
    isEnable = 1;
    lcd.clear();
    lcd.print(" Enter Password:");
    input_Index = 0;
    isNotAllow = false;
    data_send = '0';
    Serial.write(data_send);
  }
  if(key == '*' && isNotAllow == true) // Nhấn phím '*' để setup
  {
    lcd.clear();
    lcd.print("Current Password:");
    while(pass_Index < 4){
      char key = keypad.getKey();
      if(key){
        beep(50);
        current_pass[pass_Index]= key;
        cursorCollums = pass_Index + 6;
        lcd.setCursor(cursorCollums, 1);
        lcd.print("*");
        pass_Index++;
        if(key == 'A'){
          lcd.clear();
          lcd.print("Current Password:");
          pass_Index=0;}
        if(key == 'B'){
          pass_Index=5;}
      }
    }
    if(strncmp(current_pass, initial_pass, 4) == 0){
      pass_Index=0;
      lcd.clear();
      lcd.print("New Password:");
      lcd.setCursor(0,1);
      while(pass_Index<4){
        char key=keypad.getKey();
        if(key){
          beep(50);
          cursorCollums = pass_Index + 6;
          initial_pass[pass_Index]=key;
          lcd.setCursor(cursorCollums,1);
          lcd.print(key);
          delay(500);
          lcd.setCursor(cursorCollums,1);
          lcd.print("*");
          EEPROM.write(pass_Index,key);
          pass_Index++;
          isNotAllow = false;
        }
      }
      lcd.clear();
      lcd.print(" Pass Changed");
      delay(1000);
      lcd.clear();
      pass_Index = 0;
      lcd.print(" Enter Password:");
    }else if(pass_Index == 5){
      lcd.clear();
      lcd.print("   Exiting...");
      delay(2000);
      lcd.clear();
      lcd.print(" Enter Password:");
      input_Index = 0;
      isNotAllow = false;
      pass_Index = 0;
    }else{
      lcd.clear();
      beep(500);
      delay(50);
      beep(500);
      lcd.clear();
      lcd.print("    Enter *");
      lcd.setCursor(0,1);
      lcd.print("  to Try Again");
      isNotAllow = true;
      pass_Index = 0;
      countIncorrect++;
    }
  }
  if(buttonDoor == 0){
    while(buttonDoor == 0){
      if(state_Door == 0){  // Nếu cửa đang đóng thì mở cửa ra
        state_Door = 1;
        isEnable = 1;
      }else{
        state_Door = 0;
        isEnable = 1;
      }
    }
  }
  if(state_Door == 1 && isEnable == 1){   // Nếu nhận tín hiệu mở cửa
    lcd.clear();
    lcd.print(" Welcome Home!");
    while(pos < 90){
      pos = pos + 2;
      myServo.write(pos);
      delay(15);
    }
    isEnable = 0;
    isNotAllow = true;
  }
  if(state_Door == 0 && isEnable == 1){   // Nếu nhận tín hiệu đóng cửa
    while(pos > 0){ 
      pos = pos - 2;
      myServo.write(pos);
      delay(15);
    }
    isEnable = 0;
    lcd.clear();
    lcd.print(" Enter Password:");
    isNotAllow =  false;
  }
}
//-------------------------------------------------------------
void beep(unsigned long t){
  digitalWrite(pinBuzzer,HIGH); // Bật còi
  delay(t);                     // Kêu thời gian t
  digitalWrite(pinBuzzer,LOW);   // Tắt còi
  return;
}
// Hàm hiển thị mật khẩu được lưu trong EEPROM lên serial monitor
void readPasswordSetup(){
  Serial.println("Pass word:");
  for(int j=0; j<4; j++){
    Serial.print((char)EEPROM.read(j)); Serial.print(" ");
    initial_pass[j] = EEPROM.read(j);  // Lưu mật khẩu trong EEPROM vào mật khẩu ban đầu
  }
  return;
}