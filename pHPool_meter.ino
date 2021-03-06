/**********************************************************
pHPool Meter v1.0 by Marc Cobler
marccobler.es
25/12/2013

Este es el código para el PHimetro portátil. Utiliza un Arduino Micro, una pantalla LCD 16x2,
un par de botones y switches y el kit de la sonda de PH con una sonda de temperatura 00018B20.
el kit de pH lo podemos encontrar en: 
http://www.dfrobot.com/index.php?route=product/product&path=36_68&product_id=1025#.Urq5MfTuJHU

el kit de la probeta tiene un wiki con toda la documentación necesaria:
http://dfrobot.com/wiki/index.php/PH_meter(SKU:_SEN0161)

La sonda de temperatura es digital. Tiene una resolucion configurable de 9 a 12 bits y un identificador interno de
64 bits para poder utilizar varias sondas en un mismo pin. Necesita una resistencia de 4,7K para funcionar.

Este código esta basado en el proyecto que hizo Sparkfun Electronics y el el código de ejemplo
proporcionado por DFRobot. Tambien se ha utilizado trozos de codigo de bildr para utilizar la libreria de OneWire.h 
correctamente.
http://bildr.org/2011/07/ds18b20-arduino/

************************************************************/
#include <LiquidCrystal.h> // Añadimos la libreria de adafruit(mejorada) para interactuar con la pantalla
#include "Wire.h" //Libreria necesaria. Permite comunicar con dispositivos mediante I2C/TWI. No la utilizamos
#include <OneWire.h> //Libreria de Dallas Semiconductor (Maxim) para la comunicación con la sonda de temperatura

const int sensorPH = A0;
OneWire sondaTemperatura(9); // Creamos una instancia de la clase oneWire. la comunicacion ira en el pin 9

unsigned long int valorMedio;
int buffer[10]; // array de valores del sensor de pH para luego calcular la media

int boton1 = 7; // Boton de calibracion
int boton2 = 8; // Boton de lectura de datos - TEST

LiquidCrystal lcd(12,11,6,5,4,3); // Creamos una varible LCD; syntax lcd(Register Select, Enable, D4,D5,D6,D7)

//---------------------------------------------------------------------------------------------
void setup()
{
 pinMode(boton1, INPUT); //Hacemos que el boton 1 sea una entrada
 digitalWrite(boton1, HIGH); //Ponemos el pin HIGH y asi se activa la Resistencia Pull-Up interna del ATmega
 pinMode(boton2, INPUT); //Este boton tambien es una entrada
 digitalWrite(boton2, HIGH); //Activamos la resistenca Pull-UP de este pin tambien
 
 pinMode(sensorPH, INPUT); //incializamos el pin analogico 0 como entrada de datos
 lcd.begin(16,2); // Inicializamos la pantalla con 16 columnas y 2 filas
 
 Serial.begin(9600); //inciamos el monitor serial para depuracion
}
//----------------------------------------------------------------------------------------------
void loop()
{
 if(digitalRead(boton1) == LOW) // Si se presiona el boton 1 se inicia la calibracion
  {
   lcd.clear(); // Borramos cualquier contenido de la pantalla y nos situamos en la posicion inicial
   lcd.setCursor(3,0); // Situamos el cursor en la primera linea y columna 3
   lcd.print("Calibrando...");
   calibrar(); // Llamamos a la funcion que calibra la sonda. Esta ya escribe lo necesario en pantalla
   delay(500);
  } 
  
 if(digitalRead(boton2) == LOW) // Si se presiona el boton 2 se hace la lectura del pH
  {
    lcd.clear(); // Borramos el contenido de la pantalla
    lcd.setCursor(3,0); // Situamos el cursor en la primera linea y columna 3
    lcd.print("Temp=");
    lcd.print(obtenerTemperatura(),1); // Llamamos a la funcion que obtiene la temperatura y la escribimos en la LCD
    lcd.print("ºC"); // En grados Celsius
    lcd.setCursor(3,1); // Situamos el cursor en la segunda linea y columna 3
    lcd.print("pH=");
    lcd.print(obtenerPH()); // Obtenemos el valor del pH y lo mostramos en pantalla
    delay(500);
  }   

}

//----------------------------------------------------------------------------------------------
//----------------------------------FUNCIONES---------------------------------------------------
void calibrar() 
{
  // Esta función calibra la sonda de pH con la temperatura leida del sensor de Temperatura
  delay(500);
  lcd.clear();
  lcd.setCursor(3,0);
  lcd.print("Completado!");
  lcd.setCursor(3,1);
  lcd.print("Temp=");
  lcd.print(obtenerTemperatura(),1);
  lcd.print("ºC");
  delay(500);
}
//----------------------------------------------------------------------------------------------
float obtenerTemperatura()
{
 //Esta funcion devuelve la temperatura de la sonda en grados Celsius
 byte data[12];
 byte addr[8];

 if ( !sondaTemperatura.search(addr)) {
   //no more sensors on chain, reset search
   sondaTemperatura.reset_search();
   return -1000;
 }

 if ( OneWire::crc8( addr, 7) != addr[7]) {
   Serial.println("CRC is not valid!");
   return -1000;
 }

 if ( addr[0] != 0x10 && addr[0] != 0x28) {
   Serial.print("Device is not recognized");
   return -1000;
 }

 sondaTemperatura.reset();
 sondaTemperatura.select(addr);
 sondaTemperatura.write(0x44,1); // start conversion, with parasite power on at the end

 byte present = sondaTemperatura.reset();
 sondaTemperatura.select(addr);  
 sondaTemperatura.write(0xBE); // Read Scratchpad

 
 for (int i = 0; i < 9; i++) { // we need 9 bytes
  data[i] = sondaTemperatura.read();
 }
 
 sondaTemperatura.reset_search();
 
 byte MSB = data[1];
 byte LSB = data[0];

 float tempRead = ((MSB << 8) | LSB); //using two's compliment
 float TemperatureSum = tempRead / 16;
 
 return TemperatureSum;
}
//----------------------------------------------------------------------------------------------
float obtenerPH()
{
  //Obtenemos 10 valores de muestra, para suavizar el valor
  int x; //valor temporal para ordenar los valores en orden creciente
  float offset = 0; //Ajustamos la medicion con un offset
  for(int i=0; i<10; i++) //Guardamos los 10 valores en el array buffer[]
  {
   buffer[i]=analogRead(sensorPH);
   delay(10);
  }
  for(int i=0; i<9; i++) //ordenamos los 10 valores tomados de menor a mayor
  {
   for(int j=i+1; j<10; j++)
    {
     if(buffer[i]>buffer[j])
       {
        x=buffer[i];
        buffer[i]=buffer[j];
        buffer[j]=x; 
       }
    } 
  }
  
  valorMedio = 0;
  for(int i=2; i<8; i++){
   valorMedio+=buffer[i]; 
  }
  float valorPH = (float)valorMedio*5.0/1024/6; //Convertimos la señal analogica en milivoltios
  valorPH = 3.5*valorPH+offset; //Ahora lo pasamos a PH
  return valorPH;
}
//----------------------------------------------------------------------------------------------
