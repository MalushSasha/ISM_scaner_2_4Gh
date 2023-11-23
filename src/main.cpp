#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include "nRF24L01.h"
#include <Adafruit_GFX.h>    // Включаємо бібліотеку для роботи з графікою
#include <Adafruit_ST7735.h> // Включаємо бібліотеку для дисплея ST7735


#define TFT_RST    27  // Пін скидання (RESET), -1 якщо не використовується
#define TFT_CS     26   // Пін вибору (CS)
#define TFT_DC     12   // Пін команд/даних (DC)
#define TFT_SCLK   14  // Альтернативний пін SPI Clock
#define TFT_MOSI   13  // Альтернативний пін SPI MOSI

                                  //************ змінні які треба змінити в залежності від опрошених датчиків*******
int knopka_pin = 33;
int led = 32;
int CE = 22; // GPIO для підключення радіо модуля
int CSN = 5; //GPIO для підключення радіо модуля
const int buzzerPin = 15; // Номер піна зумера

                                                          //***********************

// Об'єкт для роботи з дисплеєм
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// #define SPI_FREQUENCY  27000000 // Фактично встановлює 26,67 МГц = 80/3

RF24 radio(CE, CSN); 


       uint8_t  Font_size                = 1;                                                               // Встановлюємо розмір шрифту
       uint8_t  Loading                  = 0;                                                               // перший прохід не зберігається
       uint8_t  ChannelScan              = 0;                                                               // номер сканованого каналу
       uint8_t  ChannelPrint             = 1;                                                               // номер каналу 
       uint8_t  ChannelPowerNow[128];                                                                       // поточна потужність сигналів на кожному каналі
       uint8_t  ChannelPowerMax[128];                                                                       // максимальна потужність сигналів на кожному каналі
       uint8_t  ChannelPower[128];                                                                          // нормальна потужність сигналів на кожному каналі
       uint8_t  ChannelAnomali[128];                                                                        //Аномальна потужність
       uint16_t LoadingNormal            = 0;                                                              //Калібрування нормального значення каналу
       uint16_t ScanCycle                = 400;
       uint16_t PositionPoints[12];                                                                         // положення точок на дисплеї
const  uint16_t ColorBG          = ST7735_BLACK;                                                               // колір фону
const  uint16_t ColorScale       = ST7735_BLUE;                                                                // колір шкали та векторів графіка
const  uint16_t ColorTextLoad    = ST7735_WHITE;                                                               // колір тексту, що відображає відсотки при старті
const  uint16_t ColorGraphNow    = ST7735_WHITE;                                                               // колір графіка, що відображає поточні значення
const  uint16_t ColorGraphMax    = ST7735_RED;                                                                 // колір графіка, що відображає максимальні значення
const  uint16_t ColorNormal      = ST7735_GREEN;
const  uint16_t PositionLeft     = 10;                                                                      // крайнє ліве положення графіка на дисплеї
const  uint16_t PositionWidth    = 1;                                                                       // ширина одного каналу на дисплеї
const  uint16_t PositionTop      = 5;                                                                      // крайнє верхнє положення графіка на дисплеї
const  uint16_t PositionBottom   = 128;                                                                     // крайнє нижнє положення графіка на дисплеї
const  uint8_t  PositionHeight1  = 3;                                                                       // розмір кожної лінії каналів на нижній шкалі графіка
const  uint8_t  PositionHeight5  = 9;                                                                       // розмір кожної 5 лінії каналів на нижній шкалі графіка
const  uint8_t  PositionHeight10 = 15;                                                                      // розмір кожної 10 лінії каналів на нижній шкалі графіка
const  uint8_t  FG      = 3;                                                                                // множник значень графіка
const  uint16_t DiapazonChannel =  128;                                                                     // Кількість каналів радіо модуля

