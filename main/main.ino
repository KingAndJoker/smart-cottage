#include <SoftwareSerial.h>
#define ARRAY_SIZE(x)  (sizeof((x)) / sizeof((x)[0]))
#define PHONE_SIZE ARRAY_SIZE(phone)

#define BUTTON_HEATING_ON 5
#define BUTTON_HEATING_OFF 6
#define HEATING_OUT 12
#define DEBUG 1
SoftwareSerial SIM800(3, 2);

String phone[] = {"+XXXXXXXXXXXX"};
int heating_status = 0;
// Тест
void setup() {
  Serial.begin(9600);
  Serial.println("Start!");
  SIM800.begin(9600);
  sendATCommand("AT", true);
  delay(3000);
  sendATCommand("AT+CMGF=1", true);
  delay(3000);
  pinMode(HEATING_OUT, OUTPUT);
  pinMode(BUTTON_HEATING_ON, INPUT);
  pinMode(BUTTON_HEATING_OFF, INPUT);
  all_off(false);


  delay(3000);
  send_message_all_number("Arduino on. Heating elements are disabled.");
}

void all_off(bool send_msg) {
  heating(0, send_msg, -1);
}

String sendATCommand(String cmd, bool waiting) {
  String _resp = "";
  Serial.println(cmd);
  SIM800.println(cmd);
  if (waiting) {
    _resp = waitResponse();
    Serial.println(_resp);
  }
  return _resp;
}

String waitResponse() {
  String _resp = "";
  int count = 0;
  while (!SIM800.available() && count < 30)  {
    delay(100);
    count++;
  }

  if (SIM800.available()) {
    _resp = SIM800.readString();
  }
  else {
    Serial.println("Timeout...");
  }
  return _resp;
}

void home_status(int phone_index) {
  String msg = "home status\n";
  msg += "heating: ";
  if (heating_status == 1) {
    msg += "on\n";
  }
  if (heating_status == 0) {
    msg += "off\n";
  }

  if (phone_index > -1) {
    send_message_number(msg, phone[phone_index]);
  } else {
    send_message_all_number(msg);
  }
}

void heating(int cmd, bool send_msg, int phone_index) {
  if (cmd == 1) {                          // �������� ������
    digitalWrite(HEATING_OUT, HIGH);
    heating_status = 1;
    if (send_msg) {
      if (phone_index > -1) {
        send_message_number("Heater on.", phone[phone_index]);
      } else {
        send_message_all_number("Heater on.");
      }
    }
  }
  else {                                   // ��������� ������
    digitalWrite(HEATING_OUT, LOW);
    heating_status = 0;
    if (send_msg) {
      if (phone_index > -1) {
        send_message_number("Heater off.", phone[phone_index]);
      } else {
        send_message_all_number("Heater off.");
      }
    }
  }
}

void send_message_all_number(String msg) {
  for (int i = 0; i < PHONE_SIZE; i++) {
    send_message_number(msg, phone[i]);
  }
}

void send_message_number(String msg, String number) {
  Serial.println("PHONE:" + number);
  Serial.println("SMS:" + msg + "\n");
  sendATCommand("AT+CMGS=\"" + number + "\"", true);
  sendATCommand(msg + "\r\n" + (String)((char)26), true);
}

void parseSMS(String msg) {
  String msgbody = "";
  String msgphone = "";
  String msgheader = "";

  msg = msg.substring(msg.indexOf("+CMGR: "));
  msgheader = msg.substring(0, msg.indexOf("\r"));

  msgbody = msg.substring(msgheader.length() + 2);
  msgbody = msgbody.substring(0, msgbody.lastIndexOf("OK"));
  msgbody.trim();

  int firstIndex = msgheader.indexOf("\",\"") + 3;
  int secondIndex = msgheader.indexOf("\",\"", firstIndex);
  msgphone = msgheader.substring(firstIndex, secondIndex);
  Serial.println("Phone: " + msgphone);
  Serial.println("Message: " + msgbody);

  int phone_index = 0;
  while (phone[phone_index] != msgphone && phone_index < PHONE_SIZE) {
    phone_index++;
  }

  if (phone_index < PHONE_SIZE) {
    if (msgbody == "heating off" || msgbody == "Heating off") {
      heating(0, true, phone_index);
    }
    if (msgbody == "heating on" || msgbody == "Heating on") {
      heating(1, true, phone_index);
    }
    if (msgbody == "status" || msgbody == "Status") {
      home_status(phone_index);
    }
  }
}

void msg_modem_parse() {
  if (SIM800.available()) {
    String msg = waitResponse();
    msg.trim();
    Serial.println(msg);
    while (msg.startsWith("+CMTI:")) {
      int index = msg.lastIndexOf(",");
      String result = msg.substring(index + 1, msg.length());
      result.trim();
      msg = sendATCommand("AT+CMGR=" + result, true);
      parseSMS(msg);
      if (!SIM800.available()) {
        sendATCommand("AT+CMGDA=\"DEL ALL\"", true);
      }
    }
  }
}

void button_parse() {
  if (!digitalRead(BUTTON_HEATING_ON) || !digitalRead(BUTTON_HEATING_OFF)) {
    int button_count_on = 0;
    while (button_count_on < 10 && !digitalRead(BUTTON_HEATING_ON)) {
      button_count_on++;
      delay(100);
    }
    int button_count_off = 0;
    while (button_count_off < 10 && !digitalRead(BUTTON_HEATING_OFF)) {
      button_count_off++;
      delay(100);
    }

    if (DEBUG) {
      Serial.print("button: ");
      Serial.print(button_count_on);
      Serial.print(" ");
      Serial.print(button_count_off);
      Serial.print("\n");
    }

    if ((button_count_on == 10 && button_count_off != 10) || (button_count_on != 10 && button_count_off == 10)) {
      if (!digitalRead(BUTTON_HEATING_OFF) && heating_status == 1 && button_count_off == 10) {
        heating(0, false, 0);
      }
      if (!digitalRead(BUTTON_HEATING_ON) && heating_status == 0 && button_count_on == 10) {
        heating(1, false, 0);
      }
    }
  }
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readString();
    sendATCommand(cmd, true);
  }
  button_parse();
  msg_modem_parse();
  delay(100);
}
