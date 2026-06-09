
//navodanjavanje preko poruka 


#include <SoftwareSerial.h>

// SIM800L: TX na D2, RX na D3
SoftwareSerial sim800(2, 3); 

// --- KONFIGURACIJA PINOVA ---
const int relejPins[] = {7, 8, 9};       // Glavni releji (Ventili)
const int povratniPins[] = {10, 11, 12}; // Pomoćni releji (Povratna informacija)
const int tasterPins[] = {4, 5, 6};      // Tasteri za ručno upravljanje
const int naponPin = A0;                 // Pin za naponski djelitelj

// --- PROMENLJIVE ---
bool stanjeVentila[] = {false, false, false};
String mojBroj = "*********"; // Broj telefona

// FUNKCIJA ZA MJERENJE NAPONA
String dobijNapon() {
  int raw = analogRead(naponPin);
  // Formula: (V_ref / 1023) * Omjer_Otpornika
  // Za 10k i 5k otpornike, omjer je tacno 3.0
  float vOut = (raw * 5.0) / 1023.0;
  float vIn = vOut * 3.0; 
  
  // Vraca napon kao tekst (npr. "12.6V")
  return String(vIn, 1) + "V";
}

void setup() {
  Serial.begin(9600);
  sim800.begin(9600);

  for (int i = 0; i < 3; i++) {
    pinMode(relejPins[i], OUTPUT);
    digitalWrite(relejPins[i], HIGH); // Releji su ugaseni na startu (HIGH nivo)
    pinMode(povratniPins[i], INPUT_PULLUP); 
    pinMode(tasterPins[i], INPUT_PULLUP);
  }

  Serial.println("Sistem startuje...");
  delay(10000); // Vrijeme da se SIM800L poveze na mrezu

  sim800.println("AT+CMGF=1"); // Postavi SMS na tekstualni mod
  delay(200);
  sim800.println("AT+CNMI=2,2,0,0,0"); // Direktno citanje dolaznih poruka
  Serial.println("SISTEM SPREMAN. Trenutni napon: " + dobijNapon());
}

void loop() {
  // 1. SMS KOMANDE
  if (sim800.available()) {
    String sms = sim800.readString();
    sms.toLowerCase();
    
    for (int i = 0; i < 3; i++) {
      if (sms.indexOf("upali" + String(i + 1)) > -1) kontrolaVentila(i, true);
      if (sms.indexOf("ugasi" + String(i + 1)) > -1) kontrolaVentila(i, false);
    }
  }

  // 2. RUČNA KONTROLA (TASTERI)
  for (int i = 0; i < 3; i++) {
    if (digitalRead(tasterPins[i]) == LOW) {
      delay(50); // Debounce
      if (digitalRead(tasterPins[i]) == LOW) {
        stanjeVentila[i] = !stanjeVentila[i];
        kontrolaVentila(i, stanjeVentila[i]);
        while (digitalRead(tasterPins[i]) == LOW); // Cekaj da pustis taster
      }
    }
  }
}

void kontrolaVentila(int index, bool upali) {
  String trenutniNapon = dobijNapon();
  String poruka = "";

  if (upali) {
    digitalWrite(relejPins[index], LOW); // Pali relej (LOW aktivira vecinu modula)
    stanjeVentila[index] = true;
    
    delay(3000); // Sacekaj malo da se pomocni relej stabilizuje

    // Provjera povratne informacije
    if (digitalRead(povratniPins[index]) == LOW) {
      poruka = "Sistem: Ventil " + String(index + 1) + " UKLJUCEN. Napon: " + trenutniNapon;
    } 
    else {
      poruka = "ALARM: Ventil " + String(index + 1) + " NEMA STRUJE! Napon: " + trenutniNapon;
    }
  } 
  else {
    digitalWrite(relejPins[index], HIGH); // Gasi relej
    stanjeVentila[index] = false;
    poruka = "Sistem: Ventil " + String(index + 1) + " ISKLJUCEN. Napon: " + trenutniNapon;
  }
  
  posaljiSMS(poruka);
}

void posaljiSMS(String tekst) {
  sim800.print("AT+CMGS=\"");
  sim800.print(mojBroj);
  sim800.println("\"");
  delay(500);
  sim800.print(tekst);
  delay(500);
  sim800.write(26); // CTRL+Z za slanje
  delay(3000);
  Serial.println("SMS Poslan: " + tekst);
}