void setup(){
    Serial.begin(115200);
    pinMode(knopka_pin, INPUT_PULLUP);  // Встановлення підтяжки
    pinMode(led, OUTPUT);
    pinMode(buzzerPin, OUTPUT);
    ledcSetup(0, 5000, 8); // Ініціалізація LEDC для каналу 0
    ledcAttachPin(buzzerPin, 0); // Призначення каналу 0 піну зумера
//  підготовляємо дисплей та модуль до роботи
    tft.initR(INITR_BLACKTAB);                   // ініціюємо дисплей
    tft.fillScreen(ColorBG);                    //заливка кольором весь дисплей;
    tft.setTextSize(Font_size);                 // розмір шрифту   
    tft.setRotation(3);                         //поворот экрана от 0 до 3 
// ініціюємо роботу модуля nRF24L01+ 
    radio.begin();                                                                                          
    radio.setAutoAck(false);                                                                                // вимкнути режим підтвердження прийому
    memset(ChannelPowerNow, 0, sizeof(ChannelPowerNow));                                                    // заповнюємо всі елементи масиву ChannelPowerNow значенням 0
    memset(ChannelPowerMax, 0, sizeof(ChannelPowerMax));                                                    // заповнюємо всі елементи масиву ChannelPowerMax значенням 0
   
// Визначення розмірів шрифту
   char text[] = "W"; // Текст для вимірювання
    int16_t x, y; // Координати не важливі, оскільки ми не виводимо текст
    uint16_t w, h; // Визначаємо ширину та висоту прямокутника, який обмежує символ
    tft.getTextBounds(text, 0, 0, &x, &y, &w, &h);
  //**************

    PositionPoints[6]=PositionBottom-h;                                                 // положення тексту з номерами каналів
    PositionPoints[7]=PositionBottom-h*2;                                               // становище тексту з числами частот
    PositionPoints[8]=PositionBottom-h*2-PositionHeight10;                              // положення нульової координати по осі Y
//  виводимо нижню шкалу графіка для всього діапазону каналів
        tft.setTextColor(ColorScale);                                                                        // встановлюємо колір заливки шкали
    for(uint8_t i=0; i<128; i++){                                                                           // створюємо цикл по всьому діапазону каналів
        PositionPoints[9]=PositionLeft+i*PositionWidth;                                                     // положення координати чергового каналу по осі X
        tft.drawLine(PositionPoints[9],i?PositionPoints[8]:PositionTop,PositionPoints[9],PositionPoints[8]+(i%10?(i%5?PositionHeight1:PositionHeight5):PositionHeight10), ColorScale);  // виводимо вертикальні лінії шкали графіка
        if(i%20==0)                                                                        // % залишок від ділення на 20. 5 % 2 поверне 1
            {      // Виведення тексту на дисплей для кожного i, кратного 20
                int x = PositionPoints[9] - (i == 0 ? (w / 2) : (i < 100 ? w : w * 3 / 2));
                int y = PositionPoints[7];
                tft.setCursor(x, y);
                tft.print(i);
            }
        if(i%40==0)
         // виводимо текст - частоти каналів
        {
            int x = PositionPoints[9] - (i == 0 ? (w / 2) : (w * 2));
            int y = PositionPoints[6];
            tft.setCursor(x, y);
            tft.print((int)2400+i); 
        }
    }   tft.drawLine(PositionLeft,PositionPoints[8],(PositionLeft+128*PositionWidth),PositionPoints[8],ColorScale); // виводимо горизонтальну лінію
        tft.setCursor(PositionLeft+129*PositionWidth, PositionPoints[7]);  // Встановлюєм позицію курсору на екрані 
        tft.print("N");                             // виводимо назву шкали - номер каналу
        tft.setCursor(PositionLeft+129*PositionWidth, PositionPoints[6]);  // Встановлюєм позицію курсору на екрані 
        tft.print("MGz");                             // виводимо назву шкали - частота каналу
}
bool glushitu = 0;      //Натиснуто глушити аномальні канали
unsigned long prT200 = 0; // час попередньої перевірки
// Порівняння з нормою та реакція
void scanNormal(){
    //Запис масиву норми
    if(LoadingNormal<= ScanCycle ){
    ChannelPower[ChannelScan]=ChannelPowerNow[ChannelScan];
    Serial.println("#: " + String(ChannelScan) + "= " + String(ChannelPower[ChannelScan]));
    Serial.println("#: " + String(LoadingNormal) );
                   
                tft.fillRect((tft.width() ) / 2, PositionTop + 10, 160, 8, ColorBG); // Заповнюємо прямокутник фоновим кольором
                tft.setCursor((tft.width()) / 2, PositionTop + 10);
                tft.print((String)map(LoadingNormal,0,ScanCycle,0,100)+" %");                   //Виводим відсоток відсканованих каналів. 
                 
    }
    
//Реакція на перевищення норми
    if(LoadingNormal > ScanCycle ){
    int16_t temp = ChannelPowerNow[ChannelScan] - ChannelPower[ChannelScan];
        
            if (ChannelPower[ChannelScan] < 5 && temp > 4 ) //Зявився аномальний сигнал. При початковому рівні 0
            {
                ChannelAnomali[ChannelScan]=ChannelPowerNow[ChannelScan];
                Serial.println("0 рівеня #: " + String(ChannelScan)+ " = " + String(temp) );
            }else if(temp > 8){
                ChannelAnomali[ChannelScan]=ChannelPowerNow[ChannelScan];
                Serial.println("зайнятий канал #: " + String(ChannelScan)+ " = " + String(temp) );
            }else{ChannelAnomali[ChannelScan]=0;}
           
    }

    for(int i = 0; i < DiapazonChannel; i++){
    if(ChannelAnomali[i] >0 && glushitu==1){ // && (millis() - prT200) > 2000
    tone(buzzerPin, 523);
    digitalWrite(led, HIGH);
    prT200 = millis();
    
    }
  
    }

    if((millis() - prT200) > 200){
    noTone(buzzerPin);
    
    }



}

