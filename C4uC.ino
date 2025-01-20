#include <avr/pgmspace.h> // necessary for IDE version below 1.0 (2011)
#include  <Wire.h>
#include  <LiquidCrystal_I2C.h>

#define BUTTON        14
#define ENC_A         15
#define ENC_B         16

// stati menu
#define START             0
#define MODE_AUTO         1
#define SET_CAB           3
#define SET_BT            4
#define BACK_AUTO_MAIN    7
#define MODE_MANUAL       2
#define SET_TIME          5
#define SET_MAH           6
#define BACK_MANUAL_MAIN  8
#define TIME_MAH_SUB     10
#define BACK_T_M_SUB     11
#define INPUT_TIMEMAH    12 // stato utilizzato per settare tempo e mAh
#define CHARGING_ST       9
#define LCD_OFF          13
#define CHARGING_MD      14

// possibili comandi (da pulsanti o encoder)
#define NOC      0
#define RIGHT    1
#define LEFT     2
#define PUSH     3

LiquidCrystal_I2C lcd(0x27, 20, 4);

const char m00[] PROGMEM = "                    ";  // stringa vuota, utilizzata per cancellare
const char m01[] PROGMEM = " <      START     > ";
const char m02[] PROGMEM = " 2 click to charge  ";
const char m03[] PROGMEM = " <   MODE AUTO    > ";
const char m04[] PROGMEM = "  set cable or BT   ";
const char m05[] PROGMEM = " <   MODE MANUAL  > ";
const char m06[] PROGMEM = "  set time or mAh   ";
const char m07[] PROGMEM = "  <  SET CABLE    > ";
const char m08[] PROGMEM = " <  SET BLUETOOTH > ";
const char m09[] PROGMEM = " <      BACK      > ";
const char m10[] PROGMEM = " <   charging...  > ";
const char m11[] PROGMEM = "      SET TIME      ";
const char m12[] PROGMEM = "      SET mAh       ";
const char m13[] PROGMEM = " Hrs:1              ";
const char m14[] PROGMEM = " mAh:4000           ";
const char m15[] PROGMEM = "   Remaining time   ";
const char m16[] PROGMEM = "  2 click to stop   ";
const char m17[] PROGMEM = "    TURN OFF LCD    ";
const char m18[] PROGMEM = "  click to turn off ";
const char m19[] PROGMEM = "     to set time    ";
const char m20[] PROGMEM = "     to set mah     ";

const char *const string_table[] PROGMEM = {m00, m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20};

char buffer[30];

uint8_t cmd;
uint8_t lcd_line1;
uint8_t lcd_line2;
uint8_t menu_state;
uint8_t lastStateCLK;
uint8_t currentStateCLK;

uint8_t press_counter = 0;

unsigned long last = 0;
unsigned long last_blink = 0;
unsigned long last_showcountdown = 0;
unsigned long time_mah_var = 0;
unsigned long press_counter_timer = 0;  // non utilizzata

bool bool_lcd_update;
bool bool_blink = false;
bool bool_lcd_off = false;
bool bool_set_mah = false;
bool bool_set_time = false;
bool bool_charge_MD = false;
bool bool_charge_ST = false;
bool bool_set_time_mah = false;
bool bool_input_time_mah = false;



void menu_selection();
void lcd_set_state_msg();
void countdown_function();
void show_countdown();

void setup() {
  lcd.init();
  lcd.begin(20, 4);
  lcd.backlight();
  lcd.clear();

  Serial.begin(115200);
  while (!Serial);

  pinMode(13, OUTPUT);
  pinMode(BUTTON, INPUT);
  pinMode(ENC_A, INPUT);
  pinMode(ENC_B, INPUT);
  digitalWrite(13, LOW);

  menu_state = START;
  lcd_line1 = 1;
  lcd_line2 = 2;
  bool_lcd_update = true;
  cmd = NOC;
  strcpy_P(buffer, (char *)pgm_read_word(&(string_table[lcd_line1])));
  lcd.setCursor(0, 1);
  lcd.print(buffer);
  strcpy_P(buffer, (char *)pgm_read_word(&(string_table[lcd_line2])));
  lcd.setCursor(0, 2);
  lcd.print(buffer);
  lastStateCLK = digitalRead(ENC_A);
}

