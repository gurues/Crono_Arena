/*

CRONÓMETRO DE ARENA ELECTRÓNICO

Material necesario:
  * Arduino nano
  * MPU-6050 (GY-521)
  * 2 Pantallas led MAX7219 FC16
  * 2 Condesadores de 10V y 330microFaradios o superior
  * 1 pulsador 
  * 1 Interruptor
  * 2 pilas 18650
  * Circuito de carga 18650 2S
  * Led de carga actual bateria
  * Porta baterias 18650 2S

Realizado por gurues@enero2023

*/

#include <Arduino.h>
#include <Adafruit_MPU6050.h>     // Control MPU-6050 GY-521
#include <Adafruit_Sensor.h>      // Control MPU-6050 GY-521
#include <Wire.h>                 // Control paneles led
#include <MD_MAX72xx.h>           // Control paneles led 
#include "Switch.h"               // Control del pulsador

// Comentar para anular el DEBUG serie -> (Por defecto desactivo: Muestra el paso por las funciones de control de los led MAX7219)
//#define ___DEBUG___
// Comentar para anular el DEBUG1 serie -> (Por defecto activo: MPU6050, tiempo de crono y configuración del pulsador)
#define ___DEBUG1___

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define NUM_OF_MATRIX 1

// Panel LED 1
#define CLK_PIN1   5
#define DATA_PIN1  3
#define CS_PIN1    4

// Panel LED 2
#define CLK_PIN2   6
#define DATA_PIN2  8
#define CS_PIN2    7

// Objeto pulsador
#define PIN_PULSADOR    9
Switch PulsadorSwitch = Switch(PIN_PULSADOR);
              
//Objetos paneles Led
MD_MAX72XX mx1 = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN1, CLK_PIN1, CS_PIN1, NUM_OF_MATRIX);
MD_MAX72XX mx2 = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN2, CLK_PIN2, CS_PIN2, NUM_OF_MATRIX);

//Objeto Acelerómetro / Giroscopio
Adafruit_MPU6050 mpu;

// Variables para el control MPU6050 por interrupciones
#define INTERRUPT_PIN 2       
volatile int state_mpu = LOW; // Variable de control interrupción

//Variables Globales del Programa
float aX, aY, aZ;             // variables MPU6050 acelerómetro ejes X, Y, Z
int posicion = 0;             // posición del Cronómetro de arena asociada a las ordenes 
int mem_posicion = 0;         // memoria de la posición para hacer un número de mediciones antes de asignar una posición
int repeticiones = 3;         // número de repeticiones con el mismo resultado en el MPU
int muestras = repeticiones;  // muestras para determinar la posición del Cronómetro de arena
int acel = 6;                 // valor de corte para decidir en que posición esta el Cronómetro de arena
int pase = 0;                 // variable de control de paso a 2º panel led
const int  DELAYTIME = 100;   // millisegundos de espera en el inicio
int Crono = 0;                // 0 equivale a 30 seg. Tiempo en segundos de cuenta Cronómetro de arena
bool parado = false;          // Estado del Cronómetro (parado / arrancado)
bool control_int = false;     // Control de las interrupciones del MPU

//Ajuste para calibrar el Cronómetro
unsigned long  init_time = 0;         // Inicio de la cuenta
unsigned long  end_time = 0;          // Fin de la cuenta
const unsigned long c_ajuste_0 = 1;   // Ajuste de tiempo fino para el cálculo de 30 segundos
const unsigned long c_ajuste_1 = 49;  // Ajuste de tiempo fino para el cálculo de 1 minuto
const unsigned long c_ajuste = 96;    // Ajuste de tiempo fino para el cálculo de los minutos
const unsigned long  c_Tiempo = 234;  // Ajuste de tiempo basto entre led's para cumplir el tiempo programado del Cronómetro  
                                      // de arena. Por Defecto 30 seg -> 234 Factor que equivale a 30 seg
unsigned long  ajuste = c_ajuste_0;   // Predeterminado 30 segundos
unsigned long  Tiempo = c_Tiempo;     // Tiempo basto entre led's
/*
____________________________________________________________________________________________________________________________________    
            
                                            FUNCIONES DEL PROGRAMA
____________________________________________________________________________________________________________________________________
*/

// Funciones de Control del MPU6050 / GY-521 -------------------------------------------------------------------------------------------

void Read_MPU() { // Se ejecuta cuando se produce la interrupción del MPU6050
  #ifdef ___DEBUG1___
    Serial.println("");
    Serial.println(" ------- Producida Interrupción MPU6050 --------");
    Serial.println("");
  #endif
  detachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN)); 
  control_int = false;
  // Solo se permiten interrupciones si está Parado el Cronómetro
  if (((posicion == 3)||(posicion == 4)||(posicion == 1)||(posicion == 6))&&(parado)){ 
    state_mpu = !state_mpu;
    posicion = 0;
  }
  // Se permiten interrupciones en estas posiciones, da igual el estado del Cronómetro
  if ((posicion == 0)||(posicion == 2)||(posicion == 5)){
    state_mpu = !state_mpu;
    posicion = 0;
  }
}

