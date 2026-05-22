#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

//TODO: TESTAR SE O CODIGO FUNCIONA NO ESP ; SALVAR FOTO NO COMPUTADOR NA HORA DE CADASTRAR (PYTHON) ; MUDAR MAIORIDADE PARA BOOL; REQUEST GESTOS;
// VERIFICAR O SALDO DO USUARIO ANTES DE LIBERAR O PRODUTO
//tem que testar se o site ta salvando o usuario do jeito certo tambem

//O que eu(vini) mudei: caduser.js: (função solicitarReconhecimento) ; python: adaptei para estrutura de requests (nao sei se funciona)
// main.cpp: criei struct usuariologado e uma função genérica para o esp mandar o request pro python e ser retornado o usuario (ja ta salvando na struct (nao sei se funciona))


// Estrutura para armazenar os dados do usuário logado
struct UsuarioLogado {
  bool autorizado = false;
  String nome = "";
  double saldo = 0.0;
  String maioridade = "";
};

UsuarioLogado solicitarReconhecimentoFacial() {
  UsuarioLogado usuario; // Cria uma instância limpa (autorizado = false)
  
  // 1. Conecta e envia o comando para o Python abrir a câmera
  HTTPClient http;
  String urlPython = "http://192.168.1.100:5000/disparar-camera"; // IP do seu PC
  
  http.begin(urlPython);
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.POST("{}"); 
  String nomeReconhecido = "Desconhecido";

  if (httpResponseCode > 0) {
    String respostaPython = http.getString();
    StaticJsonDocument<200> docPython;
    deserializeJson(docPython, respostaPython);
    nomeReconhecido = docPython["usuario"].as<String>();
  }
  http.end(); // Fecha conexão com o Python

  // 2. Se o Python encontrou alguém, valida no LittleFS do ESP32
  if (nomeReconhecido != "Desconhecido" && nomeReconhecido != "Erro ao abrir camera") {
    DynamicJsonDocument docBanco(4096);
    String conteudoAtual = lerArquivo("/usuarios.json"); // Sua função de leitura do LittleFS
    
    if (conteudoAtual != "") {
      deserializeJson(docBanco, conteudoAtual);
      JsonArray usuarios = docBanco["usuarios"];
      
      for (JsonObject u : usuarios) {
        if (u["nome"].as<String>() == nomeReconhecido) {
          // Preenche a estrutura com os dados do arquivo local
          usuario.autorizado = true;
          usuario.nome = u["nome"].as<String>();
          usuario.saldo = u["saldo"].as<double>();
          usuario.maioridade = u["maioridade"].as<String>();
          return usuario; // Retorna o usuário preenchido imediatamente
        }
      }
    }
  }

  return usuario; // Retorna com autorizado = false se falhar ou não achar no JSON
}

// --- Configurações do Wi-Fi ---
const char* ssid = "NOME_DA_SUA_REDE";
const char* password = "SUA_SENHA_WIFI";

AsyncWebServer server(80);

// --- FUNÇÕES AUXILIARES PARA MANIPULAÇÃO DE ARQUIVOS (Equivalente ao fs do Node) ---

// Lê um arquivo do LittleFS e retorna como String
String lerArquivo(const char* caminho) {
  if (!LittleFS.exists(caminho)) {
    return "";
  }
  File arquivo = LittleFS.open(caminho, "r");
  if (!arquivo) return "";
  String conteudo = arquivo.readString();
  arquivo.close();
  return conteudo;
}