void loop() {
  // button pressed (hw PULL_UP)
  if (!digitalRead(BUTTON)) {
    cmd = PUSH;
    delay(250);
    menu_selection();
  }

  //encoder rotation
  currentStateCLK = digitalRead(ENC_B);
  if (currentStateCLK != lastStateCLK  && currentStateCLK == 1) {
    if (digitalRead(ENC_A) != currentStateCLK) {
      cmd = LEFT;
      delay(180);
      menu_selection();
    } else {
      cmd = RIGHT;
      delay(180);
      menu_selection();
    }
  }
  lastStateCLK = currentStateCLK;

  // blink di "Hrs" oppure di "mAh" mentre viene inserito il tempo o i mAh
  if (bool_input_time_mah && menu_state == TIME_MAH_SUB) {
    if (millis() - last_blink > 500) {
      last_blink = millis();
      if (!bool_blink) {
        lcd.setCursor(1, 2);
        if (bool_set_time_mah) {
          lcd.print("Hrs:");
        } else {
          lcd.print("mAh:");
        }
      } else {
        lcd.setCursor(1, 2);
        lcd.print("    ");
      }
      bool_blink ^= true;
    }
  }

  if (bool_charge_MD) {
    countdown_function();
    if (millis() - last_showcountdown >= 1000  && menu_state == CHARGING_MD) {
      show_countdown();
    }
  }
}