void inicio_MPU(){
  // Inicializando MPU6050
  if (!mpu.begin()) {
    #ifdef ___DEBUG1___
      Serial.println("Fallo no encontrado MPU6050 ..........");
    #endif
    while (1) {
      delay(10);
    }
  }
  #ifdef ___DEBUG1___
    Serial.println("");
    Serial.println("Encontrado MPU6050");
    Serial.println("");
  #endif
  // Configuración GY-521 para el control por interrupciones
  mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
  mpu.setMotionDetectionThreshold(5);
  mpu.setMotionDetectionDuration(20);
  mpu.setInterruptPinLatch(true);	
  mpu.setInterruptPinPolarity(true);
  mpu.setMotionInterrupt(true);
  #ifdef ___DEBUG1___
    Serial.println("");
  #endif
  delay(100);
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), Read_MPU, LOW);
  control_int = true;
  #ifdef ___DEBUG1___
    Serial.println("-----  MPU6050 Iniciado -> Esperando Interrupción  -----");
  #endif
}

// Funciones de Control de los Paneles LED MAX7219  ------------------------------------------------------------------------------_____

void start_crono_2();
void start_crono_4();

void init_crono_1(){  // Inicio Panel Led Nº1

  #ifdef ___DEBUG___
    Serial.println("init_crono_1() -> Inicio Panel 1 MAX7219");
  #endif

  parado = false;

  for (int i=1; i<=Crono; i++){
    mx1.control(MD_MAX72XX::TEST, true);
    delay(500);
    mx1.control(MD_MAX72XX::TEST, false);
    delay(500);
  }
  
  mx1.clear();

  // Se inicia el Cronómetro
  if (NUM_OF_MATRIX > 1)
    mx1.setChar((2*COL_SIZE)-1, '0');
  for (uint8_t f=0; f<ROW_SIZE; f++){
    for (uint8_t c=0; c<COL_SIZE; c++){
      mx1.setPoint(f, c, true);
      delay(DELAYTIME);
    }
  }

  delay(DELAYTIME*3);
}

void init_crono_2(){  // Inicio Panel Led Nº1
  
  #ifdef ___DEBUG___
    Serial.println("init_crono_2() -> Inicio Panel 2 MAX7219");
  #endif

  parado = false;

  for (int i=1; i<=Crono; i++){
    mx2.control(MD_MAX72XX::TEST, true);
    delay(500);
    mx2.control(MD_MAX72XX::TEST, false);
    delay(500);
  }
  mx2.clear();

  // Se inicia el Cronómetro 
  if (NUM_OF_MATRIX > 1)
    mx2.setChar((2*COL_SIZE)-1, '0');
  for (uint8_t f=0; f<ROW_SIZE; f++){
    for (uint8_t c=0; c<COL_SIZE; c++){
      mx2.setPoint(f, c, true);
      delay(DELAYTIME);
    }
  }

  delay(DELAYTIME*3);
}

void start_crono_1(){ // Inicio Cuenta Cronómetro Posición Verical -> Panel Nº1
  
  #ifdef ___DEBUG___
    Serial.println("start_crono_1() -> Cronómetro de Arena Vertical");
  #endif

  //Comienza la cuenta
  init_time = millis();  // Tiempo inicial Cronómetro
  uint8_t k=0;
  uint8_t f=0;
  uint8_t c=0;
  
  //Apaga 1ª media pantalla
  for (k=0; k<ROW_SIZE; k++){
    #ifdef ___DEBUG___
      Serial.print("k= "); Serial.println(k);
    #endif
    if (state_mpu)
      break;
    c=COL_SIZE;
    for (f=7-k; f<ROW_SIZE; f++){
      if (state_mpu)
        break;
      #ifdef ___DEBUG___
        Serial.print(" --> f= "); Serial.println(f);
      #endif
       c--;
      #ifdef ___DEBUG___
        Serial.print("  ---> c= "); Serial.println(c);
      #endif
      if (!(parado))  // Continua la cuenta
        mx1.setPoint(f, c, false);
      PulsadorSwitch.poll();
      pase++;
      if (!(parado)){ // Continua la cuenta
        delay (Tiempo);
        PulsadorSwitch.poll();
        start_crono_2();
        PulsadorSwitch.poll();
      }
      if (parado){ // Se para la cuenta
        f--;
        c++;
        pase--;
        if (!(control_int))
          inicio_MPU();
      }
    }
    if (parado)
      k--;
  }

  k=0;
  f=0;
  c=0;
  //Apaga la 2ª media pantalla
  for (k=0; k<7; k++){
    if (state_mpu)
      break;
    #ifdef ___DEBUG___
      Serial.print("k= "); Serial.println(k);
    #endif
    c=7-k;
    for (f=0; f<7-k; f++){
      if (state_mpu)
        break;
      #ifdef ___DEBUG___
        Serial.print(" --> f= "); Serial.println(f);
      #endif
      c--;
      #ifdef ___DEBUG___
        Serial.print("  ---> c= "); Serial.println(c);
      #endif
      if (!(parado))  // Continua la cuenta
        mx1.setPoint(f, c, false);
      PulsadorSwitch.poll();
      pase++;
      if (!(parado)){ // Continua la cuenta
        delay (Tiempo);
        PulsadorSwitch.poll();
        start_crono_2();
        PulsadorSwitch.poll();
      }
      if (parado){ // Se para la cuenta
        f--;
        c++;
        pase--;
       if (!(control_int))
          inicio_MPU();
      }
    }
    if (parado)
      k--;
  }
  pase=0;
}

