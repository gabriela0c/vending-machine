#include "WebManager.h"

WebManager::WebManager() : server(80) {}

void WebManager::begin(const char* ssid, const char* password) {
  // Inicializa LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("Erro ao inicializar LittleFS");
    return;
  }

  // Configura o ESP32 como Access Point (Ponto de Acesso)
  Serial.print("Iniciando Access Point: ");
  Serial.println(ssid);
  
  // Cria a rede Wi-Fi. O password deve ter no mínimo 8 caracteres (ou passe NULL para deixar a rede aberta)
  WiFi.softAP(ssid, password); 
  
  // Obtém e imprime o IP do ESP32 na rede que ele acabou de criar (O padrão costuma ser 192.168.4.1)
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point Iniciado! IP do Servidor: ");
  Serial.println(IP);

  // Servir arquivos estáticos da raiz do LittleFS
  server.serveStatic("/", LittleFS, "/");

  // Configura todas as rotas da API
  setupRoutes();

  // Inicia o Servidor Web
  server.begin();
}

String WebManager::lerArquivo(const char* caminho) {
  if (!LittleFS.exists(caminho)) {
    return "";
  }
  File arquivo = LittleFS.open(caminho, "r");
  if (!arquivo) return "";
  String conteudo = arquivo.readString();
  arquivo.close();
  return conteudo;
}

void WebManager::gravarArquivo(const char* caminho, String conteudo) {
  File arquivo = LittleFS.open(caminho, "w");
  if (arquivo) {
    arquivo.print(conteudo);
    arquivo.close();
  }
}

UsuarioLogado WebManager::solicitarReconhecimentoFacial() {
  UsuarioLogado usuario; // autorizado inicia como false
  
  HTTPClient http;
  String urlPython = "http://192.168.4.2:5000/get-usuario"; // IP do seu PC
  
  http.begin(urlPython);
  http.setTimeout(10000);
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.POST("{}"); 
  String nomeReconhecido = "Desconhecido";

  if (httpResponseCode > 0) {
    String respostaPython = http.getString();
    StaticJsonDocument<200> docPython;
    deserializeJson(docPython, respostaPython);
    nomeReconhecido = docPython["usuario"].as<String>();
    Serial.println(nomeReconhecido);
  }
  http.end();

  if (nomeReconhecido != "Desconhecido" && nomeReconhecido != "Erro ao abrir camera") {
    DynamicJsonDocument docBanco(4096);
    String conteudoAtual = lerArquivo("/usuarios.json");
    
    if (conteudoAtual != "") {
      deserializeJson(docBanco, conteudoAtual);
      JsonArray usuarios = docBanco["usuarios"];
      
      for (JsonObject u : usuarios) {
        if (u["nome"].as<String>() == nomeReconhecido) {
          usuario.autorizado = true;
          usuario.nome = u["nome"].as<String>();
          usuario.saldo = u["saldo"].as<double>();
          usuario.maioridade = u["maioridade"].as<String>();
          return usuario;
        }
      }
    }
  }
  return usuario; 
}

bool WebManager::enviarCadastroParaPython(String nome, String &msgErro) {
  HTTPClient http;
  String urlPython = "http://192.168.4.2:5000/cadastrar-rosto";
  
  http.begin(urlPython);
  http.setTimeout(18000); // Timeout ligeiramente maior que os 15 segundos do Python
  http.addHeader("Content-Type", "application/json");
  
  StaticJsonDocument<128> doc;
  doc["nome"] = nome;
  String jsonPayload;
  serializeJson(doc, jsonPayload);
  
  int httpResponseCode = http.POST(jsonPayload);
  bool retorno = false;

  if (httpResponseCode > 0) {
    String resposta = http.getString();
    StaticJsonDocument<200> docResp;
    deserializeJson(docResp, resposta);
    
    if (docResp["status"].as<String>() == "sucesso") {
      retorno = true;
    } else {
      msgErro = docResp["mensagem"].as<String>();
    }
  } else {
    msgErro = "Sem conexao com o servidor Python.";
  }
  
  http.end();
  return retorno;
}