void menu_selection() {
  switch (menu_state) {
    case START:
      if (cmd == RIGHT) {
        if (bool_lcd_off) {
          lcd.backlight();
          bool_lcd_off = false;
        }
        menu_state = MODE_AUTO;
        lcd_line1 = 3;
        lcd_line2 = 4;
        bool_lcd_update = true;
        press_counter = 0;
      } else if (cmd == LEFT) {
        if (bool_lcd_off) {
          lcd.backlight();
          bool_lcd_off = false;
        }
        menu_state = LCD_OFF;
        lcd_line1 = 17;
        lcd_line2 = 18;
        bool_lcd_update = true;
        press_counter = 0;
      }
      else if (cmd == PUSH) {
        press_counter += 1;
        if (press_counter >= 2) {
          bool_charge_ST = true;
          digitalWrite(13, HIGH);
          menu_state = CHARGING_ST;
          lcd_line1 = 10;
          lcd_line2 = 16;
          bool_lcd_update = true;
          press_counter = 0;
          lcd.noBacklight();
          bool_lcd_off = true;
        }
      }
      break;

    case MODE_AUTO:
      if (cmd == RIGHT) {
        menu_state = MODE_MANUAL;
        lcd_line1 = 5;
        lcd_line2 = 6;
        bool_lcd_update = true;
      } else if (cmd == LEFT) {
        menu_state = START;
        lcd_line1 = 1;
        lcd_line2 = 2;
        bool_lcd_update = true;
      } else if (cmd == PUSH) {
        menu_state = SET_CAB;
        lcd_line1 = 7;
        lcd_line2 = 0;
        bool_lcd_update = true;
      }
      break;

    case MODE_MANUAL:
      if (cmd == RIGHT) {
        menu_state = LCD_OFF;
        lcd_line1 = 17;
        lcd_line2 = 18;
        bool_lcd_update = true;
      } else if (cmd == LEFT) {
        menu_state = MODE_AUTO;
        lcd_line1 = 3;
        lcd_line2 = 4;
        bool_lcd_update = true;
      } else if (cmd == PUSH) {
        lcd_line1 = 11;
        lcd_line2 = 0;
        bool_lcd_update = true;
        menu_state = SET_TIME;
      }
      break;

    case LCD_OFF:
      if (cmd == RIGHT) {
        if (bool_lcd_off) {
          lcd.backlight();
          bool_lcd_off = false;
        }
        if (bool_charge_ST) {
          menu_state = CHARGING_ST;
          lcd_line1 = 10;
          lcd_line2 = 16;
          bool_lcd_update = true;
        } else if (bool_charge_MD) {
          menu_state = CHARGING_MD;
          lcd_line1 = 15;
          lcd_line2 = 0;
          bool_lcd_update = true;
        } else {
          menu_state = START;
          lcd_line1 = 1;
          lcd_line2 = 2;
          bool_lcd_update = true;
        }
      } else if (cmd == LEFT) {
        if (bool_lcd_off) {
          lcd.backlight();
          bool_lcd_off = false;
        }
        if (bool_charge_ST) {
          menu_state = CHARGING_ST;
          lcd_line1 = 10;
          lcd_line2 = 16;
          bool_lcd_update = true;
        } else if (bool_charge_MD) {
          menu_state = CHARGING_MD;
          lcd_line1 = 15;
          lcd_line2 = 0;
          bool_lcd_update = true;
        } else {
          menu_state = MODE_MANUAL;
          lcd_line1 = 5;
          lcd_line2 = 6;
          bool_lcd_update = true;
        }
      } else if (cmd == PUSH) {
        lcd.noBacklight();
        bool_lcd_off = true;
      }
      break;

    case CHARGING_ST:
      if (cmd == RIGHT || cmd == LEFT) {
        if (bool_lcd_off) {
          lcd.backlight();
          bool_lcd_off = false;
        }
        menu_state = LCD_OFF;
        lcd_line1 = 17;
        lcd_line2 = 18;
        bool_lcd_update = true;
        press_counter = 0;
      }
      if (cmd == PUSH) {
        press_counter += 1;
        if (press_counter >= 2) {
          digitalWrite(13, LOW);
          bool_charge_ST = false;
          press_counter = 0;
          menu_state = START;
          lcd_line1 = 1;
          lcd_line2 = 2;
          bool_lcd_update = true;
          lcd.noBacklight();
          bool_lcd_off = true;
        }
      }
      break;

// stati della modalità automatica non implmentati
    case SET_CAB:
      break;
    case SET_BT:
      break;
//

    case SET_TIME:
      if (cmd == RIGHT) {
        menu_state = SET_MAH;
        lcd_line1 = 12;
        lcd_line2 = 0;
        bool_lcd_update = true;
      } else if (cmd == LEFT) {
        menu_state = BACK_MANUAL_MAIN;
        lcd_line1 = 9;
        lcd_line2 = 0;
        bool_lcd_update = true;
      } else if (cmd == PUSH) {
        menu_state = TIME_MAH_SUB;
        lcd_line1 = 11;
        lcd_line2 = 13;
        bool_lcd_update = true;
        bool_set_time_mah = true;
        time_mah_var = 1;
      }
      break;

    case SET_MAH:
      if (cmd == RIGHT) {
        menu_state = BACK_MANUAL_MAIN;
        lcd_line1 = 9;
        lcd_line2 = 0;
        bool_lcd_update = true;
      } else if (cmd == LEFT) {
        menu_state = SET_TIME;
        lcd_line1 = 11;
        lcd_line2 = 0;
        bool_lcd_update = true;
      } else if (cmd == PUSH) {
        menu_state = TIME_MAH_SUB;
        lcd_line1 = 12;
        lcd_line2 = 14;
        bool_lcd_update = true;
        bool_set_time_mah = false;
        time_mah_var = 4000;
      }
      break;

    case BACK_MANUAL_MAIN:
      if (cmd == RIGHT) {
        menu_state = SET_TIME;
        lcd_line1 = 11;
        lcd_line2 = 0;
        bool_lcd_update = true;
      } else if (cmd == LEFT) {
        menu_state = SET_MAH;
        lcd_line1 = 12;
        lcd_line2 = 0;
        bool_lcd_update = true;
      } else if (cmd == PUSH) {
        lcd_line1 = 5;
        lcd_line2 = 6;
        bool_lcd_update = true;
        menu_state = MODE_MANUAL;
      }
      break;

    case TIME_MAH_SUB:
      if (cmd == RIGHT) {
        if (bool_input_time_mah) {
          if (bool_set_time_mah) {            // consente di impostare il numero di ore
            time_mah_var += 1;
          } else if (!bool_set_time_mah) {    // consente di inserire la quantità di mAh
            time_mah_var += 100;
          }
          lcd.setCursor(5, 2);
          lcd.print(time_mah_var);
        }
        else {
          menu_state = BACK_T_M_SUB;
          lcd_line1 = 9;
          if (bool_set_time_mah) {
            lcd_line2 = 19;
          } else {
            lcd_line2 = 20;
          }
          bool_lcd_update = true;
        }
      } else if (cmd == LEFT) {
        if (bool_input_time_mah) {
          lcd.setCursor(5, 2);
          lcd.print("              ");
          if (bool_set_time_mah) {
            if (time_mah_var <= 0) {
              time_mah_var = 0;
            }
            else {
              time_mah_var -= 1;
            }
          } else if (!bool_set_time_mah) {
            if (time_mah_var <= 0) {
              time_mah_var = 0;
            }
            else {
              time_mah_var -= 100;
            }
          }
          lcd.setCursor(5, 2);
          lcd.print(time_mah_var);
        }
        else {
          menu_state = BACK_T_M_SUB;
          lcd_line1 = 9;
          if (bool_set_time_mah) {
            lcd_line2 = 19;
          } else {
            lcd_line2 = 20;
          }
          bool_lcd_update = true;
        }

      }  else if (cmd == PUSH) {
        bool_input_time_mah ^= true;
        if (!bool_input_time_mah) {
          bool_charge_MD = true;
          bool_input_time_mah = false;
          digitalWrite(13, HIGH);
          menu_state = CHARGING_MD;
          lcd_line1 = 15;
          lcd_line2 = 0;
          bool_lcd_update = true;
          if (bool_set_time_mah) {
            time_mah_var *= 3600000;
          }
          else {
            time_mah_var =  (time_mah_var / 2000) * 3600000;  // Hp. caricatore da 2000 mAh
          }
          lcd.noBacklight();
          last = millis();
          digitalWrite(13, HIGH);
          bool_charge_MD = true;
          bool_lcd_off = true;
        }
      }
      break;

    case CHARGING_MD:
      if (cmd == RIGHT || cmd == LEFT) {
        if (bool_lcd_off) {
          lcd.backlight();
          bool_lcd_off = false;
        }
        menu_state = LCD_OFF;
        lcd_line1 = 17;
        lcd_line2 = 18;
        bool_lcd_update = true;
        press_counter = 0;
      } else if (cmd == PUSH) {
        press_counter += 1;
        if (press_counter >= 2) {
          bool_charge_MD = false;
          digitalWrite(13, LOW);
          press_counter = 0;
          menu_state = START;
          lcd_line1 = 1;
          lcd_line2 = 2;
          bool_lcd_update = true;
          lcd.noBacklight();
          bool_lcd_off = true;
        }
      }
      break;

    case BACK_T_M_SUB:
      if (cmd == RIGHT || cmd == LEFT) {
        menu_state = TIME_MAH_SUB;
        if (bool_set_time_mah) {
          lcd_line1 = 11;
          lcd_line2 = 13;
        } else {
          lcd_line1 = 12;
          lcd_line2 = 14;
        }
        bool_lcd_update = true;
      } else if (cmd == PUSH) {
        if (bool_set_time_mah) {
          menu_state = SET_TIME;
          lcd_line1 = 11;
        } else {
          menu_state = SET_MAH;
          lcd_line1 = 12;
        }
        lcd_line2 = 0;
        bool_lcd_update = true;
      }
      break;
  }

  if (bool_lcd_update) {
    bool_lcd_update = false;
    lcd_set_state_msg();
  }

  cmd = NOC;
}