void start_crono_2(){ // Inicio Cuenta Cronómetro Posición Verical -> Panel Nº2

  #ifdef ___DEBUG___
    Serial.print("start_crono_2() --------------------   pase="); Serial.println(pase);
  #endif
  //Ilumina la linea central
  if (pase<=8){
  mx2.setPoint(ROW_SIZE-pase, COL_SIZE-pase, true);
      delay (Tiempo);
  }
  //Resto de puntos
  if (pase==9){
    mx2.setPoint(0, 1, true);
    delay (Tiempo);
  }
  if (pase==10){
    mx2.setPoint(1, 0, true);
    delay (Tiempo);
  }
  if (pase==11){
    mx2.setPoint(0, 2, true);
    delay (Tiempo);
  }
  if (pase==12){
    mx2.setPoint(2, 0, true);
    delay (Tiempo);
  }
  if (pase==13){
    mx2.setPoint(1, 2, true);
    delay (Tiempo);
  }
  if (pase==14){
    mx2.setPoint(2, 1, true);
    delay (Tiempo);
  }
  if (pase==15){
    mx2.setPoint(0, 3, true);
    delay (Tiempo);
  }
  if (pase==16){
    mx2.setPoint(3, 0, true);
    delay (Tiempo);
  }
  if (pase==17){
    mx2.setPoint(1, 3, true);
    delay (Tiempo);
  }
  if (pase==18){
    mx2.setPoint(3, 1, true);
    delay (Tiempo);
  }
  if (pase==19){
    mx2.setPoint(0, 4, true);
    delay (Tiempo);
  }
  if (pase==20){
    mx2.setPoint(4, 0, true);
    delay (Tiempo);
  }
  if (pase==21){
    mx2.setPoint(2, 3, true);
    delay (Tiempo);
  }
  if (pase==22){
    mx2.setPoint(3, 2, true);
    delay (Tiempo);
  }
  if (pase==23){
    mx2.setPoint(1, 4, true);
    delay (Tiempo);
  }
  if (pase==24){
    mx2.setPoint(4, 1, true);
    delay (Tiempo);
  }
  if (pase==25){
    mx2.setPoint(0, 5, true);
    delay (Tiempo);
  }
  if (pase==26){
    mx2.setPoint(5, 0, true);
    delay (Tiempo);
  }
  if (pase==27){
    mx2.setPoint(2, 4, true);
    delay (Tiempo);
  }
  if (pase==28){
    mx2.setPoint(4, 2, true);
    delay (Tiempo);
  }
  if (pase==29){
    mx2.setPoint(1, 5, true);
    delay (Tiempo);
  }
  if (pase==30){
    mx2.setPoint(5, 1, true);
    delay (Tiempo);
  }
  if (pase==31){
    mx2.setPoint(0, 6, true);
    delay (Tiempo);
  }
  if (pase==32){
    mx2.setPoint(6, 0, true);
    delay (Tiempo);
  }
  if (pase==33){
    mx2.setPoint(3, 4, true);
    delay (Tiempo);
  }
  if (pase==34){
    mx2.setPoint(4, 3, true);
    delay (Tiempo);
  }
  if (pase==35){
    mx2.setPoint(2, 5, true);
    delay (Tiempo);
  }
  if (pase==36){
    mx2.setPoint(5, 2, true);
    delay (Tiempo);
  }
  if (pase==37){
    mx2.setPoint(6, 1, true);
    delay (Tiempo);
  }
  if (pase==38){
    mx2.setPoint(1, 6, true);
    delay (Tiempo);
  }
  if (pase==39){
    mx2.setPoint(0, 7, true);
    delay (Tiempo);
  }
  if (pase==40){
    mx2.setPoint(7, 0, true);
    delay (Tiempo);
  }
  if (pase==41){
    mx2.setPoint(3, 5, true);
    delay (Tiempo);
  }
  if (pase==42){
    mx2.setPoint(5, 3, true);
    delay (Tiempo);
  }
  if (pase==43){
    mx2.setPoint(2, 6, true);
    delay (Tiempo);
  }
  if (pase==44){
    mx2.setPoint(6, 2, true);
    delay (Tiempo);
  }
  if (pase==45){
    mx2.setPoint(1, 7, true);
    delay (Tiempo);
  }
  if (pase==46){
    mx2.setPoint(7, 1, true);
    delay (Tiempo);
  }
  if (pase==47){
    mx2.setPoint(4, 5, true);
    delay (Tiempo);
  }
  if (pase==48){
    mx2.setPoint(5, 4, true);
    delay (Tiempo);
  }
  if (pase==49){
    mx2.setPoint(3, 6, true);
    delay (Tiempo);
  }
  if (pase==50){
    mx2.setPoint(6, 3, true);
    delay (Tiempo);
  }
  if (pase==51){
    mx2.setPoint(2, 7, true);
    delay (Tiempo);
  }
  if (pase==52){
    mx2.setPoint(7, 2, true);
    delay (Tiempo);
  }
  if (pase==53){
    mx2.setPoint(4, 6, true);
    delay (Tiempo);
  }
  if (pase==54){
    mx2.setPoint(6, 4, true);
    delay (Tiempo);
  }
  if (pase==55){
    mx2.setPoint(3, 7, true);
    delay (Tiempo);
  }
  if (pase==56){
    mx2.setPoint(7, 3, true);
    delay (Tiempo);
  }
  if (pase==57){
    mx2.setPoint(5, 6, true);
    delay (Tiempo);
  }
  if (pase==58){
    mx2.setPoint(6, 5, true);
    delay (Tiempo);
  }
  if (pase==59){
    mx2.setPoint(4, 7, true);
    delay (Tiempo);
  }
  if (pase==60){
    mx2.setPoint(7, 4, true);
    delay (Tiempo);
  }
  if (pase==61){
    mx2.setPoint(5, 7, true);
    delay (Tiempo);
  }
  if (pase==62){
    mx2.setPoint(7, 5, true);
    delay (Tiempo);
  }
  if (pase==63){
    mx2.setPoint(6, 7, true);
    delay (Tiempo);
  }
  if (pase==64){
    delay(ajuste);
    mx2.setPoint(7, 6, true);
    delay (Tiempo);
    end_time = millis();  // Tiempo final Cronómetro
  }

}

