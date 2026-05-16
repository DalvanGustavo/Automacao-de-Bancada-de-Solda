#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define sw 0
#define dt 1
#define clk 2

#define i2c_scl 5
#define i2c_sda 6

#define buzzer 7
#define led 8
#define rele 9
Adafruit_SSD1306 display(128, 64, &Wire, -1);

int senha[4] = {0, 0, 0, 0};
int senhaCorreta = 0;
int errouSenha = 0;
int estadoSenha = 0;
int minutos = 0;
int segundos = 0;
int estado = 0;
int estadoLigado = 0;
int estadoUltimoCLK;
int estadoAtualCLK;

unsigned long tempoInicioClique = 0;
unsigned long tempoSegundosAnterior = 0;
unsigned long ultimoTempoGiro = 0;
int intervalo = 1000;
int estadoAnteriorBotao = HIGH;

void desenharTela() {
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  if(senhaCorreta == 0){
    if(errouSenha == 0){
      display.print("Informe a Senha");
      
      char senhaFormatada[16]; // Cria um espaço na memória para o texto
      sprintf(senhaFormatada, "%d %d %d %d", senha[0], senha[1], senha[2], senha[3]); // Junta as variáveis no formato
      
      // Exibir formato de relógio
      display.setTextSize(2);
      display.setCursor(15, 20);
      display.print(senhaFormatada);

      // Desenhar linha de seleção
      int alturaLinha = 40; // Subi um pouco a linha para ficar colada no número (tamanho 2 é menor que 3)
      int xInicio = 15 + (estadoSenha * 24); 
      display.drawLine(xInicio, alturaLinha, xInicio + 10, alturaLinha, SSD1306_WHITE);
    }else{
      display.setTextSize(2);
      display.setCursor(34, 15);
      display.print("Senha");
      display.setCursor(4, 35);
      display.print("Incorreta!");
      errouSenha = 0;
    }
  }
  else{
    if(estadoLigado == 0){
      if(estado == 0){
        display.print("Editando: Minutos");
      }else{
        display.print("Editando: Segundos");
      }
    }else{
      display.print("Ligado...");
    }
    char tempoFormatado[6]; // Cria um espaço na memória para o texto
    sprintf(tempoFormatado, "%02d:%02d", minutos, segundos); // Junta as variáveis no formato
    
    // Exibir formato de relógio
    display.setTextSize(3);
    display.setCursor(15, 20);
    display.print(tempoFormatado);

    // Desenhar linha de seleção
    int alturaLinha = 45;
    if(estadoLigado == 0){
      if(estado == 0) {
        display.drawLine(15, alturaLinha, 45, alturaLinha, SSD1306_WHITE);
      } else {
        display.drawLine(70, alturaLinha, 100, alturaLinha, SSD1306_WHITE);
      }
    }
  }
  display.display();
}

void setup() {
  Wire.begin(i2c_sda, i2c_scl);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    for(;;);
  }
  pinMode(clk, INPUT_PULLUP);
  pinMode(dt, INPUT_PULLUP);
  pinMode(sw, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  pinMode(rele, OUTPUT);
  pinMode(led, OUTPUT);
  estadoUltimoCLK = digitalRead(clk);
  desenharTela();
}