void lcd_set_state_msg() {
  lcd.clear();
  strcpy_P(buffer, (char *)pgm_read_word(&(string_table[lcd_line1])));
  lcd.setCursor(0, 1);
  lcd.print(buffer);
  strcpy_P(buffer, (char *)pgm_read_word(&(string_table[lcd_line2])));
  lcd.setCursor(0, 2);
  lcd.print(buffer);
}

void countdown_function() {
  if (millis() - last >= time_mah_var) {
    digitalWrite(13, LOW);
    bool_charge_MD = false;
    menu_state = START;
    lcd_line1 = 1;
    lcd_line2 = 2;
    bool_lcd_update = true;
    menu_selection();
  }
}

// funzione per la visualizzazione del tempo rimanente - modalità manuale
void show_countdown() {
  last_showcountdown = millis();
 
  uint8_t h = ((time_mah_var - (millis() - last)) / 1000) / 3600;
  uint8_t m = (((time_mah_var - (millis() - last)) / 1000) / 60) % 60;
  uint8_t s = ((time_mah_var - (millis() - last)) / 1000) % 60;

  lcd.setCursor(6, 2);
  if (h < 1) {
    lcd.print((String)"00");
  } else if (h < 10) {
    lcd.print((String)"0" + h);
  } else {
    lcd.print(h);
  }
  lcd.setCursor(9, 2);
  if (m < 10) {
    lcd.print((String)"0" + m);
  } else {
    lcd.print(m);
  }
  lcd.setCursor(12, 2);
  if (s < 10) {
    lcd.print((String)"0" + s);
  } else {
    lcd.print(s);
  }
}