void start_crono_3(){ // Inicio Cuenta Cronómetro Posición Verical -> Panel Nº2

  #ifdef ___DEBUG___
    Serial.println("start_crono_3() -> Cronómetro de Arena Vertical");
  #endif

  //Comienza la cuenta
  init_time = millis();  // Tiempo inicial Cronómetro
  uint8_t k=0;
  uint8_t f=0;
  uint8_t c=0;
  
  //Apaga 1ª media pantalla
  for (k=7; k>0; k--){
    if (state_mpu)
      break;
    #ifdef ___DEBUG___
      Serial.print("k= "); Serial.println(k);
    #endif
    c=COL_SIZE-k;
    for (f=0; f<COL_SIZE-k; f++){
      if (state_mpu)
        break;
      #ifdef ___DEBUG___
        Serial.print(" --> f= "); Serial.println(f);
      #endif
       c--;
      #ifdef ___DEBUG___
        Serial.print("  ---> c= "); Serial.println(c);
      #endif
      if (!(parado))  // la cuenta continua
        mx2.setPoint(f, c, false);
      PulsadorSwitch.poll();
      pase++;
      if (!(parado)){ // la cuenta continua
        delay (Tiempo);
        PulsadorSwitch.poll();
        start_crono_4();
        PulsadorSwitch.poll();
      }
      if (parado){ // Se para la cuenta
        f--;
        c++;
        pase--;
        if (!(control_int))
          inicio_MPU();
      }
    }
    if (parado)
      k++;
  }

  k=0;
  f=0;
  c=0;
  //Apaga la 2ª media pantalla
  for (k=0; k<COL_SIZE; k++){
    if (state_mpu)
      break;
    #ifdef ___DEBUG___
      Serial.print("k= "); Serial.println(k);
    #endif
    c=COL_SIZE;
    for (f=k; f<ROW_SIZE; f++){
      if (state_mpu)
        break;
      #ifdef ___DEBUG___
        Serial.print(" --> f= "); Serial.println(f);
      #endif
      c--;
      #ifdef ___DEBUG___
        Serial.print("  ---> c= "); Serial.println(c);
      #endif
      if (!(parado))  // la cuenta continua
        mx2.setPoint(f, c, false);
      PulsadorSwitch.poll();
      pase++;
      if (!(parado)){ // la cuenta continua
        delay (Tiempo);
        PulsadorSwitch.poll();
        start_crono_4();
        PulsadorSwitch.poll();
      }
      if (parado){  // se para la cuenta
        f--;
        c++;
        pase--;
        if (!(control_int))
          inicio_MPU();
      }
    }
    if (parado) // se para la cuenta
      k--;
  }
  pase=0;
}