void deblokuvany(){
        if(glushitu==0){
        digitalWrite(led, LOW);
                for(int i = 0; i < DiapazonChannel; i++){
                    ChannelAnomali[i] = 0;
                    ChannelPowerMax[i] = 0;
                }
    }
}

byte address[][6] = {"1Node","2Node","3Node","4Node","5Node","6Node"};
void podavlenny(){
    // if(xx ==0){ 
         for(uint8_t i=0; i<128; i++){Serial.println("#Channel : " + String(i) + "= " + String(ChannelAnomali[i]));} //для налаштування. можна видалити.
    //      xx = 1;
    // }

            for (int i = 0; i < DiapazonChannel; i++) {
                if(ChannelAnomali[i] != 0){
                        radio.stopListening(); //переводить модуль в режим очікування відправки даних. 
                        radio.setPALevel(RF24_PA_HIGH);
                        // radio.openWritingPipe(address[1]);
                        radio.setChannel(i);  //вказуємо в якому каналі працювати. За замовченням 76 (0x4C). Всього 126.
                        radio.setDataRate (RF24_250KBPS); //Швидкість обміну. На вибір RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
                        // повинна бути однакова на приймачі та передавачі!
                        //за найнижчої швидкості маємо найвищу чутливість і дальність!!
                        long comand = 2078119397;    // дані відправленні в канал
                         
                            // radio.powerUp(); // Включити радіо передавач
                            radio.write(&comand, sizeof(comand));
                            

                        // ввімкнення радіо на читання
                        // radio.startListening();//переводить модуль в режим очікування прийому даних. 
                        // radio.powerDown(); // Вимкнути радіо передавач
                }
            }
    


}