void WebManager::setupRoutes() {
  // ==================== PRODUTOS ====================
  server.on("/produtos", HTTP_GET, [this](AsyncWebServerRequest *request) {
    String conteudo = this->lerArquivo("/produtos.json");
    if (conteudo == "") conteudo = "{\"produtos\":[]}";
    request->send(200, "application/json", conteudo);
  });
  
  server.on("/salvar-produto", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(1024);
      DeserializationError erro = deserializeJson(doc, data, len);
      
      if (erro) {
        request->send(400, "text/plain", "Erro ao processar JSON");
        return;
      }
  
      String nome = doc["nome"];
      double valor = doc["valor"];
      String maioridade = doc["maioridade"];
  
      DynamicJsonDocument docBanco(4096);
      String conteudoAtual = this->lerArquivo("/produtos.json");
      if (conteudoAtual != "") {
        deserializeJson(docBanco, conteudoAtual);
      }
      
      JsonArray produtos = docBanco["produtos"].as<JsonArray>();
      if (!produtos) {
        produtos = docBanco["produtos"].to<JsonArray>();
      }
  
      // Verifica se o produto já existe para atualizar, se não, adiciona
      bool atualizado = false;
      for (JsonObject p : produtos) {
        if (p["nome"].as<String>() == nome) {
          p["valor"] = valor;
          p["maioridade"] = maioridade;
          atualizado = true;
          break;
        }
      }
  
      if (!atualizado) {
        if (produtos.size() < 4) { // Limite máximo de 4 motores/slots
          JsonObject novoProd = produtos.createNestedObject();
          novoProd["nome"] = nome;
          novoProd["valor"] = valor;
          novoProd["maioridade"] = maioridade;
        } else {
          request->send(400, "text/plain", "Erro: Limite maximo de 4 produtos atingido!");
          return;
        }
      }
  
      String resultado;
      serializeJson(docBanco, resultado);
      this->gravarArquivo("/produtos.json", resultado);
      request->send(200, "text/plain", "Produto cadastrado com sucesso!");
    }
  );

  server.on("/deletar-produto", HTTP_DELETE, [this](AsyncWebServerRequest *request) {
    if (request->hasParam("nome")) {
      String nomeParaDeletar = request->getParam("nome")->value();
      DynamicJsonDocument docBanco(4096);
      String conteudoAtual = this->lerArquivo("/produtos.json");
  
      if (conteudoAtual != "") {
        deserializeJson(docBanco, conteudoAtual);
        JsonArray produtos = docBanco["produtos"];
  
        DynamicJsonDocument docNovoBanco(4096);
        JsonArray novosProdutos = docNovoBanco["produtos"].to<JsonArray>();
  
        for (JsonObject p : produtos) {
          if (p["nome"].as<String>() != nomeParaDeletar) {
            novosProdutos.add(p);
          }
        }
  
        String resultado;
        serializeJson(docNovoBanco, resultado);
        this->gravarArquivo("/produtos.json", resultado);
        request->send(200, "text/plain", "Produto removido com sucesso!");
        return;
      }
    }
    request->send(400, "text/plain", "Parametro nome ausente");
  });

  // ==================== USUÁRIOS ====================
  server.on("/usuarios", HTTP_GET, [this](AsyncWebServerRequest *request) {
    String conteudo = this->lerArquivo("/usuarios.json");
    if (conteudo == "") {
      request->send(200, "application/json", "{\"usuarios\":[]}");
    } else {
      request->send(200, "application/json", conteudo);
    }
  });

  server.on("/salvar-usuario", HTTP_POST, 
    [](AsyncWebServerRequest *request) { request->send(200, "text/plain", "Usuario salvo!"); }, 
    NULL,
    [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<512> docNovo;
      deserializeJson(docNovo, data, len);

      StaticJsonDocument<256> usuarioFormatado;
      usuarioFormatado["nome"] = docNovo["nome"];
      usuarioFormatado["cpf"] = docNovo["cpf"];
      usuarioFormatado["senha"] = docNovo["senha"];
      usuarioFormatado["maioridade"] = docNovo["maioridade"];
      usuarioFormatado["saldo"] = docNovo["saldo"].as<double>(); 

      DynamicJsonDocument docBanco(4096);
      String conteudoAtual = this->lerArquivo("/usuarios.json");
      if (conteudoAtual != "") deserializeJson(docBanco, conteudoAtual);
      else docBanco["usuarios"].to<JsonArray>();

      JsonArray usuarios = docBanco["usuarios"];
      String nomeNovo = usuarioFormatado["nome"].as<String>();
      bool encontrado = false;

      for (JsonObject u : usuarios) {
        if (u["nome"].as<String>() == nomeNovo) {
          u["cpf"] = usuarioFormatado["cpf"];
          u["senha"] = usuarioFormatado["senha"];
          u["maioridade"] = usuarioFormatado["maioridade"];
          u["saldo"] = usuarioFormatado["saldo"];
          encontrado = true;
          break;
        }
      }

      if (!encontrado) usuarios.add(usuarioFormatado);

      String resultado;
      serializeJson(docBanco, resultado);
      this->gravarArquivo("/usuarios.json", resultado);
      Serial.println("Dados salvos no ESP32. Disparando camera do Python para salvar face...");
    }
  );

  server.on("/solicitar-reconhecimento", HTTP_GET, [this](AsyncWebServerRequest *request) {
    UsuarioLogado user = this->solicitarReconhecimentoFacial();
    
    StaticJsonDocument<256> resposta;
    resposta["autorizado"] = user.autorizado;
    resposta["nome"] = user.nome;
    resposta["saldo"] = user.saldo;
    resposta["maioridade"] = user.maioridade;
    
    String jsonResposta;
    serializeJson(resposta, jsonResposta);
    request->send(200, "application/json", jsonResposta);
  });

  server.on("/solicitar-cadastro", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!request->hasParam("nome")) {
      request->send(400, "application/json", "{\"sucesso\":false,\"mensagem\":\"Nome ausente\"}");
      return;
    }

    String nomeUsuario = request->getParam("nome")->value();
    String erroMensagem = "";
    
    // Chama a função que se comunica com o Python no PC
    bool cadastrado = this->enviarCadastroParaPython(nomeUsuario, erroMensagem);

    StaticJsonDocument<256> respostaJson;
    respostaJson["sucesso"] = cadastrado;
    respostaJson["mensagem"] = cadastrado ? "Sucesso" : erroMensagem;

    String resultado;
    serializeJson(respostaJson, resultado);
    request->send(200, "application/json", resultado);
  });

  server.on("/selecionar-produto-gesto", HTTP_POST, 
    [](AsyncWebServerRequest *request) { 
      request->send(200, "application/json", "{\"status\":\"recebido\"}"); 
    }, 
    NULL,
    [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<128> docNovo;
      deserializeJson(docNovo, data, len);
      
      int prod = docNovo["produto"].as<int>();
      if (prod >= 1 && prod <= 4) {
        this->produtoSelecionado = prod;
        this->temNovoProduto = true;
        Serial.print("Produto recebido por gesto: ");
        Serial.println(prod);
      }
    }
  );

  server.on("/deletar-usuario", HTTP_DELETE, [this](AsyncWebServerRequest *request) {
    if (request->hasParam("nome")) {
      String nomeParaDeletar = request->getParam("nome")->value();
      DynamicJsonDocument docBanco(4096);
      String conteudoAtual = this->lerArquivo("/usuarios.json");
      
      if (conteudoAtual != "") {
        deserializeJson(docBanco, conteudoAtual);
        JsonArray usuarios = docBanco["usuarios"];
        
        DynamicJsonDocument docNovoBanco(4096);
        JsonArray novosUsuarios = docNovoBanco["usuarios"].to<JsonArray>();

        for (JsonObject u : usuarios) {
          if (u["nome"].as<String>() != nomeParaDeletar) {
            novosUsuarios.add(u);
          }
        }

        String resultado;
        serializeJson(docNovoBanco, resultado);
        this->gravarArquivo("/usuarios.json", resultado);

        HTTPClient http;
        
        String urlPython = "http://192.168.4.2:5000/deletar-rosto?nome=" + nomeParaDeletar;
        
        http.begin(urlPython);
        int httpResponseCode = http.sendRequest("DELETE"); // Envia o método DELETE para o Flask
        
        if (httpResponseCode > 0) {
          Serial.print("Python respondeu a exclusao com codigo: ");
          Serial.println(httpResponseCode);
        } else {
          Serial.print("Erro ao conectar no Python para exclusao: ");
          Serial.println(http.errorToString(httpResponseCode).c_str());
        }
        http.end();
      
        request->send(200, "text/plain", "Usuario removido");
        return;
      }
    }
    request->send(400, "text/plain", "Nome nao fornecido ou erro");
  });

  

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/login.html", "text/html");
  });
}