void start_crono_4(){ // Inicio Cuenta Cronómetro Posición Verical -> Panel Nº1

  #ifdef ___DEBUG___
    Serial.print("start_crono_4() --------------------   pase="); Serial.println(pase);
  #endif
  //Ilumina la linea central
  if (pase<=8){
    mx1.setPoint(pase-1, pase-1, true);
    delay (Tiempo);
  }
  //Resto de puntos 
  if (pase==9){
    mx1.setPoint(6, 7, true);
    delay (Tiempo);
  }
  if (pase==10){
    mx1.setPoint(7, 6, true);
    delay (Tiempo);
  }
  if (pase==11){
    mx1.setPoint(5, 7, true);
    delay (Tiempo);
  }
  if (pase==12){
    mx1.setPoint(7, 5, true);
    delay (Tiempo);
  }
  if (pase==13){
    mx1.setPoint(5, 6, true);
    delay (Tiempo);
  }
  if (pase==14){
    mx1.setPoint(6, 5, true);
    delay (Tiempo);
  }
  if (pase==15){
    mx1.setPoint(4, 7, true);
    delay (Tiempo);
  }
  if (pase==16){
    mx1.setPoint(7, 4, true);
    delay (Tiempo);
  }
  if (pase==17){
    mx1.setPoint(4, 6, true);
    delay (Tiempo);
  }
  if (pase==18){
    mx1.setPoint(6, 4, true);
    delay (Tiempo);
  }
  if (pase==19){
    mx1.setPoint(3, 7, true);
    delay (Tiempo);
  }
  if (pase==20){
    mx1.setPoint(7, 3, true);
    delay (Tiempo);
  }
  if (pase==21){
    mx1.setPoint(4, 5, true);
    delay (Tiempo);
  }
  if (pase==22){
    mx1.setPoint(5, 4, true);
    delay (Tiempo);
  }
  if (pase==23){
    mx1.setPoint(3, 6, true);
    delay (Tiempo);
  }
  if (pase==24){
    mx1.setPoint(6, 3, true);
    delay (Tiempo);
  }
  if (pase==25){
    mx1.setPoint(2, 7, true);
    delay (Tiempo);
  }
  if (pase==26){
    mx1.setPoint(7, 2, true);
    delay (Tiempo);
  }
  if (pase==27){
    mx1.setPoint(3, 5, true);
    delay (Tiempo);
  }
  if (pase==28){
    mx1.setPoint(5, 3, true);
    delay (Tiempo);
  }
  if (pase==29){
    mx1.setPoint(2, 6, true);
    delay (Tiempo);
  }
  if (pase==30){
    mx1.setPoint(6, 2, true);
    delay (Tiempo);
  }
  if (pase==31){
    mx1.setPoint(1, 7, true);
    delay (Tiempo);
  }
  if (pase==32){
    mx1.setPoint(7, 1, true);
    delay (Tiempo);
  }
  if (pase==33){
    mx1.setPoint(3, 4, true);
    delay (Tiempo);
  }
  if (pase==34){
    mx1.setPoint(4, 3, true);
    delay (Tiempo);
  }
  if (pase==35){
    mx1.setPoint(2, 5, true);
    delay (Tiempo);
  }
  if (pase==36){
    mx1.setPoint(5, 2, true);
    delay (Tiempo);
  }
  if (pase==37){
    mx1.setPoint(1, 6, true);
    delay (Tiempo);
  }
  if (pase==38){
    mx1.setPoint(6, 1, true);
    delay (Tiempo);
  }
  if (pase==39){
    mx1.setPoint(7, 0, true);
    delay (Tiempo);
  }
  if (pase==40){
    mx1.setPoint(0, 7, true);
    delay (Tiempo);
  }
  if (pase==41){
    mx1.setPoint(2, 4, true);
    delay (Tiempo);
  }
  if (pase==42){
    mx1.setPoint(4, 2, true);
    delay (Tiempo);
  }
  if (pase==43){
    mx1.setPoint(1, 5, true);
    delay (Tiempo);
  }
  if (pase==44){
    mx1.setPoint(5, 1, true);
    delay (Tiempo);
  }
  if (pase==45){
    mx1.setPoint(0, 6, true);
    delay (Tiempo);
  }
  if (pase==46){
    mx1.setPoint(6, 0, true);
    delay (Tiempo);
  }
  if (pase==47){
    mx1.setPoint(2, 3, true);
    delay (Tiempo);
  }
  if (pase==48){
    mx1.setPoint(3, 2, true);
    delay (Tiempo);
  }
  if (pase==49){
    mx1.setPoint(1, 4, true);
    delay (Tiempo);
  }
  if (pase==50){
    mx1.setPoint(4, 1, true);
    delay (Tiempo);
  }
  if (pase==51){
    mx1.setPoint(0, 5, true);
    delay (Tiempo);
  }
  if (pase==52){
    mx1.setPoint(5, 0, true);
    delay (Tiempo);
  }
  if (pase==53){
    mx1.setPoint(1, 3, true);
    delay (Tiempo);
  }
  if (pase==54){
    mx1.setPoint(3, 1, true);
    delay (Tiempo);
  }
  if (pase==55){
    mx1.setPoint(0, 4, true);
    delay (Tiempo);
  }
  if (pase==56){
    mx1.setPoint(4, 0, true);
    delay (Tiempo);
  }
  if (pase==57){
    mx1.setPoint(1, 2, true);
    delay (Tiempo);
  }
  if (pase==58){
    mx1.setPoint(2, 1, true);
    delay (Tiempo);
  }
  if (pase==59){
    mx1.setPoint(0, 3, true);
    delay (Tiempo);
  }
  if (pase==60){
    mx1.setPoint(3, 0, true);
    delay (Tiempo);
  }
  if (pase==61){
    mx1.setPoint(0, 2, true);
    delay (Tiempo);
  }
  if (pase==62){
    mx1.setPoint(2, 0, true);
    delay (Tiempo);
  }
  if (pase==63){
    mx1.setPoint(0, 1, true);
    delay (Tiempo);
  }
  if (pase==64){
    delay(ajuste);
    mx1.setPoint(1, 0, true);
    delay (Tiempo);
    end_time = millis();  // Tiempo final Cronómetro
  }

}