// Grava uma String em um arquivo no LittleFS (Equivalente ao fs.writeFileSync)
void gravarArquivo(const char* caminho, String conteudo) {
  File arquivo = LittleFS.open(caminho, "w");
  if (arquivo) {
    arquivo.print(conteudo);
    arquivo.close();
  }
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);

  // Inicializa LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("Erro ao inicializar LittleFS");
    return;
  }

  // Conecta ao Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado! IP: ");
  Serial.println(WiFi.localIP());

  // Servir arquivos estáticos do site (HTML, CSS, JS, JPG) automaticamente da pasta /
  server.serveStatic("/", LittleFS, "/");

  // =========================================================================
  // ROUTER: PRODUTOS
  // =========================================================================

  // GET /produtos -> MOSTRAR PRODUTOS
  server.on("/produtos", HTTP_GET, [](AsyncWebServerRequest *request) {
    String conteudo = lerArquivo("/produtos.json");
    if (conteudo == "") {
      request->send(200, "application/json", "{\"produtos\":[]}");
    } else {
      request->send(200, "application/json", conteudo);
    }
  });

  // POST /salvar-produto -> SALVAR/ATUALIZAR PRODUTO
  server.on("/salvar-produto", HTTP_POST, 
    [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", "Produto salvo!");
    }, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      // Parse do produto enviado pelo fetch
      StaticJsonDocument<256> docNovo;
      deserializeJson(docNovo, data, len);

      // Carrega o banco existente ou cria um novo
      DynamicJsonDocument docBanco(4096);
      String conteudoAtual = lerArquivo("/produtos.json");
      if (conteudoAtual != "") {
        deserializeJson(docBanco, conteudoAtual);
      } else {
        docBanco["produtos"].to<JsonArray>();
      }

      JsonArray produtos = docBanco["produtos"];
      String nomeNovo = docNovo["nome"].as<String>();
      bool encontrado = false;

      // Procura produto existente para atualizar
      for (JsonObject p : produtos) {
        if (p["nome"].as<String>() == nomeNovo) {
          p["preco"] = docNovo["preco"]; // Adicione aqui outros campos do seu produto se houver
          encontrado = true;
          Serial.println("Produto atualizado");
          break;
        }
      }

      // Se não existir, adiciona
      if (!encontrado) {
        produtos.add(docNovo);
        Serial.println("Produto adicionado");
      }

      // Salva de volta no arquivo
      String resultado;
      serializeJson(docBanco, resultado);
      gravarArquivo("/produtos.json", resultado);
    }
  );

  // DELETE /produto/:nome -> DELETAR PRODUTO
  // Nota: O ESPAsyncWebServer não faz parse automático de parâmetros na URL como o Express (:nome).
  // No JS, faça a requisição passando como query param: /deletar-produto?nome=Camisa
  server.on("/deletar-produto", HTTP_DELETE, [](AsyncWebServerRequest *request) {
    if (request->hasParam("nome")) {
      String nomeParaDeletar = request->getParam("nome")->value();

      DynamicJsonDocument docBanco(4096);
      String conteudoAtual = lerArquivo("/produtos.json");
      
      if (conteudoAtual != "") {
        deserializeJson(docBanco, conteudoAtual);
        JsonArray produtos = docBanco["produtos"];
        
        // Cria um novo array filtrado (equivalente ao .filter do JS)
        DynamicJsonDocument docNovoBanco(4096);
        JsonArray novosProdutos = docNovoBanco["produtos"].to<JsonArray>();

        for (JsonObject p : produtos) {
          if (p["nome"].as<String>() != nomeParaDeletar) {
            novosProdutos.add(p);
          }
        }

        String resultado;
        serializeJson(docNovoBanco, resultado);
        gravarArquivo("/produtos.json", resultado);
        request->send(200, "text/plain", "Produto removido");
        return;
      }
    }
    request->send(400, "text/plain", "Nome nao fornecido ou erro");
  });


  // =========================================================================
  // ROUTER: USUÁRIOS
  // =========================================================================

  // GET /usuarios -> MOSTRAR USUÁRIOS
  server.on("/usuarios", HTTP_GET, [](AsyncWebServerRequest *request) {
    String conteudo = lerArquivo("/usuarios.json");
    if (conteudo == "") {
      request->send(200, "application/json", "{\"usuarios\":[]}");
    } else {
      request->send(200, "application/json", conteudo);
    }
  });

  // POST /salvar-usuario -> SALVAR/ATUALIZAR USUÁRIO
  server.on("/salvar-usuario", HTTP_POST, 
    [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", "Usuario salvo!");
    }, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<512> docNovo;
      deserializeJson(docNovo, data, len);

      // Monta o objeto de forma explícita igual Node.js
      StaticJsonDocument<256> usuarioFormatado;
      usuarioFormatado["nome"] = docNovo["nome"];
      usuarioFormatado["cpf"] = docNovo["cpf"];
      usuarioFormatado["senha"] = docNovo["senha"];
      usuarioFormatado["maioridade"] = docNovo["maioridade"];
      usuarioFormatado["saldo"] = docNovo["saldo"].as<double>(); // Garante tipo Number/double

      DynamicJsonDocument docBanco(4096);
      String conteudoAtual = lerArquivo("/usuarios.json");
      if (conteudoAtual != "") {
        deserializeJson(docBanco, conteudoAtual);
      } else {
        docBanco["usuarios"].to<JsonArray>();
      }

      JsonArray usuarios = docBanco["usuarios"];
      String nomeNovo = usuarioFormatado["nome"].as<String>();
      bool encontrado = false;

      // Procura usuário existente
      for (JsonObject u : usuarios) {
        if (u["nome"].as<String>() == nomeNovo) {
          u["cpf"] = usuarioFormatado["cpf"];
          u["senha"] = usuarioFormatado["senha"];
          u["maioridade"] = usuarioFormatado["maioridade"];
          u["saldo"] = usuarioFormatado["saldo"];
          encontrado = true;
          Serial.println("Usuario atualizado");
          break;
        }
      }

      if (!encontrado) {
        usuarios.add(usuarioFormatado);
        Serial.println("Usuario adicionado");
      }

      String resultado;
      serializeJson(docBanco, resultado);
      gravarArquivo("/usuarios.json", resultado);
    }
  );

// CADASTRAR USUARIO PELO SITE
server.on("/solicitar-reconhecimento", HTTP_GET, [](AsyncWebServerRequest *request) {
  
  // Chama a função
    UsuarioLogado user = solicitarReconhecimentoFacial();
    
    // Prepara o JSON de resposta para o navegador
    StaticJsonDocument<256> resposta;
    resposta["autorizado"] = user.autorizado;
    resposta["nome"] = user.nome;
    resposta["saldo"] = user.saldo;
    resposta["maioridade"] = user.maioridade; //MUDAR MAIORIDADE PARA BOOL ASSIM QUE POSSIVEL
    
    String jsonResposta;
    serializeJson(resposta, jsonResposta);
    
    request->send(200, "application/json", jsonResposta);
  });

  // DELETE /deletar-usuario -> DELETAR USUÁRIO
  server.on("/deletar-usuario", HTTP_DELETE, [](AsyncWebServerRequest *request) {
    if (request->hasParam("nome")) {
      String nomeParaDeletar = request->getParam("nome")->value();

      DynamicJsonDocument docBanco(4096);
      String conteudoAtual = lerArquivo("/usuarios.json");
      
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
        gravarArquivo("/usuarios.json", resultado);
        request->send(200, "text/plain", "Usuario removido");
        return;
      }
    }
    request->send(400, "text/plain", "Nome nao fornecido ou erro");
  });

  //Define a primeira pagina ao acessar o IP como sendo a de login
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {

  request->send(LittleFS, "/login.html", "text/html");

  });
  // Inicia o Servidor
  server.begin();
}

void loop() {}