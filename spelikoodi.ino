// Speli by Alpo Hassinen, 2019
// Arduino Nano

#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C-osoite on yleensä 0x20, 0x27 tai 0x3F. Ellei näy, kokeile muuta arvoa
  // LCD kytketään nastoihin GND, 5V, SDA (pinniin A4) ja SCL (pinniin A5)

const word alkunopeus = 1000;                 // Nopeus alussa (1000, eli nappien väli noin 1000 ms = 1 sekunti
const word miniminopeus = 200;                // Ainakin tämän verran valojen väli (200 ms) Peli ei tule tätä nopeammaksi. Oltava >= kuin paloaika.
const word paloaika = 200;                    // Kuinka kauan lamppu päällä (200 ms) Oltava <= kuin miniminopeus
const word nopeutus = 4;                      // Joka napin jälkeen nopeutetaan (4 ms) Jos laitat isomman peli vaikeutuu nopeammin
const word nopeusmuutos = 100;                // Satunnaisuus 0 - 250 (50) Pelin nopeus vaihtelee, ja on siis hieman hankalampi. Tässä nopeutus joku arvo välillä 0 ja 100 ms
const word nappiviive = 1;                    // Tauko näppäinten tutkimisen välillä (1 ms) Jos isonnat tätä niin peli hidastuu vastaavasti, esim arvolla 2 tulle kaksi kertaa hitaammaksi

const byte valo0 = 2;                         // Valot ovat pinneissä D2 - D5
const byte valo1 = 3;
const byte valo2 = 4;
const byte valo3 = 5;
const byte nappi0 = 6;                        // Napit ovat pinneissä D6 - D9
const byte nappi1 = 7;
const byte nappi2 = 8;
const byte nappi3 = 9;

const word eeEnkkaOs = 2;                     // Mihin EEPROM-osoitteeseen tallennetaan ennätys, enkka vie 2 tavua
const word eePelikrOs = 4;                   // Mihin EEPROM-osoitteeseen tallennetaan pelikerrat. Pelikerrat tarvitsee 4 tavua (seuraava vapaa osoite on siis 8)

byte lamppu[20];                              // Tähän taulukkoon tallennetaan arvotut valot
byte lamppukohta;                             // Valojen tallennuskohta
byte nappikohta;                              // Nappien painallusten kohta

word tulos = 0;                               // Oma tulos
word ennatys = 0;                             // Kaikkien aikojen ennätys
word istuntoennatys = 0;                      // Tämän pelikerran ennätys, nollautuu kun virta katkaistaan
long pelikerta = 0;                           // Montako kertaa tätä peliä on pelattu yhteensä

word nopeus;                                  // Pelin nopeus ja siitä laskettu viive
word viive;

byte arvonta;                                 // Arvottu valo
byte vanhaarvonta;                            // edellinen valo
byte nappi;                                   // Painettu nappi
byte vanhanappi;                              // edellinen painettu nappi

boolean loppu;                                // Jos true, niin peli loppu

void setup()
{
  pinMode(nappi0, INPUT_PULLUP);              // Napit sisäänmenoja, niissä ylösvetovastus
  pinMode(nappi1, INPUT_PULLUP);
  pinMode(nappi2, INPUT_PULLUP);
  pinMode(nappi3, INPUT_PULLUP);
  pinMode(valo0, OUTPUT);                     // Valot ulostuloja
  pinMode(valo1, OUTPUT);
  pinMode(valo2, OUTPUT);
  pinMode(valo3, OUTPUT);

  kaikkivalot(1);                             // Kaikki valot päälle

  // Jos painetaan vas ja kolmas vas napit alas ja laitetaan virta päälle, niin ennätys nollataan
  if (digitalRead(nappi3) == 0 && digitalRead(nappi2) == 1 && digitalRead(nappi1) == 0 && digitalRead(nappi0) == 1) {
    ennatys = 0;
    EEPROM.put(eeEnkkaOs, ennatys);
  }

  // EEPROM.put(eePelikrOs, pelikerta);              // Jos haluat nollata pelikerrat, niin laita tämä rivi yhden ajokerran, sitten poista rivi uudelleen

  EEPROM.get(eeEnkkaOs, ennatys);                    // Ennätys ja pelikerrat on tallennettu EEPROM-muistiin näihin osoitteiseen
  EEPROM.get(eePelikrOs, pelikerta);

  lcd.init();                           // LCD näytön alustus
  lcd.backlight();                      // Taustavalo päälle
  lcd.setCursor(0, 0);
  lcd.print("Speli  ");                 // Alkuteksti. Jos tarvitsee, niin ä kirjain on ((char)225) ja ö = ((char)239)

  lcd.setCursor(0, 1);                  // Huom! Ylärivi on 0 ja alarivi on 1. Tässä siis kirjoitetaan alariville
  lcd.print("Pelattu ");
  lcd.print(pelikerta);
  lcd.print(" krt");

  delay (3000);                         // Odotellaan hieman
}