void start_crono_11(){  // Inicio Cuenta Cronómetro Posición horizontal -> Panel Nº1
  
  #ifdef ___DEBUG___
    Serial.println("start_crono_11() -> Cronómetro de Arena Horizontal");
  #endif

  //Comienza la cuenta
  init_time = millis(); // Tiempo inicial Cronómetro
  uint8_t k=0;
  uint8_t f=0;
  uint8_t c=0;
  
  //Apaga 1ª media pantalla
  for (k=0; k<ROW_SIZE-1; k++){
    #ifdef ___DEBUG___
      Serial.print("k= "); Serial.println(k);
    #endif
    if (state_mpu)
      break;
    c=COL_SIZE-k-2;
    for (f=0; f<=k; f++){
      if (state_mpu)
        break;
      #ifdef ___DEBUG___
        Serial.print(" --> f= "); Serial.println(f);
      #endif
       c++;
      #ifdef ___DEBUG___
        Serial.print("  ---> c= "); Serial.println(c);
      #endif
      PulsadorSwitch.poll();
      if (!(parado)){ // Continua la cuenta
        mx1.setPoint(f, c, false);
        delay (Tiempo);
        PulsadorSwitch.poll();
      }
      if (parado){ // Se para la cuenta
        f--;
        c--;
        if (!(control_int))
          inicio_MPU();
      }
    }
    if (parado) // Se para la cuenta
      k--;
  }

//Comienza la cuenta
  k=0;
  f=0;
  c=0;
  
  //Enciende 2ª media pantalla Panel Nº2
  for (k=0; k<ROW_SIZE; k++){
    #ifdef ___DEBUG___
      Serial.print("k= "); Serial.println(k);
    #endif
    if (state_mpu)
      break;
    c=COL_SIZE-k-2;
    for (f=0; f<=k; f++){
      if (state_mpu)
        break;
      #ifdef ___DEBUG___
        Serial.print(" --> f= "); Serial.println(f);
      #endif
       c++;
      #ifdef ___DEBUG___
        Serial.print("  ---> c= "); Serial.println(c);
      #endif
      PulsadorSwitch.poll();
      if (!(parado)){ // Continua la cuenta
        mx2.setPoint(c, f, true);
        delay (Tiempo);
        PulsadorSwitch.poll();
      }
      if (parado){ // Se para la cuenta
        f--;
        c--;
        if (!(control_int))
          inicio_MPU();
      }
    }
    if (parado) // Se para la cuenta
      k--;
  }
  delay(ajuste);
  end_time = millis();  // Tiempo final Cronómetro
}

void start_crono_22(){  // Inicio Cuenta Cronómetro Posición horizontal -> Panel Nº2

  #ifdef ___DEBUG___
    Serial.println("start_crono_22() -> Cronómetro de Arena Horizontal");
  #endif

  // Apaga 1ª media pantalla Panel Nº2
  init_time = millis(); // Tiempo inicial Cronómetro
  uint8_t k=0;
  uint8_t f=0;
  uint8_t c=0;
  
  for (k=0; k<ROW_SIZE-1; k++){
    #ifdef ___DEBUG___
      Serial.print("k= "); Serial.println(k);
    #endif
    if (state_mpu)
      break;
    c=COL_SIZE-k-2;
    for (f=0; f<=k; f++){
      if (state_mpu)
        break;
      #ifdef ___DEBUG___
        Serial.print(" --> f= "); Serial.println(f);
      #endif
       c++;
      #ifdef ___DEBUG___
        Serial.print("  ---> c= "); Serial.println(c);
      #endif
      PulsadorSwitch.poll();
      if (!(parado)){ // Continua la cuenta
        mx2.setPoint(c, f, false);
        delay (Tiempo);
        PulsadorSwitch.poll();
      }
      if (parado){ // Se para la cuenta
        f--;
        c--;
        if (!(control_int))
          inicio_MPU();
      }
    }
    if (parado) // Se para la cuenta
      k--;
  }

// Enciende 2ª media pantalla Panel Nº1
  k=0;
  f=0;
  c=0;

  for (k=0; k<ROW_SIZE; k++){
    #ifdef ___DEBUG___
      Serial.print("k= "); Serial.println(k);
    #endif
    if (state_mpu)
      break;
    c=COL_SIZE-k-2;
    for (f=0; f<=k; f++){
      if (state_mpu)
        break;
      #ifdef ___DEBUG___
        Serial.print(" --> f= "); Serial.println(f);
      #endif
       c++;
      #ifdef ___DEBUG___
        Serial.print("  ---> c= "); Serial.println(c);
      #endif
      PulsadorSwitch.poll();
      if (!(parado)){ // Continua la cuenta
        mx1.setPoint(f, c, true);
        delay (Tiempo);
        PulsadorSwitch.poll();
      }
      if (parado){ // Se para la cuenta
        f--;
        c--;
        if (!(control_int))
          inicio_MPU();
      }
    }
    if (parado) // Se para la cuenta
      k--;
  }
  delay(ajuste);
  end_time = millis();  // Tiempo final Cronómetro
}