void loop() {
  int estadoAtualBotao = digitalRead(sw);

  if (estadoAtualBotao != estadoAnteriorBotao) {
      
    if (estadoAtualBotao == LOW) {
      tempoInicioClique = millis();
    } 
    
    else {
      unsigned long tempoPressionado = millis() - tempoInicioClique;
      if(senhaCorreta == 0){
        if(tempoPressionado >= 5000){
          if(senha[0] == 5 && senha[1] == 5 && senha[2] == 5 && senha[3] == 5){
            senhaCorreta = 1;
          }
          else{
            senhaCorreta = 0;
            errouSenha = !errouSenha;
            estadoSenha = 0;
            for(int i =0; i < 4; i++){
              senha[i] = 0;
            }
            desenharTela();
            digitalWrite(buzzer, HIGH);
            delay(2000);
            digitalWrite(buzzer, LOW);
          }
          desenharTela();
        }
        else if (tempoPressionado > 50) { 
          estadoSenha++;
          if(estadoSenha == 4) estadoSenha = 0;
          desenharTela();
        }
      }
      
      //Senha confirmada
      else if(senhaCorreta == 1){
        if(tempoPressionado >= 10000){
          senhaCorreta = 0;
          estadoLigado = 0;
          estadoSenha = 0;
          digitalWrite(led, LOW);
          digitalWrite(buzzer, LOW);
          digitalWrite(rele, LOW);
          for(int i =0; i < 4; i++){senha[i] = 0;}
          desenharTela();
        }
        else if (tempoPressionado >= 4000) { 
          estadoLigado = 1;
          if(estadoLigado == 1) tempoSegundosAnterior = millis();
          desenharTela();
        }
        else if (tempoPressionado > 50 && estadoLigado == 0) { 
          estado = !estado;
          desenharTela();
        }
      }
    }
  }
  estadoAnteriorBotao = estadoAtualBotao;

  if(senhaCorreta == 0){
    estadoAtualCLK = digitalRead(clk);
    if(estadoAtualCLK != estadoUltimoCLK && estadoAtualCLK == 1 ){
      if(millis() - ultimoTempoGiro > 5){
        int direcao = (digitalRead(dt) != estadoAtualCLK) ? 1 : -1;
        senha[estadoSenha] += direcao;
        if(senha[estadoSenha] > 9) senha[estadoSenha] = 0;
        if(senha[estadoSenha] < 0) senha[estadoSenha] = 9;
        ultimoTempoGiro = millis();
        delay(50);
        desenharTela();
      }
    }
    estadoUltimoCLK = estadoAtualCLK;
  }

  if(senhaCorreta == 1){
    if(estadoLigado == 0){
      digitalWrite(led, LOW);
      digitalWrite(buzzer, LOW);
      digitalWrite(rele, LOW);
      estadoAtualCLK = digitalRead(clk);
      if(estadoAtualCLK != estadoUltimoCLK && estadoAtualCLK == 1 ){
        if(millis() - ultimoTempoGiro > 5){
          int direcao = (digitalRead(dt) != estadoAtualCLK) ? 1 : -1;
          if(estado == 0){
            minutos += direcao;
            if(minutos > 30) minutos = 0;
            if(minutos < 0) minutos = 30;
          }
          else if(estado == 1){
            segundos += direcao;
            if(segundos > 59) segundos = 0;
            if(segundos < 0) segundos = 59;
          }
          ultimoTempoGiro = millis();
          delay(50);
          desenharTela();
        }
      }
      estadoUltimoCLK = estadoAtualCLK;
    }
    else{
      bool atualizarTela = false;
      if(minutos == 0 && segundos == 0){
        digitalWrite(rele, LOW);
        digitalWrite(led, LOW);
        digitalWrite(buzzer, LOW);
        estadoLigado = 0;
        minutos = 10;
      }
      else{
        digitalWrite(rele, HIGH);
        unsigned long tempoSegundosAtual = millis();
        if(tempoSegundosAtual - tempoSegundosAnterior >= intervalo){
          tempoSegundosAnterior = tempoSegundosAtual;
          segundos--;
          atualizarTela = true;
        }
        if(segundos < 0){
          segundos = 59;
          minutos--;
          atualizarTela = true;
        }
        estadoAtualCLK = digitalRead(clk);
        if (estadoAtualCLK != estadoUltimoCLK && estadoAtualCLK == 1) {
          if (millis() - ultimoTempoGiro > 5) {
            int direcao = (digitalRead(dt) != estadoAtualCLK) ? 1 : -1;
            minutos = constrain(minutos + direcao, 0, 30);
            ultimoTempoGiro = millis();
            atualizarTela = true;
          }
        }
        estadoUltimoCLK = estadoAtualCLK;
        if(minutos < 5){
          digitalWrite(led, HIGH);
          if(segundos == 59) digitalWrite(buzzer, HIGH);
          else digitalWrite(buzzer, LOW);
        }
        else{
          digitalWrite(led, LOW);
          digitalWrite(buzzer, LOW);
        }
      }
      if(atualizarTela == true) desenharTela();
    }
  }
}