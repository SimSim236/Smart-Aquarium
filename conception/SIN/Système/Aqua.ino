/*

 * Aqua.ino
 *
 * Ce sketch Arduino est conçu pour gérer et contrôler un système de distribution de nourriture pour poisson dans un aquarium.
 * Il assure la distribution autonome ou non de la nourriture pour les poissons, selon la quantité de poissons,
 * cela étant surveiller par l'Arduino, il reçois depuis l'application mobile des instructions (Paramètrages, distribution de nourriture).
 * Ce système peut embarquer un mode manuel (Application ou Boutton physique pour distribuer)
 * et un mode automatique (Distribution à des heures prédéfinies et paramètrables depuis l'application).
 *
 * Le but étant de faciliter la vie des propriétaires d'aquariums en leur permettant de contrôler leur système à distance.
 * Objectif : Assister l'Homme.
 *
 * Fonctionnalités principales :
 * - Surveillance d'une modification de paramètres via Bluetooth (Fréquence, Quantité)
 * - Contrôle automatisé ou manuel d'un servo moteur qui réalise une rotation.
 * - Sauvegarde les paramètres dans l'EEPROM (Mémoire morte)
 * - Interface utilisateur pour le contrôle manuel et l'affichage de l'état du système
 *
 * Dépendances :
 * - Bibliothèque de base Arduino
 * - Bibliothèques de contrôle de capteurs (DS3231 (RTC))
 * - Bibliothèques de contrôle des actionneurs (Servo, Buzzer)
 * - Bibliothèque personnel (BuzzerSong)
 *
 * Auteur : [Sim & Lutr4nn]
 * Date : [2024]
 * Version : 1.0

*/

// Libraries
#include <Servo.h>          // Servo motor
#include <DS3231.h>         // RTC
#include <SoftwareSerial.h> // Bluetooth
#include <EEPROM.h>         // EEPROM - Stockage de données
#include <BuzzerSong.h>     // Buzzer - Take On Me -> Aha

// Define (Pins)
#define rxPin 2
#define txPin 3

#define RED 11
#define GRN 12
#define BLU 13

#define servoPin 7
#define buzzerPin 8
#define boutonPinM 9
#define boutonPinD 6

// Variables

String msg;

// Variable pour le moteur & répétitions
int stopAngle = 87;  // Position d'arrêt moteur - 87° Lycée / 88° Home
int moveAngle = 100; // Angle de rotation du servo
int p;               // Nombre de poissons

// Variables pour les heures de distribution
const int MAX_FEEDS = 3;
int feedHours[MAX_FEEDS];
int feedMinutes[MAX_FEEDS];
int hourCount = 0;
int minuteCount = 0;
int nbreH;

// Variables pour les états des modes
bool manualMode = false;

// Déclaration des objets
Servo myServo;
DS3231 clock;
RTCDateTime dt;
SoftwareSerial bluetooth(rxPin, txPin); // RX, TX
BuzzerSong buzzer(buzzerPin);           // Take On Me -> Aha

void setup()
{
    // Initialisation de la communication série
    Serial.begin(9600);
    Serial.println();
    Serial.println("--------------- SETUP ---------------");
    Serial.println();
    bluetooth.begin(9600);
    delay(10);
    Serial.println("Initialize HC05");

    // Initialisation du module RTC
    clock.begin();
    clock.setDateTime(__DATE__, __TIME__);
    Serial.println("Initialize DS3231 - RTC");

    // Attacher le servo moteur à la broche 9
    myServo.attach(servoPin);
    myServo.write(stopAngle); // Position initiale du servo
    delay(500);
    Serial.println("Initialize Servo");

    // Initialisation pinMode
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);

    pinMode(RED, OUTPUT);
    pinMode(GRN, OUTPUT);
    pinMode(BLU, OUTPUT);

    pinMode(buzzerPin, OUTPUT);

    pinMode(boutonPinM, INPUT);
    pinMode(boutonPinD, INPUT);
    Serial.println("Initialize Components");

    // Initialisation EEPROM - Stockage des données
    Serial.println("Initialize EEPROM");
    Serial.println();

    if (EEPROM.read(256) != 123)
    {
        EEPROM.write(256, 123);
        p = 1;
        nbreH = 1;
    }
    else
    {
        EEPROM.get(0, p);
        EEPROM.get(25, nbreH);
        for (int i = 0; i < nbreH; i++)
        {
            EEPROM.get(10 + i * sizeof(int), feedHours[i]);
            EEPROM.get(50 + i * sizeof(int), feedMinutes[i]);
            Serial.println("Heure de distriubtion: " + String(feedHours[i]) + "h" + String(feedMinutes[i]));
        }
    }
    Serial.println(String(p) + " poissons");
    Serial.println();

    dt = clock.getDateTime();
    Serial.println("Heure actuelle: " + String(dt.hour) + "h" + String(dt.minute));
    Serial.println();
    Serial.println("--------------- LOOP ---------------");
    Serial.println();
}

