
#define STAGE_FLOOR     0
#define STAGE_SET       1
#define STAGE_SINGLE    2
#define STAGE_PORT      3
#define STAGE_DOT       4
#define STAGE_VALUE     5
#define STAGE_FINAL     6

#define ARRAY_SIZE(a) (sizeof((a)) / sizeof((a)[0]))

unsigned char i = 0;

char ioMap[] = {
  5,   6,  7,  8,  9, 10, 11, 12, // Relais 0-7
  A0, A1, A2, A3, A4, A5,  3      // Digital 8-E
};

unsigned char stage = STAGE_FLOOR;
unsigned char val = 0;
unsigned char tmp = 0;
unsigned char port = 0;

unsigned int led_off = 0;

void parserSerialStream() {

  if (Serial.available() == 0) {
    return;
  }

  char data = Serial.read();
  
  Serial.write(data);
  
  switch(stage) {

    case STAGE_FLOOR:

        port = 0;

        if(data == 's') {
            stage = STAGE_SET;
        }

        break;

    case STAGE_SET:

        if(data == 'p') {
            stage = STAGE_SINGLE;
        } else {
            stage = STAGE_FLOOR;
        }

        break;

    case STAGE_SINGLE:

        if(data >= 48 && data <= 57) {
          tmp = data-48;
        } else if(data >= 65 && data <= 69) {
          tmp = data-65+10;
        }
        
        if(tmp >= 0 || tmp < 16) {
            port = tmp;
            stage = STAGE_DOT;
        } else {
            stage = STAGE_FLOOR;
        }

        break;

    case STAGE_DOT:

        if(data == '.') {
            stage = STAGE_VALUE;
        } else {
            stage = STAGE_FLOOR;
        }

        break;

    case STAGE_VALUE:

        tmp = data-48;

        if( tmp == 0 || tmp == 1 ) {
            val = tmp;
            stage = STAGE_FINAL;
        } else {
            stage = STAGE_FLOOR;    
        }

        break;

    case STAGE_FINAL:

        if( data == '\r' ) {

            PORTD |= (1<<PIN4);
            led_off = millis()+1000;
            
            digitalWrite(2, HIGH);

            if(val) {
              digitalWrite(ioMap[port], HIGH);
            } else {
              digitalWrite(ioMap[port], LOW);
            }
            
            Serial.write("\r\nOK\r\n");

            stage = STAGE_FLOOR;
        } else {
            stage = STAGE_FLOOR;
        }

        break;
    }
}

void setup() {
  Serial.begin(19200);
  
  for(i = 0; i < ARRAY_SIZE(ioMap); i++) {
    pinMode(ioMap[i], OUTPUT);      // schaltet den Pin als digitalen Ausgang
    digitalWrite(ioMap[i], LOW);
  }
  
  pinMode(2, HIGH);
  digitalWrite(2, LOW);
}

void loop() {
  parserSerialStream();
  
  if(led_off < millis()) {
    digitalWrite(2, LOW);  
  }
}