// Funciones de Control Pulsador  --------------------------------------------------------------------------------------------------

void pushedCallbackFunction(void* s) {  // Pulsador -> Para y arranca el Cronómetro

  if (posicion == 4){
    parado = !parado;
    mx1.setPoint(7, 7, parado);
  }
  if (posicion == 3){
    parado = !parado;
    mx2.setPoint(0, 0, parado);
  }
  if (posicion == 1){
    parado = !parado;
    mx1.setPoint(0, 7, parado);
    mx2.setPoint(0, 7, parado);
  }
  if (posicion == 6){
    parado = !parado;
    mx1.setPoint(7, 0, parado);
    mx2.setPoint(7, 0, parado);
  }

  #ifdef ___DEBUG1___
    Serial.println("");
    Serial.print("Pulsación Corta: ");
    Serial.println((char*)s);
    if (parado)
      Serial.print("................. Cronometro de Arena detenido");
    else
      Serial.print("................. Cronometro de Arena Arrancado");
  #endif
}

void ReleasedCallbackCallbackFunction(void* s) {  // Pulsador -> Programa minutos de cuenta del Cronómetro
  if (posicion == 2){
    Tiempo = c_Tiempo;   
    ajuste = c_ajuste;   
    Crono++;
    mx2.control(MD_MAX72XX::TEST, true);
    delay(1000);
    mx2.control(MD_MAX72XX::TEST, false);
    if (Crono == 0 ){
      ajuste = c_ajuste_0;
    }
    if (Crono == 1 ){
      Tiempo = (2*Tiempo)*Crono; // 60 segundos*Crono = minutos
      ajuste = c_ajuste_1;
    }
    if (Crono > 1){
      Tiempo = (2*Tiempo)*Crono; // 60 segundos*Crono = minutos
      ajuste = c_ajuste_1+ajuste*(Crono-1);
    }
    #ifdef ___DEBUG1___
      Serial.println("");
      Serial.print("Pulsación relajada: ");
      Serial.println((char*)s);
      if (Crono == 0){
        Serial.print("Crono = "); Serial.println(Crono);
        Serial.print("Cte Tiempo = "); Serial.println(Tiempo);
        Serial.print("Cte Ajuste = "); Serial.println(c_ajuste_0);  
      }
      else if (Crono == 1){
        Serial.print("Crono = "); Serial.println(Crono);
        Serial.print("Cte Tiempo = "); Serial.println(Tiempo);
        Serial.print("Cte Ajuste = "); Serial.println(c_ajuste_1);  
      }
      else{
        Serial.print("Crono = "); Serial.println(Crono);
        Serial.print("Cte Tiempo = "); Serial.println(Tiempo);
        Serial.print("Cte Ajuste = "); Serial.println(ajuste);
      }
    #endif
  }
}

void LongPressCallbackFunction(void* s) { // Pulsador -> Programa por defecto la cuenta del Cronómetro (30 segundos)
  if (posicion == 2){
    Crono = -1;
    mx1.control(MD_MAX72XX::TEST, true);
    delay(1000);
    mx1.control(MD_MAX72XX::TEST, false);
    #ifdef ___DEBUG1___
      Serial.println("");
      Serial.print("Pulsación Larga: ");
      Serial.println((char*)s);
      Serial.print("Crono = 30 segundos"); 
    #endif
  }
}

  //________________________________________________________________________________________________________________________________