bool WebManager::obterProdutoPorSlot(int slot, String &nome, double &preco, String &maioridade) {
  String conteudo = lerArquivo("/produtos.json");
  if (conteudo == "") return false;

  DynamicJsonDocument doc(4096);
  if (deserializeJson(doc, conteudo)) return false;

  JsonArray produtos = doc["produtos"].as<JsonArray>();
  int index = slot - 1; // Ajusta slot (1-4) para índice do array (0-3)

  if (index >= 0 && index < produtos.size()) {
    JsonObject p = produtos[index];
    nome = p["nome"].as<String>();
    preco = p["valor"].as<double>();
    maioridade = p["maioridade"].as<String>();
    return true;
  }
  return false;
}

bool WebManager::debitarSaldoUsuario(String nomeUsuario, double valor) {
  String conteudo = lerArquivo("/usuarios.json");
  if (conteudo == "") return false;

  DynamicJsonDocument doc(4096);
  if (deserializeJson(doc, conteudo)) return false;

  JsonArray usuarios = doc["usuarios"].as<JsonArray>();
  bool debitouComSucesso = false;

  for (JsonObject u : usuarios) {
    if (u["nome"].as<String>() == nomeUsuario) {
      double saldoAtual = u["saldo"].as<double>();
      if (saldoAtual >= valor) {
        u["saldo"] = saldoAtual - valor;
        debitouComSucesso = true;
        break;
      }
    }
  }

  if (debitouComSucesso) {
    String resultado;
    serializeJson(doc, resultado);
    gravarArquivo("/usuarios.json", resultado); // Atualiza permanentemente no LittleFS
    return true;
  }
  return false;
}

void WebManager::resetarModoGestos() {
  HTTPClient http;
  String urlPython = "http://192.168.4.2:5000/cancelar-operacao";
  
  http.begin(urlPython);
  http.setTimeout(3000);
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.POST("{}");
  if (httpResponseCode > 0) {
    Serial.println("[HTTP] Python notificado para voltar ao modo Gestos.");
  } else {
    Serial.print("[HTTP] Erro ao notificar Python: ");
    Serial.println(http.errorToString(httpResponseCode).c_str());
  }
  http.end();
}
