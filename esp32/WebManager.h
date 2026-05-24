#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

// Estrutura para armazenar os dados do usuário logado
struct UsuarioLogado {
  bool autorizado = false;
  String nome = "";
  double saldo = 0.0;
  String maioridade = "";
};

class WebManager {
private:
  AsyncWebServer server;
  
  // Métodos privados para manipulação interna
  String lerArquivo(const char* caminho);
  void gravarArquivo(const char* caminho, String conteudo);
  void setupRoutes();

public:
  // Construtor inicializa o servidor na porta 80
  WebManager();
  
  // Inicializa o LittleFS, Wi-Fi e os Endpoints
  void begin(const char* ssid, const char* password);
  
  // Faz a requisição para o Python e verifica no banco local
  UsuarioLogado solicitarReconhecimentoFacial();

  bool enviarCadastroParaPython(String nome, String &msgErro);

  volatile int produtoSelecionado = 0;
  volatile bool temNovoProduto = false;
  // Busca os dados de um produto com base no slot físico (1 a 4)
  bool obterProdutoPorSlot(int slot, String &nome, double &preco, String &maioridade);
  
  // Deduz o valor do produto do saldo atual do usuário e salva no LittleFS
  bool debitarSaldoUsuario(String nomeUsuario, double valor);

  void resetarModoGestos();
};