void loop()
{
  lcd.clear();                          // Kuvaruudulle tietoa ennätyksistä ja edellisestä tuloksesta
  lcd.setCursor(0, 0);
  lcd.print("Enkka");
  lcd.setCursor(6, 0);
  lcd.print(ennatys);                   // Kaikkien aikojen ennätys
  lcd.setCursor(12, 0);
  lcd.print(istuntoennatys);            // Tämän pelikerran ennätys (nollautuu kun virta katkaistaan)
  lcd.setCursor(0, 1);
  lcd.print("Tulos");
  lcd.setCursor(6, 1);
  lcd.print(0);                         // Oma pelin tulos
  lcd.setCursor(12, 1);
  lcd.print(tulos);                     // Edellisen kierroksen tulos

  kaikkivalot(0);                       // Kaikki valot pois
  valo(0);                              // Ainoastaan reunimmainen päälle
  
  do {                                  // Odotellaan kunnes pelaaja painaa reunimmaista nappia
    byte puppu = random(4);             // Satunnaisluku saadaan tällä hieman satunnaisemmaksi
  }
  while (digitalRead(nappi0) == 1);

  digitalWrite(valo0, 0);
  delay(100);

  while (digitalRead(nappi0) == 0) {};  // Odotetaan että käyttäjä ottaa käden pois napilta

  pelikerta ++;                         // Tallennetaan pelikerta muistiin
  EEPROM.put(eePelikrOs, pelikerta);

  delay(2000);

  vanhaarvonta = 50;                    // Näihin joku luku aluksi > 4
  vanhanappi = 50;
  nappi = 50;

  nopeus = alkunopeus;

  lamppukohta = 0;
  nappikohta = 0;
  tulos = 0;

  loppu = false;

  // randomSeed(1);                             // Jos haluat aina samanlaisen arvonnan, niin laita tämä rivi :-)

  do {                                          // Peli pyörii tässä silmukassa
    do {                                        // Arvotaan seuraava lamppu
      arvonta = random(4);
    }
    while (arvonta == vanhaarvonta);            // Ei samaa valoa peräkkäin

    vanhaarvonta = arvonta;
    
    lamppu[lamppukohta] = arvonta;              // Arvottu valo laitetaan taulukkoon jemmaan
    lamppukohta++;
    if (lamppukohta >= 20) lamppukohta = 0;
    
    valo(arvonta);                              // Valo päälle

                                                // Lasketaan viive valojen välillä
    if (nopeus > miniminopeus) nopeus = nopeus - nopeutus ;

    viive = nopeus - random(nopeusmuutos);      // Satunnaisuutta nopeuteen
    if (viive < miniminopeus) viive = miniminopeus;

    for (word vi = 0; vi <= viive; vi++) {      // Odotetaan viiveen aika tässä silmukassa
      
      if (vi == paloaika) kaikkivalot(0);       // Kun paloaika on kulunut, sammuta valot. Jos poistat tämän rivin, valo palaa pitempään
      
      delay(nappiviive);                        // Pikkuisen hidastusta, eli 1 millisekunti
      otanappi();                               // Kutsutaan aliohjelmaa, jos nappia painettu, niin se on muuttujassa nappi

      if (nappi != vanhanappi) {                // Jos painettu eri nappia (Tällä eliminoidaan saman napin painallus uudelleen)
        vanhanappi = nappi;
        
        if (nappi == lamppu[nappikohta]) {      // Painettu oikeaa nappia
          tulos++;
          lcd.setCursor(6, 1);
          lcd.print(tulos);
          nappikohta++;
          if (nappikohta >= 20) nappikohta = 0;
        } else {
          loppu = true;                          // Painettu väärää nappia, lopetetaan
          break;
        }
      }
    }
  }
  while (loppu == false);

                                                 // Tuli virhe
  kaikkivalot(1);                                // Kaikki valot päälle
  if (tulos > ennatys) {                         // Jos ennätystulos, niin tallennetaan EEPROM-muistiin
    ennatys = tulos;
    EEPROM.put(eeEnkkaOs, ennatys);
  }
  if (tulos > istuntoennatys) {                   // Myös tämän pelikerran ennätys laitetaan muistiin
    istuntoennatys = tulos;
  }

/*
  // Debuggausta yms varten: Jos haluat katsoa, että minkä virheen teit, niin näin voit tulostaa ensin painetun napin, ja sitten mikä sen olisi pitänyt olla
  lcd.setCursor(0, 1);
  lcd.print(nappi); 
  lcd.print(" ");
  lcd.print(lamppu[nappikohta]);
  lcd.print("  ");
*/ 
 
  delay(5000);                                   // Pakollinen hengähdystauko ennen seuraavaa peliä

} // Loop päättyy

// Aliohjelmat

void otanappi() {                                // Mitä nappia painettu
  if (digitalRead(nappi0) == 0) nappi = 0;
  if (digitalRead(nappi1) == 0) nappi = 1;
  if (digitalRead(nappi2) == 0) nappi = 2;
  if (digitalRead(nappi3) == 0) nappi = 3;
}

void kaikkivalot(byte arvo) {                   // Kaikki valot päälle tai pois
  digitalWrite(valo0, arvo);
  digitalWrite(valo1, arvo);
  digitalWrite(valo2, arvo);
  digitalWrite(valo3, arvo);
}

void valo(byte nro) {                           // Yksi valo päälle
  kaikkivalot(0);                               // Sammuta ensin kaikki
  if (nro == 0) digitalWrite(valo0, 1);         // Sitten haluttu valo päälle
  if (nro == 1) digitalWrite(valo1, 1);
  if (nro == 2) digitalWrite(valo2, 1);
  if (nro == 3) digitalWrite(valo3, 1);
}