void loop()
{
    myServo.write(stopAngle); // Position initiale du servo
    delay(500);
    dt = clock.getDateTime();

    int etat1 = digitalRead(boutonPinM); // Bouton Mode Manuel -> Bleu
    int etat2 = digitalRead(boutonPinD); // Bouton Distribution -> Jaune

    // Vérifier les heures de distribution
    if (manualMode == false)
    {
        for (int i = 0; i < nbreH; i++)
        {
            if (dt.hour == feedHours[i] && dt.minute == feedMinutes[i])
            {
                distributeFood();
                setLEDColor(255, 255, 0); // Jaune
                delay(60000);
                setLEDColor(0, 0, 0); // Jaune
            }
        }
    }

    if (manualMode == false && etat1 == 1)
    {
        manualMode = true;
        setLEDColor(0, 255, 255); // Bleu clair
        delay(500);
    }
    else if (manualMode == true && etat1 == 1)
    {
        manualMode = false;
        clignoterLEDB(255, 0, 255, 3); // Violet
        delay(500);
    }

    if (manualMode == true && etat2 == 1)
    {
        distributeFood();
        setLEDColor(0, 255, 255); // Bleu clair
        delay(500);
    }
    else if (manualMode == false && etat2 == 1)
    {
        clignoterLEDB(255, 0, 0, 3); // Rouge
        delay(500);
    }

    if (bluetooth.available() > 0)
    {
        readSerialPort();
        if (msg.startsWith("D") && manualMode == true)
        { // Commande pour distribution manuelle
            distributeFood();
            setLEDColor(0, 255, 255); // Bleu clair
            delay(500);
        }
        else if (msg.startsWith("M"))
        { // Commande pour activer le mode manuel
            manualMode = true;
            setLEDColor(0, 255, 255); // Bleu clair
            delay(500);
        }
        else if (msg.startsWith("A") && manualMode == true)
        { // Commande pour activer le mode automatique
            manualMode = false;
            clignoterLEDB(255, 0, 255, 3); // Violet
            delay(500);
        }
        else if (msg.startsWith("P"))
        {                                        // Commande pour mettre à jour le nombre de poissons
            String value_str = msg.substring(1); // Extraire la partie après "P"
            p = value_str.toInt();
            Serial.print("Nombre de poissons mis à jour: ");
            Serial.println(p);
            EEPROM.put(0, p);
        }
        else if (msg.startsWith("Ho"))
        { // Commande pour mettre à jour les heures de distribution
            hourCount = 0;
            String value_str = msg.substring(2);
            processHours(value_str);
            Serial.println("Heures de distribution :");
            for (int i = 0; i < hourCount; i++)
            {
                Serial.print(feedHours[i]);
                Serial.print(" ");
            }
            Serial.println();
            int nbreH = hourCount;
            EEPROM.put(25, nbreH);
        }
        else if (msg.startsWith("Im"))
        { // Commande pour mettre à jour les minutes de distribution
            minuteCount = 0;
            String value_str = msg.substring(2); // Récupère la sous-chaîne après "Mi"
            processMinutes(value_str);
            Serial.println("Minutes de distribution :");
            for (int i = 0; i < minuteCount; i++)
            {
                Serial.print(feedMinutes[i]);
                Serial.print(" ");
            }
            Serial.println();
        }
        else if (msg.startsWith("Mo"))
        { // Commande pour changer l'angle du moteur si soucis
            if (stopAngle == 87)
            {
                stopAngle = 88;
            }
            else
            {
                stopAngle = 87;
            }
        }
        else if (msg.startsWith == ' ' || msg.startsWith == 'D' && manualMode == false)
        { // En cas d'erreur // Commande invalide
            bluetooth.println("Error, try again.");
            clignoterLED(255, 0, 0, 3); // Rouge
            delay(500);
        }
    }
}

