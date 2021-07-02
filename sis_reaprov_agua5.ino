#include <Wire.h>
#include <LiquidCrystal_I2C.h>

int botao = 11;   // botao ativa a válvula instantaneamente
int rele = 2;     // rele é o acionador da válvula
int pot = 0;      // pot é o potenciômetro que regula o aproveitamento
int sensor = 3;   //  sensor de água
int ledAgua = 12; // ledAgua é um led que indica a passagem de água


int estadoBotao = 0;  // estado do botao
int estadoSensor = 0; // estado do sensor
float valPot = 0;     // valor lido do potenciômetro (0 a 1024)
float aprov = 0;      // porcentagem do aproveitamento de água.

long tolerancia_0ciclo = 3000;
long tolerancia_1ciclo = 3000;
long tolerancia_2ciclo = 3000;
long tempoInic = 0;
long tempoInic2 = 0;
long tempoFinal = 0;
long tempoEntreCiclos = 600000; // 10 minutos de intervalo.

#define ESTADO_INICIAL 0 
#define PRIMEIRO_CICLO 1
#define SEGUNDO_CICLO  2

int ControleMaquinaEstado = ESTADO_INICIAL;



void ExecutaMaquinaEstado(void);

LiquidCrystal_I2C lcd(0x3F,2,1,0,4,5,6,7,3, POSITIVE);

void setup()
{

  Serial.begin(9600);
  pinMode(botao, INPUT);
  pinMode(sensor, INPUT);
  pinMode(rele, OUTPUT);
  pinMode(ledAgua, OUTPUT);

  lcd.begin(16, 2);
  lcd.setBacklight(HIGH);
  lcd.setCursor(0,0);
  lcd.print("Aproveitar:");

}

void interface(void){
    aprov = (valPot*100)/1023;
    int aprovPorcento = aprov;
    if(aprovPorcento >= 10){    // condicionais para consertar falhas
      lcd.setCursor(12,0);      // no display
      lcd.print(aprovPorcento);
    }
    else{
      lcd.setCursor(12,0);
      lcd.print(0);
      lcd.setCursor(13,0);
      lcd.print(aprovPorcento);
    }
    if(aprovPorcento != 100){
      lcd.setCursor(14,0);
      lcd.print(" ");
    }
    lcd.setCursor(15,0);
    lcd.print('%');
    lcd.setCursor(0,1);
    lcd.print("Ciclo:");
    lcd.setCursor(7,1);
    lcd.print(ControleMaquinaEstado);
    lcd.setCursor(10,1);
  
  if(!digitalRead(sensor)){
    lcd.print("->->");
  }
  else{
    lcd.print("    ");
  }
}

void aproveitamento(float aprov){
  tempoInic = millis();
  tempoFinal = 0;
  int aproveitamento = aprov;
  if(aproveitamento <= 2){
    aproveitamento = 0;
  }
  if(aproveitamento >= 98){
    aproveitamento = 100;
  }
  Serial.println(aproveitamento);
  if(aproveitamento!=0){
    while(tempoFinal - tempoInic < 100*(aproveitamento)){
      digitalWrite(rele,HIGH);
      tempoFinal = millis();
      interface();
    }
  }

  tempoInic = millis();
  tempoFinal = 0;
  
    while(tempoFinal - tempoInic < 100*(100-(aproveitamento))){
      if(aproveitamento!=100){
        digitalWrite(rele,LOW);
      }
      tempoFinal = millis();
      interface(); 
    }
  
}

void loop()
{
  ControleMaquinaEstado = ESTADO_INICIAL;
  while(1){
    // atualizando os valores lidos
    unsigned long tempoAtual = millis();
    estadoBotao = digitalRead(botao);
    estadoSensor = digitalRead(sensor);
    valPot = analogRead(pot);


    ExecutaMaquinaEstado(); 

    interface();

    Serial.println(tempoFinal - tempoInic);
  }

}

void ExecutaMaquinaEstado(void){
  switch(ControleMaquinaEstado)
  {
  case ESTADO_INICIAL:
    {
      if(!estadoSensor == LOW){
        ControleMaquinaEstado = ESTADO_INICIAL;
        digitalWrite(rele,LOW);
      }
      else{
        tempoFinal = 0;
        tempoInic = millis();
        while( tempoFinal - tempoInic < tolerancia_0ciclo && !estadoSensor == HIGH){
          tempoFinal = millis();
          estadoSensor = digitalRead(sensor);
        }
        if(tempoFinal - tempoInic > tolerancia_0ciclo){
          ControleMaquinaEstado = PRIMEIRO_CICLO;
        }
      }
      Serial.println("Estado Inicial");
      break;  
    }
  case PRIMEIRO_CICLO:
    {
      tempoInic2 = millis();
      while(!estadoSensor==HIGH){
        aproveitamento(aprov);
        estadoSensor = digitalRead(sensor);
        Serial.println(!estadoSensor);
      }
      tempoFinal = millis(); 
      Serial.println(tempoFinal - tempoInic2);
      if( tempoFinal - tempoInic2 > tolerancia_1ciclo){
        ControleMaquinaEstado = SEGUNDO_CICLO;
        // delay(tempoEntreCiclos);
      }
      else{
        ControleMaquinaEstado = PRIMEIRO_CICLO;
      }
      Serial.println("Primeiro Ciclo");
      break;
    }
  case SEGUNDO_CICLO:
    {
      tempoInic = millis();
      if(!estadoSensor == HIGH){
        while(!estadoSensor == HIGH){  
          digitalWrite(rele,HIGH);
          estadoSensor = digitalRead(sensor); 
        }
        tempoFinal = millis();
        if(tempoFinal - tempoInic > tolerancia_2ciclo){
          ControleMaquinaEstado = ESTADO_INICIAL;
        }
        else{
          ControleMaquinaEstado = SEGUNDO_CICLO;
        }
      }
      Serial.println("Segundo Ciclo");
      break;
    }
  }
}


