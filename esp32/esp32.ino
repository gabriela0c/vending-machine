#include "DcMotor.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Wire.h>
#include "TTP229.h"
#include "Adafruit_VL53L0X.h"
#include "WebManager.h" // Nossa nova classe de rede e arquivos

#define TFT_CS     17
#define TFT_RST    4
#define TFT_DC     16

#define WIDTH 170
#define HEIGHT 320

#define DISTANCIA_PRODUTO_MM 230
#define BUZZER_PIN 2

TTP229 ttp229;
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
Adafruit_VL53L0X lox = Adafruit_VL53L0X(); 
WebManager webManager; // Instância do gerenciador web

DcMotor motor1;
DcMotor motor2;
DcMotor motor3;
DcMotor motor4;
DcMotor* motors[4] = {&motor1, &motor2, &motor3, &motor4};

void quickWrite(String text) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(10, ((int) WIDTH/2)-20);
  tft.print(text);
}

byte waitForInput(TTP229 &keyboard) {
  byte key = 0;
  while (key == 0) {
    key = keyboard.readKeypad();
    delay(100);
  }
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
  return key;
}

void releaseProduct(int prod) {
  motors[prod-1]->runMotor(FORWARDS);
  VL53L0X_RangingMeasurementData_t measure;

  do {
    lox.rangingTest(&measure, false);
    delay(50);
  } while (measure.RangeStatus == 4 || measure.RangeMilliMeter > DISTANCIA_PRODUTO_MM);

  motors[prod-1]->runMotor(STOP);
  quickWrite("Retire o produto");

  do {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(150);
    digitalWrite(BUZZER_PIN, LOW);
    lox.rangingTest(&measure, false);
    delay(150); 
  } while (measure.RangeStatus != 4 && measure.RangeMilliMeter <= DISTANCIA_PRODUTO_MM);
  
  digitalWrite(BUZZER_PIN, LOW);
  delay(1000); 
}

void setup() {
  Serial.begin(115200);
  
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  ttp229 = TTP229(12, 35);
  motor1 = DcMotor(13, 14);
  motor2 = DcMotor(27, 26);
  motor3 = DcMotor(25, 33);
  motor4 = DcMotor(32, 19);

  tft.init(WIDTH, HEIGHT);
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setTextWrap(true);

  if (!lox.begin()) {
    quickWrite("Erro no VL53L0X");
    Serial.println(F("Falha ao iniciar o VL53L0X"));
    while(1); 
  }

  // Inicializa Arquivos, Web Server e Conexão com sua rede
  quickWrite("Conectando Wi-Fi...");
  webManager.begin("esp32_machine", "vending123");
}

void loop() {
  quickWrite("Escolha o produto\n(Teclado ou Gesto)");
  Serial.println("Aguardando escolha do produto...");

  int escolha = 0;

  // 1. Loop de escuta híbrida (Teclado ou Gesto)
  while (true) {
    if (webManager.temNovoProduto) { // Se veio via Wi-Fi/Gesto do Python
      escolha = webManager.produtoSelecionado;
      webManager.temNovoProduto = false; 
      break; 
    }

    byte teclaPressionada = ttp229.readKeypad();
    if (teclaPressionada >= 1 && teclaPressionada <= 4) {
      escolha = teclaPressionada;
      digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW);
      break; 
    }
    delay(50); 
  }

  // Se chegou aqui, uma escolha de 1 a 4 foi capturada
  String nomeProd = "";
  double precoProd = 0.0;
  String maioridadeProd = "";

  // 2. VALIDAÇÃO: Verifica se existe produto cadastrado para este motor/slot
  if (!webManager.obterProdutoPorSlot(escolha, nomeProd, precoProd, maioridadeProd)) {
    quickWrite("Slot " + String(escolha) + "\nNao cadastrado!");
    digitalWrite(BUZZER_PIN, HIGH); delay(1000); digitalWrite(BUZZER_PIN, LOW);
    webManager.resetarModoGestos();
    delay(2000);
    return; // Reinicia o loop, cancelando a operação
  }

  // Mostra o produto selecionado antes da biometria facial
  quickWrite(nomeProd + "\nR$ " + String(precoProd, 2) + "\nOlhe para a camera");

  // 3. RECONHECIMENTO FACIAL
  UsuarioLogado user = webManager.solicitarReconhecimentoFacial();
  
  if (user.autorizado) {
    
    // 4. VALIDAÇÃO DE MAIORIDADE (Bônus de segurança)
    if (maioridadeProd == "sim" && user.maioridade == "nao") {
      quickWrite("Produto exclusivo\npara maiores (+18)");
      digitalWrite(BUZZER_PIN, HIGH); delay(1500); digitalWrite(BUZZER_PIN, LOW);
      return;
    }
    
    // 5. VERIFICAÇÃO DE SALDO
    if (user.saldo >= precoProd) {
      
      // 6. REALIZA O DÉBITO NO LittleFS
      if (webManager.debitarSaldoUsuario(user.nome, precoProd)) {
        quickWrite("Pago! Remanescente:\nR$ " + String(user.saldo - precoProd, 2));
        delay(1500);
        
        quickWrite("Liberando produto...");
        releaseProduct(escolha); // Ativa o respectivo motor helicoidal
      } else {
        quickWrite("Erro interno\nao processar debito");
        delay(2500);
      }

    } else {
      quickWrite("Saldo insuficiente!\nSaldo: R$ " + String(user.saldo, 2));
      digitalWrite(BUZZER_PIN, HIGH); delay(1000); digitalWrite(BUZZER_PIN, LOW);
      delay(2500);
    }

  } else {
    quickWrite("Usuario nao\nreconhecido");
    digitalWrite(BUZZER_PIN, HIGH); delay(1000); digitalWrite(BUZZER_PIN, LOW);
    delay(2000);
  }
}