void distributeFood()
{
    setLEDColor(255, 255, 0); // Jaune
    buzzer.playWeWishYou();
    clignoterLED(255, 255, 0, 3); // Jaune
    for (int i = 0; i < p; i++)
    {
        setLEDColor(255, 255, 0); // Jaune
        myServo.write(moveAngle);
        delay(1000);
        myServo.write(stopAngle);
        delay(2000);
        myServo.write(-moveAngle);
        delay(450);
        myServo.write(stopAngle);
        delay(3000);
    }
    setLEDColor(0, 255, 0); // Vert
    delay(1000);
    setLEDColor(255, 255, 0); // Jaune
}

void setLEDColor(int green, int blue, int red)
{
    analogWrite(GRN, green);
    analogWrite(BLU, blue);
    analogWrite(RED, red);
}

void clignoterLEDB(int red, int green, int blue, int time)
{
    for (int i = 0; i < time; ++i)
    {
        setLEDColor(red, green, blue);
        tone(buzzerPin, 600);
        delay(500);
        tone(buzzerPin, 900);
        setLEDColor(0, 0, 0); // Nothing
        delay(500);
        noTone(buzzerPin);
        delay(500);
    }
}

void clignoterLED(int red, int green, int blue, int time)
{
    for (int i = 0; i < time; ++i)
    {
        setLEDColor(red, green, blue);
        delay(500);
        setLEDColor(0, 0, 0); // Nothing
        delay(500);
    }
}

void processHours(String hourData)
{
    int startIndex = 0;
    int separatorIndex = 0;

    while ((separatorIndex = hourData.indexOf(',', startIndex)) != -1)
    {
        String hour = hourData.substring(startIndex, separatorIndex);
        Serial.println("Heure: " + hour);

        if (hourCount < MAX_FEEDS)
        {
            feedHours[hourCount] = hour.toInt();
            EEPROM.put(10 + hourCount * sizeof(int), feedHours[hourCount]);
            hourCount++;
        }

        startIndex = separatorIndex + 1;
    }

    String lastHour = hourData.substring(startIndex);
    Serial.println("Heure: " + lastHour);

    if (hourCount < MAX_FEEDS)
    {
        feedHours[hourCount] = lastHour.toInt();
        EEPROM.put(10 + hourCount * sizeof(int), feedHours[hourCount]);
        hourCount++;
    }
}

void processMinutes(String minuteData)
{
    int startIndex = 0;
    int separatorIndex = 0;

    while ((separatorIndex = minuteData.indexOf(',', startIndex)) != -1)
    {
        String minute = minuteData.substring(startIndex, separatorIndex);
        Serial.println("Minutes: " + minute);

        if (minuteCount < MAX_FEEDS)
        {
            feedMinutes[minuteCount] = minute.toInt();
            EEPROM.put(50 + minuteCount * sizeof(int), feedMinutes[minuteCount]);
            minuteCount++;
        }

        startIndex = separatorIndex + 1;
    }

    String lastMinute = minuteData.substring(startIndex);
    Serial.println("Minutes: " + lastMinute);

    if (minuteCount < MAX_FEEDS)
    {
        feedMinutes[minuteCount] = lastMinute.toInt();
        EEPROM.put(50 + minuteCount * sizeof(int), feedMinutes[minuteCount]);
        minuteCount++;
    }
}

void readSerialPort()
{
    msg = "";
    while (bluetooth.available())
    {
        delay(10);
        if (bluetooth.available() > 0)
        {
            char c = bluetooth.read();
            msg += c;
        }
    }
}

// By Sim - 2024
