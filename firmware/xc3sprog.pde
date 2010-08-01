#define TDI 2
#define TCK 3
#define TMS 4
#define TDO 5

#define TMSBIT 1
#define TDIBIT 2
#define RXBIT 4

void setup() {
	Serial.begin(115200);
	pinMode(TDI, OUTPUT);
	pinMode(TCK, OUTPUT);
	pinMode(TMS, OUTPUT);
	pinMode(TDO, INPUT);
	digitalWrite(TDO, LOW); // disable pullup
}
void loop() {
	while (!Serial.available());
	char c = Serial.read();
	digitalWrite(TCK, LOW);
	digitalWrite(TMS, ((c & TMSBIT) == TMSBIT) ? HIGH : LOW);
	digitalWrite(TDI, ((c & TDIBIT) == TDIBIT) ? HIGH : LOW);
	digitalWrite(TCK, HIGH);
	if ((c & RXBIT) == RXBIT) { // rx
		Serial.write(0x30 | (digitalRead(TDO) == HIGH ? 1 : 0));
	}
}