void setup(void) {

  Serial.begin(9600);
  while (!Serial)
    delay(10);

  #ifdef ___DEBUG___
    Serial.println("");
    Serial.println("........................ Inilicializo Cronómero de Arena Digital DEBUG ........................");
    Serial.print("Crono = "); Serial.println(Crono);
    Serial.print("Cte Tiempo = "); Serial.println(Tiempo);
    Serial.print("Cte Ajuste = "); Serial.println(ajuste); 
  #endif
  #ifdef ___DEBUG1___
    #ifndef ___DEBUG___
      Serial.println("");
      Serial.println("........................ Inilicializo Cronómero de Arena Digital DEBUG1 ........................");
      Serial.print("Crono = "); Serial.println(Crono);
      Serial.print("Cte Tiempo = "); Serial.println(Tiempo);
      Serial.print("Cte Ajuste = "); Serial.println(ajuste); 
    #endif
  #endif

  // Inicializo objetos pulsador
  PulsadorSwitch.setLongPressCallback(&LongPressCallbackFunction, (void*)"long press");
  PulsadorSwitch.setReleasedCallback(&ReleasedCallbackCallbackFunction, (void*)"relased");
  PulsadorSwitch.setPushedCallback(&pushedCallbackFunction, (void*)"pushed");

  // Inicializo Paneles LED MAX7219
  mx1.begin();
  mx1.control(MD_MAX72XX::TEST, true);
  delay(2000);
  mx1.control(MD_MAX72XX::TEST, false);
  mx1.control(MD_MAX72XX::INTENSITY, 2);
  mx2.begin();
  mx2.control(MD_MAX72XX::TEST, true);
  delay(2000);
  mx2.control(MD_MAX72XX::TEST, false);
  mx2.control(MD_MAX72XX::INTENSITY, 2);

  // Inicializo MPU6050 GY-521
  inicio_MPU();

  // Fin Setup()
  mx1.control(MD_MAX72XX::TEST, true);
  mx2.control(MD_MAX72XX::TEST, true);
  delay(2000);
  mx1.control(MD_MAX72XX::TEST, false);
  mx2.control(MD_MAX72XX::TEST, false);
}

void loop() {

  // Busqueda de Posición del Cronómetro de Arena
  if (state_mpu){
    state_mpu = false;
    mx1.clear();
    mx2.clear();
    parado = false;

    while (muestras>=0){
      sensors_event_t a, g, temp;
      mpu.getEvent(&a, &g, &temp);

      aX = a.acceleration.x;
      aY = a.acceleration.y;
      aZ = a.acceleration.z;

      // Posiciones Horizontales
      if ((aX < 0) && (aY > 0) && (aZ > acel))
        posicion = 1;
      if ((aX > 0 ) && (aY < 0) && (aZ < -acel))
        posicion = 6;
      // Posición Inicial
      if ((aX > acel) && (aY > 0) && (aZ > 0))
        posicion = 2;
      if ((aX > acel) && (aY < 0) && (aZ > 0))
        posicion = 2;
      // Posiciones Verticales
      if ((aX < 0) && (aY > acel) && (aZ > 0))
        posicion = 3;
      if ((aX > 0 ) && (aY < -acel) && (aZ > 0))
        posicion = 4;
      // Comprueba que se detecta la posición x muestras
      if ((posicion == mem_posicion) && (posicion!=0)){
        muestras--;
        mem_posicion = posicion;
      }
      else{
        muestras = repeticiones;
        mem_posicion = posicion;
      }

      int x1 = random(0,7);
      int y1 = random(0,7);
      mx1.setPoint(x1, y1, true);
      int x2 = random(0,7);
      int y2 = random(0,7);
      mx2.setPoint(x2, y2, true);
      delay (500);
      #ifdef ___DEBUG1___
        Serial.println("");
        Serial.print("posicion --> ");
        Serial.println(posicion);
        Serial.print("Acceleration X: ");
        Serial.print(aX);
        Serial.print(", Y: ");
        Serial.print(aY);
        Serial.print(", Z: ");
        Serial.print(aZ);
        Serial.println(" m/s^2");
        Serial.println("");
      #endif
    }

    mx1.clear();
    mx2.clear();
    muestras = repeticiones;
    if (!(control_int))
      inicio_MPU();  
  }
  
  // Cronómetro de arena (Posiciones Horizontales)  
  if (posicion == 1){
    init_crono_1();
    start_crono_11();
    posicion = 5;
  }
  if (posicion == 6){
    init_crono_2();
    start_crono_22();
    posicion = 5;
  }
    
  // Cronómetro de arena (Posiciones Verticales)
  if (posicion == 3){
    init_crono_2();
    start_crono_3();
    posicion = 5;
  }
  if (posicion == 4){
    init_crono_1();
    start_crono_1();
    posicion = 5;
  }
  
  // Final de cuenta Cronómetro de arena
  if (posicion == 5){
    #ifdef ___DEBUG1___
      if ((init_time)>0){
        Serial.print("Tiempo Cronometrado = ");
        Serial.println(end_time - init_time);
        init_time = 0;
        end_time = 0;
      }
    #endif
    mx1.control(MD_MAX72XX::TEST, true);
    delay(1000);
    mx1.control(MD_MAX72XX::TEST, false);
    delay(1000);
    mx2.control(MD_MAX72XX::TEST, true);
    delay(1000);
    mx2.control(MD_MAX72XX::TEST, false);
    delay(1000);
    if (!(control_int))
      inicio_MPU();
  }

  // Posición inicial para configurar Cronómetro de arena
  if (posicion == 2){
    mx1.setPoint(0, 0, true);
    mx2.setPoint(0, 0, true);
    mx1.setPoint(7, 7, true);
    mx2.setPoint(7, 7, true);
    mx1.setPoint(0, 7, true);
    mx2.setPoint(0, 7, true);
    mx1.setPoint(7, 0, true);
    mx2.setPoint(7, 0, true);

    // Control pulsador para configurar la cuenta del Cronómetro reloj de arena
    PulsadorSwitch.poll();
  }

}