void loop(){
    deblokuvany();
    glushitu = digitalRead(knopka_pin);
    // if(glushitu ==0 ){podavlenny();}
// скануємо черговий канал та зберігаємо його дані
    radio.setChannel(ChannelScan);                                                                            // встановлюємо черговий канал роботи модуля
    radio.startListening();                                                                                 // починаємо прослуховувати черговий канал
    delayMicroseconds(128);                                                                                  // Прослуховуємо канал задану кількість часу
    radio.stopListening();                                                                                  // зупиняємо прослуховування
    if(radio.testRPD()){ChannelPowerNow[ChannelScan]++; //testCarrier() або testRPD якщо сигнал на вказаному каналі мав потужність > -64 дБм, то збільшуємо значення масиву ChannelPowerNow для поточного каналу
    // Serial.println("#: " + String(ChannelScan) + "= " + String(ChannelPowerNow[ChannelScan]));
    }                                                
    if (ChannelPowerMax[ChannelScan]<ChannelPowerNow[ChannelScan]){                                         // якщо потужність відсканованого каналу перевищує раніше отримане максимальне значення цього каналу, то ...
        ChannelPowerMax[ChannelScan]=ChannelPowerNow[ChannelScan];                                          // зберігаємо потужність відсканованого каналу як максимальну
        scanNormal();
        // if(glushitu == 1){scanNormal();}      
        if(LoadingNormal<=ScanCycle){LoadingNormal++;}
    }   ChannelScan++;                                                                                     // збільшуємо номер сканованого каналу
//  виводимо отримані дані                                                                              // дані виводяться по одному каналу через повний цикл сканування всіх каналів
    if(ChannelScan==127){                                                                                   // якщо було проскановано останній можливий канал, то ...
        ChannelScan=0;                                                                                      // переходимо до 0 каналу
// якщо це перший прохід по діапазону каналів після запуску, то ...        
        if(Loading<=128){                                                                                    
            Loading++;                                                                                      // збільшуємо лічильник каналів
            
            tft.setTextColor(ColorTextLoad);                                                                 // встановлюємо колір тексту, що відображає відсотки при старті
             
            // if(Loading>=128)
            // {
            
            // tft.setCursor((tft.width()) / 2, PositionTop + 10);
            // tft.fillRect((tft.width() ) / 2, PositionTop + 10, 160, 8, ColorBG); // Заповнюємо прямокутник фоновим кольором. для стирання
            // }                                  
            // else{
                
            //     tft.fillRect((tft.width() ) / 2, PositionTop + 10, 160, 8, ColorBG); // Заповнюємо прямокутник фоновим кольором
            //     tft.setCursor((tft.width()) / 2, PositionTop + 10);
            //     tft.print((String)map(Loading,0,128,0,100)+" %");                   //Виводим відсоток відсканованих каналів. 
                 
            //     }                 
        }else{                                                                    
// якщо це не перший прохід діапазоном каналів, то ...
//         Розраховуємо положення точок графіків на дисплеї
            PositionPoints[0]=PositionPoints[8]-ChannelPowerMax[ChannelPrint-1]*FG; if(PositionPoints[0]<PositionTop){PositionPoints[0]=PositionTop;} // координата 1 точки по осі Y для графіка максимальних значень
            PositionPoints[1]=PositionPoints[8]-ChannelPowerMax[ChannelPrint  ]*FG; if(PositionPoints[1]<PositionTop){PositionPoints[1]=PositionTop;} // координата 2 точки по осі Y для графіка максимальних значень
            PositionPoints[2]=PositionPoints[8]-ChannelPowerNow[ChannelPrint-1]*FG; if(PositionPoints[2]<PositionTop){PositionPoints[2]=PositionTop;} // координата 1 точки по осі Y для графіка поточних значень
            PositionPoints[3]=PositionPoints[8]-ChannelPowerNow[ChannelPrint  ]*FG; if(PositionPoints[3]<PositionTop){PositionPoints[3]=PositionTop;} // координата 2 точки по осі Y для графіка поточних значень
            PositionPoints[4]=(ChannelPrint-1)*PositionWidth+PositionLeft;                                  // координата 1 точки по осі X для обох графіків
            PositionPoints[5]= ChannelPrint   *PositionWidth+PositionLeft;                                  // координата 2 точки по осі X для обох графіків
    PositionPoints[10]=PositionPoints[8]-ChannelPower[ChannelPrint-1]*FG; if(PositionPoints[10]<PositionTop){PositionPoints[10]=PositionTop;} // координата 1 точки по осі Y для графіка поточних значень
    PositionPoints[11]=PositionPoints[8]-ChannelPower[ChannelPrint  ]*FG; if(PositionPoints[11]<PositionTop){PositionPoints[11]=PositionTop;} // координата 2 точки по осі Y для графіка поточних значень
   // Виводимо графіки на дисплей
            tft.setTextColor(ColorBG);       tft.fillRect(PositionPoints[4]+1,0,PositionPoints[5],PositionPoints[8],ColorBG);                              // стираємо з дисплею частину попередніх графіків
            tft.setTextColor(ColorScale);    tft.drawLine(PositionPoints[4],PositionPoints[8],PositionPoints[5],PositionPoints[8],ColorScale);                // виводимо горизонтальну частину лінії шкали
            tft.setTextColor(ColorGraphMax); tft.drawLine(PositionPoints[4],PositionPoints[0],PositionPoints[5],PositionPoints[1],ColorGraphMax);                // виводимо графік максимальних даних
            tft.setTextColor(ColorGraphNow); tft.drawLine(PositionPoints[4],PositionPoints[2],PositionPoints[5],PositionPoints[3],ColorGraphNow);                // виводимо графік поточних даних

            tft.drawLine(PositionPoints[4],PositionPoints[10],PositionPoints[5],PositionPoints[11],ColorNormal);

        }   ChannelPowerNow[ChannelPrint-1]=0;                                                                     // стираємо показання потужності для щойно виведеного каналу
            ChannelPrint++; if(ChannelPrint==128){ChannelPrint=1;}                                          // збільшуємо номер каналу, що виводиться
    }
}