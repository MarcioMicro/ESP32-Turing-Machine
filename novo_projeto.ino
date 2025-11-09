/**
 * ============================================================================
 * ESP32 Turing Machine - Projeto de Conclusão de Curso
 * ============================================================================
 *
 * Arquivo: novo_projeto.ino
 * Descrição: Implementação completa de uma Máquina de Turing em ESP32
 *            com interface web e display OLED
 *
 * ÍNDICE DO CÓDIGO (use Ctrl+F para navegar):
 *
 * 1. INCLUDES E CONFIGURAÇÕES
 *    - Bibliotecas
 *    - Constantes de hardware (display, botões)
 *    - Configurações de rede (WiFi, AP)
 *
 * 2. OBJETOS E ESTRUTURAS GLOBAIS
 *    - Servidor web e DNS
 *    - Display OLED
 *    - ExecutionResult
 *    - TuringMachine (classe principal)
 *    - TransitionInfo (struct auxiliar)
 *
 * 3. FUNÇÕES AUXILIARES
 *    - Transições: buscarTransicao(), isEstadoFinal(), aplicarTransicao()
 *    - Símbolos: isSimboloValidoParaEntrada(), obterPrimeiroSimboloValido()
 *    - Arquivos: servirArquivo()
 *
 * 4. INICIALIZAÇÃO
 *    - inicializarDisplay()
 *    - inicializarBotoes()
 *    - inicializarLittleFS()
 *    - conectarWiFi() / iniciarAP()
 *
 * 5. INTERFACE DISPLAY
 *    - drawTape()
 *    - animateTransition()
 *    - displayMessage()
 *
 * 6. NAVEGAÇÃO E MENUS
 *    - desenharMenuPrincipal()
 *    - desenharMenuSelecaoMT()
 *    - desenharEditorFita()
 *    - processarBotoes() e variações
 *
 * 7. EXECUÇÃO DA MÁQUINA
 *    - iniciarExecucaoAutomatica()
 *    - iniciarExecucaoPasso()
 *    - executarPassoAutomatico()
 *    - executarProximoPasso()
 *
 * 8. SERVIDOR WEB
 *    - Handlers (handleRoot, handleCSS, handleJS, etc.)
 *    - API REST (/api/save, /api/load, /api/execute, etc.)
 *    - configurarServidor()
 *
 * 9. SETUP E LOOP
 *    - setup()
 *    - loop()
 *
 * ============================================================================
 */

#include <LittleFS.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>

// Para manipulação de JSON
#include <ArduinoJson.h>

// Para comunicação I2C com display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ==============================================================================
// CONFIGURAÇÕES DE HARDWARE
// ==============================================================================

// Display OLED (I2C)
const int OLED_WIDTH = 128;
const int OLED_HEIGHT = 64;
const int OLED_SDA = 5;
const int OLED_SCL = 4;
const int OLED_RESET = -1;  // Reset compartilhado com ESP32
const int OLED_ADDRESS = 0x3C;

// Constantes para desenho da fita
const int CELL_SIZE = 18;         // Tamanho de cada célula da fita
const int CELL_SPACING = 17;      // Espaçamento entre células
const int TAPE_Y = 22;            // Posição Y da fita
const int VISIBLE_CELLS = 7;      // Células visíveis na tela

// Botões (com pull-up interno)
const int BTN_PREV = 12;     // Botão ANTERIOR
const int BTN_SELECT = 14;   // Botão SELECIONAR
const int BTN_NEXT = 2;      // Botão PRÓXIMO

// ==============================================================================
// CONFIGURAÇÕES DE REDE
// ==============================================================================

// WiFi Station (conectar em rede existente)
const char* WIFI_SSID = "MARCIO-WIFI";
const char* WIFI_PASSWORD = "102030405060";

// Access Point (criar rede própria)
const char* AP_SSID = "ESP32_TuringMachine";
const char* AP_PASSWORD = "123";

// Timeout para conexão WiFi
const int TIMEOUT_WIFI = 10000;  // 10 segundos

bool modoAP = false;

// ==============================================================================
// CONFIGURAÇÕES DA MÁQUINA DE TURING
// ==============================================================================

// Tamanho máximo da fita
// const int MAX_TAPE_SIZE = 100;

// Limite de passos de execução (proteção contra loop infinito)
// const int MAX_STEPS = 1000;

// Delay entre passos (visualização no display)
// const int STEP_DELAY_MS = 100;

// ==============================================================================
// OBJETOS GLOBAIS
// ==============================================================================

// Servidor web na porta 80
WebServer server(80);

// Servidor DNS para captive portal
DNSServer dnsServer;
const byte DNS_PORT = 53;

// Display OLED
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

// ==============================================================================
// DECLARAÇÕES ANTECIPADAS (Forward Declarations)
// ==============================================================================

// Variáveis globais (declaradas aqui, definidas depois)
extern bool displayAtivo;

// Funções do display (declaradas aqui, implementadas depois)
void drawTape(String tape, int headPos, String currentState, int stepCount, String statusMsg);
void animateTransition(String oldTape, String newTape, int oldPos, int newPos,
                       String oldState, String newState, int stepCount);
void displayMessage(String title, String message, int delayMs = 2000);

// ==============================================================================
// ESTRUTURAS DE DADOS
// ==============================================================================

/**
 * Estrutura para informações de transição
 */
struct TransitionInfo {
  bool found;
  String nextState;
  String newSymbol;
  String direction;
};

/**
 * Estrutura para resultado de execução
 */
struct ExecutionResult {
  bool accepted;
  int steps;
  String finalTape;
  String message;
  String history; // JSON com histórico de passos
};

// ==============================================================================
// DECLARAÇÕES FORWARD (Protótipos de Funções)
// ==============================================================================

// Funções helper de transição
TransitionInfo buscarTransicao(String currentState, char currentSymbol, JsonObject config);
TransitionInfo buscarTransicao(String currentState, char currentSymbol);
bool isEstadoFinal(String state);
bool isEstadoFinal(String state, JsonObject config);
int aplicarTransicao(TransitionInfo transition, String &tape, int headPosition, String &currentState);
int aplicarTransicao(TransitionInfo transition);

// Funções de display
void mostrarResultadoFinal(String titulo, String motivo, int passos, bool waitForButton = true);

// ==============================================================================
// CLASSE MÁQUINA DE TURING
// ==============================================================================

/**
 * Classe Máquina de Turing
 */
class TuringMachine {
public:
  String currentState;
  String tape;
  int headPosition;
  int stepCount;

  // Limites de segurança
  static const int MAX_STEPS = 1000;
  static const int MAX_TAPE_SIZE = 500;

  /**
   * Executa a máquina de Turing
   *
   * Parâmetros:
   *   - input: String de entrada
   *   - configJSON: Configuração em JSON
   *   - useDisplay: Se true, mostra execução no display OLED
   *   - stepDelay: Delay entre passos (ms) quando usando display
   */
  ExecutionResult execute(String input, String configJSON, bool useDisplay = false, int stepDelay = 500) {
    ExecutionResult result;
    result.accepted = false;
    result.steps = 0;
    result.message = "";
    result.history = "[";

    // Parse da configuração JSON
    StaticJsonDocument<8192> doc;
    DeserializationError error = deserializeJson(doc, configJSON);

    if (error) {
      result.message = "Erro ao parsear JSON: " + String(error.c_str());
      result.finalTape = "";
      result.steps = 0;
      result.history = "]";
      Serial.println("✗ " + result.message);
      return result;
    }

    // Extrair estado inicial
    String initialState = doc["initialState"] | "q0";

    // Inicializar fita: ^input_____
    tape = "^" + input;
    for (int i = 0; i < 20; i++) tape += "_"; // Padding

    headPosition = 0;
    currentState = initialState;
    stepCount = 0;

    Serial.println("\n=== Executando Máquina de Turing ===");
    Serial.printf("Entrada: %s\n", input.c_str());
    Serial.printf("Fita inicial: %s\n", tape.c_str());
    Serial.printf("Estado inicial: %s\n", currentState.c_str());

    // Mostrar fita inicial no display
    if (useDisplay && displayAtivo) {
      displayMessage("Executando", "Iniciando...", 1000);
      drawTape(tape, headPosition, currentState, stepCount, "Pronto");
      delay(1000);
    }

    // Loop de execução
    bool transitionFound = true;
    while (stepCount < MAX_STEPS && transitionFound) {
      // Ler símbolo atual
      if (headPosition < 0 || headPosition >= tape.length()) {
        result.message = "Erro: Cabeça saiu da fita";
        result.finalTape = tape;
        result.steps = stepCount;
        result.history += "]";
        return result;
      }

      char currentSymbol = tape[headPosition];
      String symbolStr = String(currentSymbol);

      Serial.printf("Passo %d: Estado=%s, Posição=%d, Símbolo=%c\n",
                    stepCount, currentState.c_str(), headPosition, currentSymbol);

      // Adicionar ao histórico
      if (stepCount > 0) result.history += ",";
      result.history += "{";
      result.history += "\"step\":" + String(stepCount) + ",";
      result.history += "\"state\":\"" + currentState + "\",";
      result.history += "\"position\":" + String(headPosition) + ",";
      result.history += "\"symbol\":\"" + symbolStr + "\",";
      result.history += "\"tape\":\"" + tape + "\"";
      result.history += "}";

      // === USAR FUNÇÃO HELPER CENTRALIZADA ===
      // Buscar transição usando função reutilizável
      TransitionInfo transition = buscarTransicao(currentState, currentSymbol, doc.as<JsonObject>());

      if (!transition.found) {
        Serial.printf("✗ Sem transição para estado '%s' e símbolo '%s'\n",
                      currentState.c_str(), symbolStr.c_str());
        transitionFound = false;
        break;
      }

      // Salvar estado antigo para animação
      String oldTape = tape;
      int oldPosition = headPosition;
      String oldState = currentState;

      Serial.printf("→ Transição: (%s, %s, %s)\n",
                    transition.nextState.c_str(), transition.newSymbol.c_str(),
                    transition.direction.c_str());

      // === USAR FUNÇÃO HELPER CENTRALIZADA ===
      // Aplicar transição usando função reutilizável
      int newPosition = aplicarTransicao(transition, tape, headPosition, currentState);

      // Animar transição no display
      if (useDisplay && displayAtivo) {
        animateTransition(oldTape, tape, oldPosition, newPosition,
                         oldState, currentState, stepCount);
      }

      // Atualizar posição da cabeça
      headPosition = newPosition;

      stepCount++;

      // Proteção contra loop infinito
      if (stepCount >= MAX_STEPS) {
        result.message = "Timeout: Limite de passos excedido";
        result.finalTape = tape;
        result.steps = stepCount;
        result.history += "]";
        return result;
      }

      // === USAR FUNÇÃO HELPER CENTRALIZADA ===
      // Verificar se chegou em estado final (prioridade máxima)
      if (isEstadoFinal(currentState, doc.as<JsonObject>())) {
        result.history += "]";
        result.finalTape = tape;
        result.steps = stepCount;
        result.accepted = true;
        result.message = "ACEITO - Estado final";
        Serial.println("✓ ACEITO! Estado final: " + currentState);

        // Mostrar resultado final no display (usando função unificada)
        // waitForButton = false porque execução é via API (não trava servidor)
        if (useDisplay && displayAtivo) {
          mostrarResultadoFinal("ACEITO", "Estado final", stepCount, false);
        }

        return result;
      }
    }

    // Se saiu do loop, foi porque não encontrou transição
    result.history += "]";
    result.finalTape = tape;
    result.steps = stepCount;
    result.accepted = false;
    result.message = "REJEITADO - Sem transicao";
    Serial.println("✗ REJEITADO! Sem transição para estado: " + currentState);

    Serial.printf("Passos executados: %d\n", stepCount);
    Serial.printf("Fita final: %s\n", tape.c_str());

    // Mostrar resultado final no display (usando função unificada)
    // waitForButton = false porque execução é via API (não trava servidor)
    if (useDisplay && displayAtivo) {
      mostrarResultadoFinal("REJEITADO", "Sem transicao", stepCount, false);
    }

    return result;
  }
};

// Instância global da máquina
TuringMachine tm;

// ==============================================================================
// VARIÁVEIS GLOBAIS
// ==============================================================================

// Modo de operação
// bool modoAP = false;  // true se estiver em modo Access Point

// Estado do display (sempre declarado, independente de ENABLE_OLED)
bool displayAtivo = false;

// Controle de execução
bool maquinaExecutando = false;
bool maquinaPausada = false;
bool executarPasso = false;  // true para executar apenas um passo
int scrollOffset = 0;  // Offset para scroll da fita

// Debounce dos botões
unsigned long ultimoDebounce = 0;
const int DEBOUNCE_DELAY_MS = 500;  // 200ms para debounce mais confiável

// ==============================================================================
// SISTEMA DE NAVEGAÇÃO E MENUS
// ==============================================================================

/**
 * Estados do sistema de navegação
 */
enum MenuState {
  MENU_PRINCIPAL,      // Menu inicial com opções
  MENU_SELECIONAR_MT,  // Seleção de máquina salva
  EDITOR_FITA,         // Edição da fita inicial
  EXECUTANDO_AUTO,     // Execução automática
  EXECUTANDO_PASSO,    // Execução passo-a-passo
  RESULTADO_FINAL      // Tela de resultado
};

MenuState estadoAtual = MENU_PRINCIPAL;
int opcaoSelecionada = 0;  // Índice da opção selecionada no menu

/**
 * Configuração da máquina atual
 */
String configAtual = "";       // JSON da configuração carregada
String nomeArquivoAtual = "";  // Nome do arquivo da MT atual
JsonArray alfabetoFita;        // Alfabeto da fita (para editor)
String entradaUsuario = "";    // String de entrada sendo editada
int posicaoEditor = 0;         // Posição do cursor no editor

// Posições especiais do editor (relativas à entrada)
// -2: Voltar ao menu
// -1: Reset fita
//  0: Iniciar MT (">")
//  1+: Células editáveis (entradaUsuario[posicao-1])

// Lista de arquivos .json disponíveis
String arquivosDisponiveis[20];  // Máximo 20 MTs salvas
int numArquivos = 0;
int arquivoSelecionado = 0;

// Execução passo-a-passo
int passoAtual = 0;
bool aguardandoProximoPasso = false;

// JSON Document global para evitar estouro de stack
// Usado em várias funções que fazem parse do configAtual
StaticJsonDocument<8192> docGlobal;

// ==============================================================================
// FUNÇÕES AUXILIARES DE TRANSIÇÃO
// ==============================================================================

/**
 * Busca transição no JSON da configuração (versão com JsonObject passado)
 *
 * Parâmetros:
 *   - currentState: Estado atual da MT
 *   - currentSymbol: Símbolo sendo lido
 *   - config: Objeto JSON com a configuração da MT
 *
 * Retorna: struct TransitionInfo com dados da transição
 */
TransitionInfo buscarTransicao(String currentState, char currentSymbol, JsonObject config) {
  TransitionInfo result;
  result.found = false;
  result.nextState = "?";
  result.newSymbol = "?";
  result.direction = "?";

  String symbolStr = String(currentSymbol);

  JsonObject transitions = config["transitions"];

  if (!transitions.containsKey(currentState)) {
    return result;
  }

  JsonObject stateTransitions = transitions[currentState];

  if (!stateTransitions.containsKey(symbolStr)) {
    return result;
  }

  JsonObject transition = stateTransitions[symbolStr];

  result.nextState = transition["nextState"] | "?";
  result.newSymbol = transition["newSymbol"] | "?";
  result.direction = transition["direction"] | "?";

  // Validar se a transição tem todos os campos necessários
  // Se algum campo estiver vazio ou for "?", considerar transição inválida
  if (result.nextState == "" || result.nextState == "?" ||
      result.newSymbol == "" || result.newSymbol == "?" ||
      result.direction == "" || result.direction == "?") {
    result.found = false;
    result.nextState = "?";
    result.newSymbol = "?";
    result.direction = "?";
    return result;
  }

  result.found = true;
  return result;
}

/**
 * Busca transição no JSON da configuração (versão com docGlobal)
 */
TransitionInfo buscarTransicao(String currentState, char currentSymbol) {
  return buscarTransicao(currentState, currentSymbol, docGlobal.as<JsonObject>());
}

/**
 * Verifica se um estado é final (versão com docGlobal)
 */
bool isEstadoFinal(String state) {
  JsonArray finalStates = docGlobal["finalStates"];
  for (JsonVariant finalState : finalStates) {
    if (finalState.as<String>() == state) {
      return true;
    }
  }
  return false;
}

/**
 * Verifica se um estado é final (versão com JsonObject passado)
 */
bool isEstadoFinal(String state, JsonObject config) {
  JsonArray finalStates = config["finalStates"];
  for (JsonVariant finalState : finalStates) {
    if (finalState.as<String>() == state) {
      return true;
    }
  }
  return false;
}

/**
 * Aplica uma transição à máquina de Turing (versão genérica)
 * Retorna nova posição da cabeça
 *
 * Parâmetros:
 *   - transition: Informações da transição a aplicar
 *   - tape: Referência para a fita (será modificada)
 *   - headPosition: Posição atual da cabeça
 *   - currentState: Referência para o estado atual (será modificado)
 */
int aplicarTransicao(TransitionInfo transition, String &tape, int headPosition, String &currentState) {
  if (!transition.found) {
    return headPosition;  // Sem transição, não move
  }

  // Escrever novo símbolo
  tape[headPosition] = transition.newSymbol[0];

  // Mudar estado
  currentState = transition.nextState;

  // Calcular nova posição
  int newPosition = headPosition;
  if (transition.direction == "L") {
    newPosition--;
  } else if (transition.direction == "R") {
    newPosition++;
  }
  // Direção "S" (STOP) não é mais suportada - removida

  // Expandir fita se necessário
  if (newPosition >= tape.length()) {
    tape += "_";
  }

  return newPosition;
}

/**
 * Aplica uma transição à máquina de Turing (versão com variável global tm)
 * Retorna nova posição da cabeça
 */
int aplicarTransicao(TransitionInfo transition) {
  return aplicarTransicao(transition, tm.tape, tm.headPosition, tm.currentState);
}

// ==============================================================================
// FUNÇÕES AUXILIARES DE ALFABETO E SÍMBOLOS
// ==============================================================================

/**
 * Verifica se símbolo é válido para entrada do usuário
 * (exclui marcadores especiais: ^, _, x)
 */
bool isSimboloValidoParaEntrada(String simbolo) {
  return simbolo != "^" && simbolo != "_" && simbolo != "x" && simbolo.length() > 0;
}

/**
 * Obtém primeiro símbolo válido do alfabeto de entrada
 */
char obterPrimeiroSimboloValido() {
  JsonArray inputAlphabet = docGlobal["alphabet"];

  if (inputAlphabet.size() == 0) {
    inputAlphabet = alfabetoFita;
  }

  for (int i = 0; i < inputAlphabet.size(); i++) {
    String simbolo = inputAlphabet[i].as<String>();
    if (isSimboloValidoParaEntrada(simbolo)) {
      return simbolo[0];
    }
  }

  // Fallback
  return '0';
}

/**
 * Obtém lista de símbolos válidos como String
 */
String obterSimbolosValidosString() {
  JsonArray inputAlphabet = docGlobal["alphabet"];

  if (inputAlphabet.size() == 0) {
    inputAlphabet = alfabetoFita;
  }

  String simbolosValidos = "";
  for (int i = 0; i < inputAlphabet.size(); i++) {
    String simbolo = inputAlphabet[i].as<String>();
    if (isSimboloValidoParaEntrada(simbolo)) {
      simbolosValidos += simbolo[0];
    }
  }

  if (simbolosValidos.length() == 0) {
    simbolosValidos = "01";  // Fallback binário
  }

  return simbolosValidos;
}

// ==============================================================================
// FUNÇÕES DE INICIALIZAÇÃO
// ==============================================================================

/**
 * Adiciona headers CORS para permitir requisições do navegador
 */
void addCORSHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

/**
 * Inicializa o display OLED
 *
 * Configura a comunicação I2C e testa a conexão com o display.
 * Exibe mensagem inicial de boot.
 *
 * Retorna: true se inicializou corretamente, false se falhou
 */
bool inicializarDisplay() {
  Serial.println("Inicializando display OLED...");

  // Inicializar comunicação I2C com pinos customizados
  Wire.begin(OLED_SDA, OLED_SCL);

  // Tentar inicializar o display
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS, false, false)) {
    Serial.println("✗ Falha ao inicializar display OLED!");
    return false;
  }

  // Limpar display
  display.clearDisplay();

  // Exibir mensagem de boot
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println("Turing");
  display.setCursor(10, 30);
  display.println("Machine");
  display.setTextSize(1);
  display.setCursor(20, 55);
  display.println("Iniciando...");
  display.display();

  delay(2000);

  displayAtivo = true;
  Serial.println("✓ Display OLED inicializado!");
  return true;
}

/**
 * Inicializa os botões físicos
 *
 * Configura os pinos GPIO como entrada com pull-up interno.
 * Isso significa que o botão pressionado lê LOW (0V).
 */
void inicializarBotoes() {
  pinMode(BTN_PREV, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);

  Serial.println("✓ Botões inicializados!");
}

// ==============================================================================
// FUNÇÕES DO DISPLAY OLED
// ==============================================================================

/**
 * Desenha a fita da Máquina de Turing no display
 *
 * Parâmetros:
 *   - tape: String representando a fita
 *   - headPos: Posição da cabeça de leitura/escrita
 *   - currentState: Estado atual da máquina
 *   - stepCount: Número de passos executados
 *   - statusMsg: Mensagem de status (ex: "Executando", "Aceito", "Rejeitado")
 */
void drawTape(String tape, int headPos, String currentState, int stepCount, String statusMsg) {
  display.clearDisplay();

  // ===== LINHA SUPERIOR: Estado e Passo =====
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("q atual:");
  display.print(currentState);

  display.setCursor(70, 0);
  display.print("Passo:");
  display.print(stepCount);

  // Linha separadora
  display.drawFastHLine(0, 10, OLED_WIDTH, SSD1306_WHITE);

  // ===== FITA COM SCROLL =====
  // A seta SEMPRE fica na posição 3 (centro fixo)
  // A fita começa com índice 0 nessa posição
  // Então: startCell = headPos - 3 (pode ser negativo no início!)
  int startCell = headPos - 3;
  scrollOffset = startCell;

  // Desenhar células visíveis
  int x = 0;
  for (int i = 0; i < VISIBLE_CELLS; i++) {
    int cellIndex = startCell + i;

    // Só desenha se o índice for válido (>= 0 e dentro da fita)
    if (cellIndex >= 0 && cellIndex < tape.length()) {
      char symbol = tape[cellIndex];

      // Destacar célula onde está a cabeça
      if (cellIndex == headPos) {
        display.fillRect(x, TAPE_Y, CELL_SIZE, CELL_SIZE, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
      } else {
        display.drawRect(x, TAPE_Y, CELL_SIZE, CELL_SIZE, SSD1306_WHITE);
        display.setTextColor(SSD1306_WHITE);
      }

      // Desenhar símbolo centralizado
      display.setCursor(x + 6, TAPE_Y + 5);
      display.setTextSize(1);
      display.write(symbol);
    }
    // Se cellIndex < 0, deixa espaço vazio (não desenha nada)

    x += CELL_SPACING;
  }

  // Seta EMBAIXO da fita, apontando PARA CIMA (formato de flecha ▲)
  // A seta sempre fica no centro (posição 3), pois a fita está centralizada
  int arrowX = (3 * CELL_SPACING) + 8;
  int arrowY = TAPE_Y + CELL_SIZE + 2;  // Logo abaixo da fita
  display.fillTriangle(
    arrowX, arrowY,              // Ponta (para cima)
    arrowX - 5, arrowY + 7,      // Base esquerda (embaixo)
    arrowX + 5, arrowY + 7,      // Base direita (embaixo)
    SSD1306_WHITE
  );

  // ===== LINHA INFERIOR: Apenas status/transição =====
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 55);
  display.print(statusMsg);

  display.display();
}

// FUNÇÃO REMOVIDA: drawTapeComTransicao() antiga
// Agora usamos apenas drawTape() com formatação de transição no parâmetro statusMsg

/**
 * Anima a transição entre dois estados de forma SUAVE
 *
 * Mostra visualmente:
 *   1. Símbolo sendo escrito na fita (fade in do novo símbolo)
 *   2. Fita deslizando suavemente (scroll pixel por pixel)
 *   3. Mudança de estado
 */
void animateTransition(String oldTape, String newTape, int oldPos, int newPos,
                       String oldState, String newState, int stepCount) {

  // Buscar informação da transição que está sendo executada
  docGlobal.clear();
  deserializeJson(docGlobal, configAtual);

  char symbolAtOldPos = newTape[oldPos];
  TransitionInfo trans = buscarTransicao(oldState, oldTape[oldPos]);

  String transInfo = "trans: ";
  if (trans.found) {
    transInfo += trans.nextState + " | " + trans.newSymbol + " | ";
    if (trans.direction == "L") transInfo += "E";
    else if (trans.direction == "R") transInfo += "D";
    else transInfo += "S";
  } else {
    transInfo += "-";
  }

  // ===== FASE 1: ESCRITA DO SÍMBOLO (fade effect) =====
  drawTape(oldTape, oldPos, oldState, stepCount, transInfo);
  delay(200);

  // Simula escrita com fade (alternando entre velho e novo símbolo rapidamente)
  for (int fade = 0; fade < 3; fade++) {
    drawTape(oldTape, oldPos, oldState, stepCount, transInfo);
    delay(80);
    drawTape(newTape, oldPos, oldState, stepCount, transInfo);
    delay(80);
  }

  // Mantém novo símbolo escrito
  drawTape(newTape, oldPos, oldState, stepCount, transInfo);
  delay(150);

  // ===== FASE 2: MOVIMENTO SUAVE DA FITA COM EASING =====
  if (oldPos != newPos) {
    int direction = (newPos > oldPos) ? 1 : -1;  // 1 = direita, -1 = esquerda
    int framesPerCell = 12;  // Frames de animação por célula (mais = mais suave)

    // Como a fita está SEMPRE CENTRALIZADA, basta animar o scroll
    // A cabeça sempre fica na posição 3 (centro)

    // Anima movimento suave pixel por pixel com easing
    for (int frame = 0; frame <= framesPerCell; frame++) {
      display.clearDisplay();

      // ===== LINHA SUPERIOR =====
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.print("q atual:");
      display.print(oldState);
      display.setCursor(70, 0);
      display.print("Passo:");
      display.print(stepCount);
      display.drawFastHLine(0, 10, OLED_WIDTH, SSD1306_WHITE);

      // ===== EASING: Ease-in-out para movimento suave =====
      float t = (float)frame / framesPerCell;
      // Função ease-in-out quadrática
      float easedProgress = t < 0.5 ?
                           2 * t * t :
                           1 - pow(-2 * t + 2, 2) / 2;

      // Calcular offset em pixels baseado no progresso
      int pixelOffset = (int)(easedProgress * CELL_SPACING);

      // ===== ANIMAÇÃO DA FITA =====
      // Interpolar entre posição antiga e nova
      // Seta sempre na posição 3, então startCell = headPos - 3
      int oldStartCell = oldPos - 3;  // Pode ser negativo!
      int newStartCell = newPos - 3;  // Pode ser negativo!

      // Calcular startCell interpolado (para transição suave)
      float startCellFloat = oldStartCell + (newStartCell - oldStartCell) * easedProgress;
      int startCell = (int)startCellFloat;

      // Desenhar células com offset animado
      for (int i = 0; i < VISIBLE_CELLS + 1; i++) {
        int cellIndex = startCell + i;

        // Só desenha se o índice for válido
        if (cellIndex >= 0 && cellIndex < newTape.length()) {
          char symbol = newTape[cellIndex];

          // Posição X da célula com scroll suave
          int cellX = i * CELL_SPACING;

          // Adicionar micro-offset para interpolação suave entre células
          float subCellOffset = (startCellFloat - startCell) * CELL_SPACING;
          cellX -= (int)subCellOffset;

          // Só desenha se visível na tela
          if (cellX >= -CELL_SIZE && cellX <= OLED_WIDTH) {
            // Destacar célula onde a cabeça está (sempre posição 3, centro)
            bool shouldHighlight = (cellIndex == oldPos && easedProgress < 0.5) ||
                                  (cellIndex == newPos && easedProgress >= 0.5);

            if (shouldHighlight) {
              display.fillRect(cellX, TAPE_Y, CELL_SIZE, CELL_SIZE, SSD1306_WHITE);
              display.setTextColor(SSD1306_BLACK);
            } else {
              display.drawRect(cellX, TAPE_Y, CELL_SIZE, CELL_SIZE, SSD1306_WHITE);
              display.setTextColor(SSD1306_WHITE);
            }

            display.setCursor(cellX + 6, TAPE_Y + 5);
            display.write(symbol);
          }
        }
        // Se cellIndex < 0, deixa espaço vazio
      }

      // ===== SETA INDICADORA (FIXA, EMBAIXO, APONTANDO PARA CIMA) =====
      // A seta fica FIXA na posição central (pois fita está sempre centralizada)
      int arrowX = (3 * CELL_SPACING) + 8;
      int arrowY = TAPE_Y + CELL_SIZE + 2;  // Logo abaixo da fita

      display.setTextColor(SSD1306_WHITE);
      display.fillTriangle(
        arrowX, arrowY,              // Ponta (para cima)
        arrowX - 5, arrowY + 7,      // Base esquerda (embaixo)
        arrowX + 5, arrowY + 7,      // Base direita (embaixo)
        SSD1306_WHITE
      );

      // ===== LINHA INFERIOR: Transição =====
      display.setCursor(0, 55);
      display.print(transInfo);

      display.display();
      delay(25);  // ~40fps para movimento ultra-suave
    }
  }

  // ===== FASE 3: NOVO ESTADO =====
  // Exibir próxima transição (se houver)
  char nextSymbol = newTape[newPos];
  TransitionInfo nextTrans = buscarTransicao(newState, nextSymbol);

  String nextTransInfo = "trans: ";

  // Se já está em estado final OU se a próxima transição leva a estado final
  if (isEstadoFinal(newState)) {
    nextTransInfo += newState + " (final)";
  } else if (nextTrans.found && isEstadoFinal(nextTrans.nextState)) {
    // A próxima transição levará a um estado final
    nextTransInfo += nextTrans.nextState + " (final)";
  } else if (nextTrans.found) {
    nextTransInfo += nextTrans.nextState + " | " + nextTrans.newSymbol + " | ";
    if (nextTrans.direction == "L") nextTransInfo += "E";
    else if (nextTrans.direction == "R") nextTransInfo += "D";
    else nextTransInfo += "S";
  } else {
    nextTransInfo += "-";
  }

  drawTape(newTape, newPos, newState, stepCount + 1, nextTransInfo);
  delay(150);
}

/**
 * Exibe mensagem no display
 */
void displayMessage(String title, String message, int delayMs) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 10);
  display.setTextSize(2);
  display.println(title);

  display.setCursor(0, 35);
  display.setTextSize(1);
  display.println(message);

  display.display();
  delay(delayMs);
}

/**
 * Exibe resultado final com 3 linhas e opcionalmente espera por botão
 *
 * Formato:
 *   ACEITO / REJEITADO (grande)
 *   Motivo (linha 2)
 *   Passos: N (linha 3)
 *   [Pressione qualquer botão] (se waitForButton = true)
 *
 * Parâmetros:
 *   - waitForButton: Se true, espera botão. Se false, apenas mostra e retorna.
 */
void mostrarResultadoFinal(String titulo, String motivo, int passos, bool waitForButton) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Linha 1: Título grande
  display.setTextSize(2);
  display.setCursor(0, 5);
  display.println(titulo);

  // Linha 2: Motivo
  display.setTextSize(1);
  display.setCursor(0, 28);
  display.println(motivo);

  // Linha 3: Passos
  display.setCursor(0, 40);
  display.print("Passos: ");
  display.println(passos);

  // Instrução (apenas se vai esperar botão)
  if (waitForButton) {
    display.setCursor(0, 54);
    display.println("Pressione botao...");
  }

  display.display();

  // Esperar por qualquer botão (apenas se solicitado)
  if (waitForButton) {
    while (true) {
      int botao = lerBotao();
      if (botao != 0) {
        delay(200);  // Debounce
        break;
      }
      delay(50);
    }
  } else {
    // Se não espera botão, apenas dar um delay para visualização
    delay(3000);
  }
}

/**
 * Configura todas as rotas do servidor web
 *
 * Define endpoints para:
 *   - Servir arquivos estáticos (HTML, CSS, JS)
 *   - API REST (salvar, carregar, executar)
 *   - Listar/deletar configurações salvas
 */
// void configurarServidor() {
//   // Implementar nas ETAPAS 5, 6 e posteriores
// }

// ==============================================================================
// FUNÇÕES DA MÁQUINA DE TURING
// ==============================================================================

/**
 * Carrega configuração da MT a partir de JSON
 *
 * Lê arquivo do LittleFS e popula a estrutura TuringMachine.
 * Valida se todos os campos obrigatórios estão presentes.
 *
 * Parâmetros:
 *   - nomeArquivo: Caminho do arquivo JSON no LittleFS
 *
 * Retorna: true se carregou com sucesso, false se erro
 */
// bool carregarConfiguracao(const char* nomeArquivo) {
//   // Implementar na ETAPA 13
//   return false;
// }

/**
 * Executa um passo da Máquina de Turing
 *
 * Algoritmo:
 *   1. Verificar se está em estado final -> ACEITO
 *   2. Ler símbolo na posição atual da fita
 *   3. Buscar transição (estadoAtual, símboloLido)
 *   4. Se não existe transição -> REJEITADO
 *   5. Aplicar transição:
 *      - Escrever novo símbolo
 *      - Mover cabeça (L/R/S)
 *      - Mudar para novo estado
 *   6. Incrementar contador de passos
 *
 * Retorna: true se ainda está executando, false se parou (aceito/rejeitado/erro)
 */
// bool executarPasso() {
//   // Implementar na ETAPA 14
//   return false;
// }

/**
 * Executa a MT até conclusão ou limite de passos
 *
 * Chama executarPasso() repetidamente até:
 *   - Atingir estado final (aceito)
 *   - Não ter transição (rejeitado)
 *   - Exceder MAX_STEPS (timeout)
 *
 * Retorna: String com resultado da execução
 */
// String executarMaquina() {
//   // Implementar na ETAPA 14
//   return "";
// }

// ==============================================================================
// FUNÇÕES DO DISPLAY
// ==============================================================================

/**
 * Desenha a fita da MT no display OLED
 *
 * Formato:
 *   Linha 1: Estado atual
 *   Linha 2: Status (Executando/Pausado/Parado)
 *   Linhas 3-4: Visualização da fita (5 células)
 *   Linha 5: Instruções dos botões
 *
 * A célula onde está a cabeça é destacada (invertida).
 *
 * Parâmetros:
 *   - fita: String representando a fita
 *   - posicaoCabeca: Índice atual da cabeça
 *   - estadoAtual: Nome do estado atual
 */
// void desenharFita(String fita, int posicaoCabeca, String estadoAtual) {
//   // Implementar na ETAPA 9
// }

/**
 * Atualiza mensagem de status no display
 *
 * Mostra informações como:
 *   - Conectando ao WiFi
 *   - IP obtido
 *   - Modo AP ativo
 *   - Pronto para uso
 */
// void exibirStatus(String mensagem) {
//   // Implementar na ETAPA 7
// }

// ==============================================================================
// FUNÇÕES DOS BOTÕES
// ==============================================================================

/**
 * Lê o estado dos botões com debounce
 *
 * Retorna qual botão foi pressionado (0 = nenhum, 1 = PREV, 2 = SELECT, 3 = NEXT)
 */
int lerBotao() {
  // Ler estado atual dos botões (LOW = pressionado, pois usa pull-up)
  bool btnPrevPressed = (digitalRead(BTN_PREV) == LOW);
  bool btnSelectPressed = (digitalRead(BTN_SELECT) == LOW);
  bool btnNextPressed = (digitalRead(BTN_NEXT) == LOW);

  // Verificar se algum botão foi pressionado
  if (!btnPrevPressed && !btnSelectPressed && !btnNextPressed) {
    return 0;  // Nenhum botão pressionado
  }

  // Aplicar debounce
  unsigned long agora = millis();
  if (agora - ultimoDebounce < DEBOUNCE_DELAY_MS) {
    return 0;  // Ainda dentro do período de debounce
  }

  ultimoDebounce = agora;

  // Retornar qual botão foi pressionado
  if (btnPrevPressed) return 1;    // PREV
  if (btnSelectPressed) return 2;  // SELECT
  if (btnNextPressed) return 3;    // NEXT

  return 0;
}

// ==============================================================================
// FUNÇÕES DE NAVEGAÇÃO E MENUS
// ==============================================================================

/**
 * Carrega lista de arquivos .json do LittleFS
 */
void carregarListaArquivos() {
  numArquivos = 0;

  File root = LittleFS.open("/");
  File file = root.openNextFile();

  while (file && numArquivos < 20) {
    String fileName = String(file.name());

    // Garantir que o nome começa com /
    if (!fileName.startsWith("/")) {
      fileName = "/" + fileName;
    }

    // Filtrar apenas arquivos .json (ignorar config.json)
    if (fileName.endsWith(".json") &&
        fileName != "/config.json" &&
        !fileName.endsWith("config.json")) {

      arquivosDisponiveis[numArquivos] = fileName;
      Serial.printf("  [%d] %s\n", numArquivos, fileName.c_str());
      numArquivos++;
    }

    file = root.openNextFile();
  }

  Serial.printf("Total: %d arquivos .json encontrados\n", numArquivos);
}

/**
 * Carrega configuração de uma MT do LittleFS
 */
bool carregarConfiguracao(String nomeArquivo) {
  Serial.printf("\n=== Carregando MT ===\n");
  Serial.printf("Arquivo solicitado: '%s'\n", nomeArquivo.c_str());

  // Verificar se arquivo existe
  if (!LittleFS.exists(nomeArquivo)) {
    Serial.printf("✗ Arquivo '%s' não existe no LittleFS!\n", nomeArquivo.c_str());

    // Listar arquivos disponíveis para debug
    Serial.println("\nArquivos disponíveis:");
    File root = LittleFS.open("/");
    File f = root.openNextFile();
    while (f) {
      Serial.printf("  - %s\n", f.name());
      f = root.openNextFile();
    }

    if (displayAtivo) {
      displayMessage("Erro", "Arquivo nao existe", 2000);
    }
    return false;
  }

  File file = LittleFS.open(nomeArquivo, "r");
  if (!file) {
    Serial.printf("✗ Erro ao abrir arquivo '%s'\n", nomeArquivo.c_str());
    if (displayAtivo) {
      displayMessage("Erro", "Falha ao abrir", 2000);
    }
    return false;
  }

  configAtual = file.readString();
  file.close();

  Serial.printf("Tamanho do arquivo: %d bytes\n", configAtual.length());

  if (configAtual.length() == 0) {
    Serial.println("✗ Arquivo vazio!");
    if (displayAtivo) {
      displayMessage("Erro", "Arquivo vazio", 2000);
    }
    return false;
  }

  // Parse para extrair alfabeto da fita (usar docGlobal)
  docGlobal.clear();
  DeserializationError error = deserializeJson(docGlobal, configAtual);

  if (error) {
    Serial.printf("✗ Erro ao parsear JSON: %s\n", error.c_str());
    if (displayAtivo) {
      displayMessage("Erro", "JSON invalido", 2000);
    }
    return false;
  }

  alfabetoFita = docGlobal["tapeAlphabet"];
  nomeArquivoAtual = nomeArquivo;

  // Mostrar informações da MT carregada
  String nomeMT = docGlobal["nome"] | "Sem nome";
  String nomeArquivoLimpo = nomeArquivo;
  nomeArquivoLimpo.replace("/", "");
  nomeArquivoLimpo.replace(".json", "");

  Serial.printf("✓ MT carregada: %s (arquivo: %s)\n", nomeMT.c_str(), nomeArquivoLimpo.c_str());
  Serial.printf("  Estados: %d\n", docGlobal["states"].size());
  Serial.printf("  Alfabeto: %d símbolos\n", alfabetoFita.size());

  return true;
}

/**
 * Desenha o menu principal no display
 */
void desenharMenuPrincipal() {
  display.clearDisplay();

  // Título
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("=== MENU PRINCIPAL ===");

  display.drawFastHLine(0, 10, OLED_WIDTH, SSD1306_WHITE);

  // Opções do menu
  const char* opcoes[] = {
    "1. Modo Automatico",
    "2. Modo Passo-a-Passo",
    "3. Selecionar MT"
  };

  for (int i = 0; i < 3; i++) {
    int y = 18 + (i * 12);

    // Destacar opção selecionada
    if (i == opcaoSelecionada) {
      display.fillRect(0, y - 2, OLED_WIDTH, 11, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }

    display.setCursor(2, y);
    display.print(opcoes[i]);
  }

  // Instruções na parte inferior
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 54);
  display.print("PREV/NEXT SELECT=OK");

  display.display();
}

/**
 * Desenha o menu de seleção de MT
 */
void desenharMenuSelecaoMT() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("=== SELECIONAR MT ===");

  display.drawFastHLine(0, 10, OLED_WIDTH, SSD1306_WHITE);

  if (numArquivos == 0) {
    display.setCursor(10, 25);
    display.println("Nenhuma MT salva");
  } else {
    // Mostrar até 4 arquivos por vez
    int inicio = max(0, arquivoSelecionado - 1);
    int fim = min(numArquivos, inicio + 4);

    for (int i = inicio; i < fim; i++) {
      int y = 14 + ((i - inicio) * 10);

      // Destacar arquivo selecionado
      if (i == arquivoSelecionado) {
        display.fillRect(0, y - 2, OLED_WIDTH, 10, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
      } else {
        display.setTextColor(SSD1306_WHITE);
      }

      display.setCursor(2, y);

      // Exibir nome do arquivo (remover caminho e extensão)
      String nomeDisplay = arquivosDisponiveis[i];
      nomeDisplay.replace("/", "");
      nomeDisplay.replace(".json", "");

      if (nomeDisplay.length() > 18) {
        nomeDisplay = nomeDisplay.substring(0, 15) + "...";
      }

      display.print(nomeDisplay);
    }
  }

  // Instruções
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 54);
  display.print("PREV SELEC=OK NEXT");

  display.display();
}


/**
 * Desenha o editor de fita inicial
 *
 * Layout: [VOLTAR] [RESET] [>] [célula0] [célula1] ...
 * Posições: -2       -1      0     1         2
 */
void desenharEditorFita() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("=== EDITAR FITA ===");

  display.drawFastHLine(0, 10, OLED_WIDTH, SSD1306_WHITE);

  // Mostrar MT carregada
  display.setCursor(0, 14);
  String nomeDisplay = nomeArquivoAtual;
  nomeDisplay.replace("/", "");
  nomeDisplay.replace(".json", "");
  if (nomeDisplay.length() > 21) {
    nomeDisplay = nomeDisplay.substring(0, 18) + "...";
  }
  display.print("MT: ");
  display.println(nomeDisplay);

  // Visualização da fita (células)
  int y = 28;
  int cellWidth = 16;
  int startX = 4;

  // Calcular quantas células mostrar (7 visíveis)
  // Sempre centralizar cursor na posição 3 (índice do meio)
  int inicioCelula = posicaoEditor - 3;

  for (int i = 0; i < 7; i++) {
    int posicaoAtual = inicioCelula + i;
    int x = startX + (i * cellWidth);

    // Destacar posição do cursor
    bool highlighted = (posicaoAtual == posicaoEditor);

    if (highlighted) {
      display.fillRect(x, y, cellWidth - 1, 16, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.drawRect(x, y, cellWidth - 1, 16, SSD1306_WHITE);
      display.setTextColor(SSD1306_WHITE);
    }

    // Desenhar conteúdo da célula
    display.setCursor(x + 4, y + 4);

    if (posicaoAtual < -2) {
      // Não mostrar nada à esquerda de '<<' (células vazias)
      // Deixa a célula em branco
    } else if (posicaoAtual == -2) {
      // Voltar ao menu
      display.print("<<");
    } else if (posicaoAtual == -1) {
      // Reset fita
      display.print("X");
    } else if (posicaoAtual == 0) {
      // Iniciar MT
      display.print(">");
    } else if (posicaoAtual > 0 && posicaoAtual <= entradaUsuario.length()) {
      // Células editáveis (posição 1 = índice 0 da string)
      display.write(entradaUsuario[posicaoAtual - 1]);
    } else if (posicaoAtual > entradaUsuario.length()) {
      // Área vazia (além do final da string) - mostrar '+' à direita
      display.print("+");
    }
  }

  // Instruções
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 50);

  if (posicaoEditor == -2) {
    display.println("SELECT=Voltar");
  } else if (posicaoEditor == -1) {
    display.println("SELECT=Resetar");
  } else if (posicaoEditor == 0) {
    display.println("SELECT=Iniciar MT");
  } else if (posicaoEditor > entradaUsuario.length()) {
    display.println("SELECT=Add simbolo");
  } else {
    display.println("SELECT=Trocar");
  }

  display.display();
}

/**
 * Alterna símbolo na posição atual do editor
 * Posição deve estar entre 1 e entradaUsuario.length()
 */
void alternarSimboloEditor() {
  if (posicaoEditor < 1 || posicaoEditor > entradaUsuario.length()) {
    Serial.println("Posição inválida para alternar símbolo");
    return;
  }

  // Buscar símbolos válidos usando função auxiliar
  docGlobal.clear();
  deserializeJson(docGlobal, configAtual);
  String simbolosValidos = obterSimbolosValidosString();

  // Obter símbolo atual e encontrar próximo no alfabeto
  int indiceString = posicaoEditor - 1;
  char simboloAtual = entradaUsuario[indiceString];
  int indiceAtual = simbolosValidos.indexOf(simboloAtual);

  if (indiceAtual == -1) {
    indiceAtual = 0;
  }

  // Próximo símbolo (circular)
  int proximoIndice = (indiceAtual + 1) % simbolosValidos.length();
  entradaUsuario[indiceString] = simbolosValidos[proximoIndice];

  Serial.printf("Símbolo alterado na pos %d: %c -> %c\n",
                posicaoEditor, simboloAtual, simbolosValidos[proximoIndice]);
}

// ==============================================================================
// SETUP - Inicialização do Sistema
// ==============================================================================

/**
 * Função de inicialização do Arduino
 *
 * Executada UMA VEZ quando o ESP32 liga ou reseta.
 *
 * Ordem de inicialização:
 *   1. Serial (para debug)
 *   2. LittleFS (sistema de arquivos)
 *   3. Display OLED
 *   4. Botões
 *   5. WiFi (Station ou AP)
 *   6. Servidor web
 */

bool inicializarLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("ERRO: Falha ao montar LittleFS");
    return false;
  }

  Serial.println("LittleFS montado com sucesso!");
  return true;
}


void listarArquivos() {
  Serial.println("=== Arquivos no LittleFS ===");
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.printf("  %s (%d bytes)\n", file.name(), file.size());
    file = root.openNextFile();
  }
}

bool conectarWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.printf("Conectando a: %s", WIFI_SSID);

  unsigned long inicio = millis();

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - inicio > TIMEOUT_WIFI) {
      Serial.println("\nTimeout na conexão WiFi");
      return false;
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✓ Conectado ao WiFi!");
  Serial.printf("  IP: %s\n", WiFi.localIP().toString().c_str());
  return true;
}


void iniciarAP() {
  Serial.println("\n=== Modo Access Point ===");

  modoAP = true;

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  IPAddress IP = WiFi.softAPIP();

  Serial.println("✓ Access Point ativo!");
  Serial.printf("  SSID: %s\n", AP_SSID);
  Serial.printf("  IP: %s\n", IP.toString().c_str());

  // Iniciar DNS captive portal
  dnsServer.start(DNS_PORT, "*", IP);
  Serial.println("✓ DNS Captive Portal ativo");
}


/**
 * Função auxiliar para servir arquivos do LittleFS
 */
void servirArquivo(const char* caminho, const char* contentType) {
  File file = LittleFS.open(caminho, "r");

  if (!file) {
    String msg = "Arquivo " + String(caminho) + " não encontrado";
    server.send(404, "text/plain", msg);
    return;
  }

  server.streamFile(file, contentType);
  file.close();
}

void handleRoot() {
  servirArquivo("/index.html", "text/html");
}

void handleCSS() {
  servirArquivo("/styles.css", "text/css");
}

void handleJS() {
  servirArquivo("/script.js", "application/javascript");
}

void handleStatus() {
  String json = "{";
  json += "\"status\":\"ok\",";
  json += "\"modo\":\"" + String(modoAP ? "AP" : "STA") + "\",";

  if (modoAP) {
    json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
  } else {
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"ssid\":\"" + WiFi.SSID() + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  }

  json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"uptime\":" + String(millis() / 1000);
  json += "}";

  server.send(200, "application/json", json);
}

void handleNotFound() {
  String message = "Erro 404 - Página não encontrada\n\n";
  message += "URI: " + server.uri() + "\n";
  message += "Método: " + String((server.method() == HTTP_GET) ? "GET" : "POST") + "\n";

  server.send(404, "text/plain", message);
}

// API: Salvar configuração da máquina
void handleSave() {
  addCORSHeaders();

  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Método não permitido");
    return;
  }

  String body = server.arg("plain");
  String fileName = "/machine.json";

  // Verificar se foi passado nome customizado
  if (server.hasArg("filename")) {
    fileName = "/" + server.arg("filename");
    if (!fileName.endsWith(".json")) {
      fileName += ".json";
    }
  }

  Serial.printf("Salvando configuração em: %s\n", fileName.c_str());
  Serial.println("Body recebido:");
  Serial.println(body);

  File file = LittleFS.open(fileName, "w");
  if (!file) {
    server.send(500, "application/json", "{\"error\":\"Erro ao criar arquivo\"}");
    return;
  }

  file.print(body);
  file.close();

  String response = "{\"success\":true,\"message\":\"Configuração salva\",\"file\":\"" + fileName + "\"}";
  server.send(200, "application/json", response);

  Serial.println("✓ Configuração salva com sucesso!");
}

// API: Carregar configuração da máquina
void handleLoad() {
  addCORSHeaders();

  String fileName = "/machine.json";

  if (server.hasArg("filename")) {
    fileName = "/" + server.arg("filename");
    if (!fileName.endsWith(".json")) {
      fileName += ".json";
    }
  }

  Serial.printf("Carregando configuração de: %s\n", fileName.c_str());

  File file = LittleFS.open(fileName, "r");
  if (!file) {
    Serial.println("✗ Arquivo não encontrado!");
    server.send(404, "application/json", "{\"error\":\"Arquivo não encontrado\"}");
    return;
  }

  String content = file.readString();
  file.close();

  Serial.println("✓ Configuração carregada:");
  Serial.println(content);

  server.send(200, "application/json", content);
}

// API: Listar arquivos salvos
void handleListFiles() {
  addCORSHeaders();

  String json = "{\"files\":[";

  File root = LittleFS.open("/");
  File file = root.openNextFile();
  bool first = true;

  while (file) {
    String fileName = String(file.name());

    // Filtrar apenas arquivos .json
    if (fileName.endsWith(".json")) {
      if (!first) json += ",";
      json += "{\"name\":\"" + fileName + "\",\"size\":" + String(file.size()) + "}";
      first = false;
    }

    file = root.openNextFile();
  }

  json += "]}";

  server.send(200, "application/json", json);
}

// API: Deletar arquivo
void handleDelete() {
  addCORSHeaders();

  if (!server.hasArg("filename")) {
    server.send(400, "application/json", "{\"error\":\"Nome do arquivo não fornecido\"}");
    return;
  }

  String fileName = "/" + server.arg("filename");
  if (!fileName.endsWith(".json")) {
    fileName += ".json";
  }

  if (LittleFS.remove(fileName)) {
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Arquivo deletado\"}");
    Serial.printf("✓ Arquivo %s deletado\n", fileName.c_str());
  } else {
    server.send(404, "application/json", "{\"error\":\"Arquivo não encontrado\"}");
  }
}

// API: Executar Máquina de Turing
void handleExecute() {
  addCORSHeaders();

  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Método não permitido");
    return;
  }

  // Obter dados do POST
  String body = server.arg("plain");

  Serial.println("\n=== API: Executar MT ===");
  Serial.println("Body recebido:");
  Serial.println(body);

  // Parse do JSON usando ArduinoJson
  StaticJsonDocument<8192> requestDoc;
  DeserializationError error = deserializeJson(requestDoc, body);

  if (error) {
    Serial.printf("✗ Erro ao parsear JSON: %s\n", error.c_str());
    String errorMsg = "{\"error\":\"Erro ao parsear JSON: " + String(error.c_str()) + "\"}";
    server.send(400, "application/json", errorMsg);
    return;
  }

  // Extrair input e config
  String input = requestDoc["input"] | "";
  JsonObject config = requestDoc["config"];

  if (input == "") {
    Serial.println("✗ Campo 'input' não encontrado");
    server.send(400, "application/json", "{\"error\":\"Campo 'input' não encontrado\"}");
    return;
  }

  if (config.isNull()) {
    Serial.println("✗ Campo 'config' não encontrado");
    server.send(400, "application/json", "{\"error\":\"Campo 'config' não encontrado\"}");
    return;
  }

  Serial.printf("Entrada: %s\n", input.c_str());

  // Serializar config de volta para string JSON
  String configJSON;
  serializeJson(config, configJSON);

  Serial.println("Config JSON:");
  Serial.println(configJSON);

  // Executar máquina
  ExecutionResult result = tm.execute(input, configJSON);

  // Montar resposta JSON
  String response = "{";
  response += "\"accepted\":" + String(result.accepted ? "true" : "false") + ",";
  response += "\"message\":\"" + result.message + "\",";
  response += "\"steps\":" + String(result.steps) + ",";
  response += "\"finalTape\":\"" + result.finalTape + "\",";
  response += "\"history\":" + result.history;
  response += "}";

  Serial.println("Resposta:");
  Serial.println(response);

  server.send(200, "application/json", response);
}

// API: Executar Máquina de Turing no Display OLED
void handleExecuteDisplay() {
  addCORSHeaders();

  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Método não permitido");
    return;
  }

  if (!displayAtivo) {
    server.send(503, "application/json", "{\"error\":\"Display OLED não está ativo\"}");
    return;
  }

  // Obter dados do POST
  String body = server.arg("plain");

  Serial.println("\n=== API: Executar MT no Display ===");

  // Parse do JSON usando ArduinoJson
  StaticJsonDocument<8192> requestDoc;
  DeserializationError error = deserializeJson(requestDoc, body);

  if (error) {
    Serial.printf("✗ Erro ao parsear JSON: %s\n", error.c_str());
    String errorMsg = "{\"error\":\"Erro ao parsear JSON: " + String(error.c_str()) + "\"}";
    server.send(400, "application/json", errorMsg);
    return;
  }

  // Extrair input, config e delay
  String input = requestDoc["input"] | "";
  JsonObject config = requestDoc["config"];
  int stepDelay = requestDoc["delay"] | 500;  // Delay padrão: 500ms

  if (input == "") {
    server.send(400, "application/json", "{\"error\":\"Campo 'input' não encontrado\"}");
    return;
  }

  if (config.isNull()) {
    server.send(400, "application/json", "{\"error\":\"Campo 'config' não encontrado\"}");
    return;
  }

  // Serializar config de volta para string JSON
  String configJSON;
  serializeJson(config, configJSON);

  // Responder imediatamente (execução será assíncrona)
  server.send(202, "application/json", "{\"status\":\"executing\",\"message\":\"Executando no display\"}");

  // Executar máquina COM display
  ExecutionResult result = tm.execute(input, configJSON, true, stepDelay);

  Serial.printf("Execução no display finalizada: %s\n", result.message.c_str());
}

// API: Iniciar modo passo a passo remotamente
void handleStartStepMode() {
  addCORSHeaders();

  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Método não permitido");
    return;
  }

  if (!displayAtivo) {
    server.send(503, "application/json", "{\"error\":\"Display OLED não está ativo\"}");
    return;
  }

  // Obter dados do POST
  String body = server.arg("plain");

  Serial.println("\n=== API: Iniciar Modo Passo a Passo ===");

  // Parse do JSON
  StaticJsonDocument<8192> requestDoc;
  DeserializationError error = deserializeJson(requestDoc, body);

  if (error) {
    Serial.printf("✗ Erro ao parsear JSON: %s\n", error.c_str());
    String errorMsg = "{\"error\":\"Erro ao parsear JSON: " + String(error.c_str()) + "\"}";
    server.send(400, "application/json", errorMsg);
    return;
  }

  // Extrair input e config
  String input = requestDoc["input"] | "";
  JsonObject config = requestDoc["config"];

  if (input == "") {
    server.send(400, "application/json", "{\"error\":\"Campo 'input' não encontrado\"}");
    return;
  }

  if (config.isNull()) {
    server.send(400, "application/json", "{\"error\":\"Campo 'config' não encontrado\"}");
    return;
  }

  // Armazenar entrada e configuração nas variáveis globais
  entradaUsuario = input;

  // Serializar config para string JSON
  configAtual = "";
  serializeJson(config, configAtual);

  // Carregar no docGlobal
  docGlobal.clear();
  deserializeJson(docGlobal, configAtual);

  Serial.printf("Entrada: %s\n", entradaUsuario.c_str());
  Serial.println("Config carregada");

  // Responder HTTP imediatamente
  server.send(200, "application/json", "{\"status\":\"step_mode_started\",\"message\":\"Modo passo a passo iniciado. Use os botões físicos do ESP32.\"}");

  // Iniciar modo passo a passo (mesma função do menu físico!)
  iniciarExecucaoPasso();
}

// Handler para requisições OPTIONS (CORS preflight)
void handleOptions() {
  addCORSHeaders();
  server.send(204); // No Content
}

void configurarServidor() {
  // Servir arquivos estáticos do LittleFS
  server.on("/", HTTP_GET, handleRoot);
  server.on("/styles.css", HTTP_GET, handleCSS);
  server.on("/script.js", HTTP_GET, handleJS);

  // API de status (JSON)
  server.on("/status", HTTP_GET, handleStatus);

  // APIs REST para Máquina de Turing
  server.on("/api/save", HTTP_POST, handleSave);
  server.on("/api/save", HTTP_OPTIONS, handleOptions);

  server.on("/api/load", HTTP_GET, handleLoad);
  server.on("/api/load", HTTP_OPTIONS, handleOptions);

  server.on("/api/files", HTTP_GET, handleListFiles);
  server.on("/api/files", HTTP_OPTIONS, handleOptions);

  server.on("/api/delete", HTTP_DELETE, handleDelete);
  server.on("/api/delete", HTTP_OPTIONS, handleOptions);

  server.on("/api/execute", HTTP_POST, handleExecute);
  server.on("/api/execute", HTTP_OPTIONS, handleOptions);

  server.on("/api/execute-display", HTTP_POST, handleExecuteDisplay);
  server.on("/api/execute-display", HTTP_OPTIONS, handleOptions);

  server.on("/api/start-step-mode", HTTP_POST, handleStartStepMode);
  server.on("/api/start-step-mode", HTTP_OPTIONS, handleOptions);

  // Rota para páginas não encontradas
  server.onNotFound(handleNotFound);

  // Iniciar servidor
  server.begin();
  Serial.println("✓ Servidor web iniciado!");
}


void setup() {
  // Inicializar comunicação serial
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=================================");
  Serial.println("  ESP32 Turing Machine");
  Serial.println("  Versão: 2.0");
  Serial.println("=================================\n");

  // ETAPA 2: LittleFS
  if (!inicializarLittleFS()) {
    Serial.println("ERRO: Não foi possível montar LittleFS");
    return;
  }

  // ETAPA 3: WiFi
  if (!conectarWiFi()) {
    // ETAPA 4: Fallback para modo AP
    iniciarAP();
  }

  // ETAPA 5: Servidor web
  configurarServidor();

  // ETAPA 7: Display OLED (opcional)
  inicializarDisplay();  // Não bloqueia se falhar

  // ETAPA 10: Botões (opcional)
  inicializarBotoes();

  // Exibir URL de acesso
  if (modoAP) {
    Serial.printf("\nAcesse: http://%s\n", WiFi.softAPIP().toString().c_str());
  } else {
    Serial.printf("\nAcesse: http://%s\n", WiFi.localIP().toString().c_str());
  }

  // Mostrar URL no display (se ativo)
  if (displayAtivo) {
    String ip = modoAP ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    displayMessage("Pronto!", ip, 3000);

    // Carregar lista de arquivos e exibir menu principal
    carregarListaArquivos();

    // Carregar primeira MT se disponível
    if (numArquivos > 0) {
      if (carregarConfiguracao(arquivosDisponiveis[0])) {
        Serial.println("✓ MT padrão carregada");
      }
    }

    // Exibir menu principal
    estadoAtual = MENU_PRINCIPAL;
    opcaoSelecionada = 0;
    desenharMenuPrincipal();
  }

  Serial.println("\n=== Sistema pronto! ===\n");
}

// ==============================================================================
// LOOP - Execução Contínua
// ==============================================================================

/**
 * Função de loop do Arduino
 *
 * Executada CONTINUAMENTE após o setup().
 * Responsável por:
 *   - Processar requisições DNS (se em modo AP)
 *   - Verificar estado da conexão WiFi
 *   - Ler botões físicos
 *   - Executar passos da MT (se ativa)
 *   - Atualizar display
 */
void loop() {
  // Processar requisições do servidor web
  server.handleClient();

  // Processar DNS captive portal (se em modo AP)
  if (modoAP) {
    dnsServer.processNextRequest();
  }

  // Verificar conexão WiFi
  if (!modoAP && WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado! Reconectando...");
    conectarWiFi();
  }

  // Processar botões se display estiver ativo
  if (displayAtivo) {
    processarBotoes();
  }

  delay(10);
}

// ==============================================================================
// PROCESSAMENTO DE BOTÕES
// ==============================================================================

/**
 * Processa eventos dos botões conforme o estado atual do sistema
 */
void processarBotoes() {
  int botao = lerBotao();

  if (botao == 0) {
    return;  // Nenhum botão pressionado
  }

  // Processar conforme estado atual
  switch (estadoAtual) {
    case MENU_PRINCIPAL:
      processarMenuPrincipal(botao);
      break;

    case MENU_SELECIONAR_MT:
      processarMenuSelecaoMT(botao);
      break;

    case EDITOR_FITA:
      processarEditorFita(botao);
      break;

    case EXECUTANDO_PASSO:
      processarExecucaoPasso(botao);
      break;

    case RESULTADO_FINAL:
      processarResultadoFinal(botao);
      break;

    default:
      break;
  }
}

/**
 * Processa botões no menu principal
 */
void processarMenuPrincipal(int botao) {
  if (botao == 1) {  // PREV
    opcaoSelecionada = (opcaoSelecionada - 1 + 3) % 3;
    desenharMenuPrincipal();

  } else if (botao == 3) {  // NEXT
    opcaoSelecionada = (opcaoSelecionada + 1) % 3;
    desenharMenuPrincipal();

  } else if (botao == 2) {  // SELECT
    // Processar opção selecionada
    if (opcaoSelecionada == 0) {
      // Modo Automático
      if (configAtual == "") {
        displayMessage("Aviso", "Selecione uma MT", 2000);
        estadoAtual = MENU_SELECIONAR_MT;
        arquivoSelecionado = 0;
        carregarListaArquivos();
        desenharMenuSelecaoMT();
      } else {
        estadoAtual = EDITOR_FITA;
        posicaoEditor = 0;
        inicializarEditor();
        desenharEditorFita();
      }

    } else if (opcaoSelecionada == 1) {
      // Modo Passo-a-Passo
      if (configAtual == "") {
        displayMessage("Aviso", "Selecione uma MT", 2000);
        estadoAtual = MENU_SELECIONAR_MT;
        arquivoSelecionado = 0;
        carregarListaArquivos();
        desenharMenuSelecaoMT();
      } else {
        estadoAtual = EDITOR_FITA;
        posicaoEditor = 0;
        inicializarEditor();
        desenharEditorFita();
      }

    } else if (opcaoSelecionada == 2) {
      // Selecionar MT
      estadoAtual = MENU_SELECIONAR_MT;
      arquivoSelecionado = 0;
      carregarListaArquivos();
      desenharMenuSelecaoMT();
    }
  }
}

/**
 * Processa botões no menu de seleção de MT
 */
void processarMenuSelecaoMT(int botao) {
  if (numArquivos == 0) {
    // Se não há arquivos, voltar ao menu principal
    if (botao == 2) {  // SELECT
      estadoAtual = MENU_PRINCIPAL;
      opcaoSelecionada = 0;
      desenharMenuPrincipal();
    }
    return;
  }

  if (botao == 1) {  // PREV
    arquivoSelecionado = (arquivoSelecionado - 1 + numArquivos) % numArquivos;
    desenharMenuSelecaoMT();

  } else if (botao == 3) {  // NEXT
    arquivoSelecionado = (arquivoSelecionado + 1) % numArquivos;
    desenharMenuSelecaoMT();

  } else if (botao == 2) {  // SELECT
    // Carregar MT selecionada
    if (carregarConfiguracao(arquivosDisponiveis[arquivoSelecionado])) {
      displayMessage("Sucesso", "MT carregada!", 1500);
      estadoAtual = MENU_PRINCIPAL;
      opcaoSelecionada = 0;
      desenharMenuPrincipal();
    }
  }
}

/**
 * Inicializa editor de fita
 *
 * Posições:
 *   -2: << (voltar)
 *   -1: X (reset)
 *    0: > (iniciar)
 *   1+: células editáveis
 */
void inicializarEditor() {
  // Se fita estiver vazia, adicionar símbolo inicial
  if (entradaUsuario.length() == 0) {
    docGlobal.clear();
    deserializeJson(docGlobal, configAtual);
    entradaUsuario = String(obterPrimeiroSimboloValido());
  }

  // Começar na posição ">" (iniciar)
  posicaoEditor = 0;

  Serial.printf("Editor inicializado: entrada='%s', pos=%d\n", entradaUsuario.c_str(), posicaoEditor);
}


/**
 * Reseta a fita do editor
 */
void resetarFita() {
  entradaUsuario = "";
  inicializarEditor();
  Serial.println("Fita resetada");
}


/**
 * Adiciona novo símbolo no final da fita
 */
void adicionarSimboloEditor() {
  docGlobal.clear();
  deserializeJson(docGlobal, configAtual);
  char novoSimbolo = obterPrimeiroSimboloValido();
  entradaUsuario += novoSimbolo;
  Serial.printf("Símbolo adicionado: %c (nova entrada: %s)\n", novoSimbolo, entradaUsuario.c_str());
}




/**
 * Processa botões no editor de fita
 *
 * Navegação:
 *   PREV: -2 ← -1 ← 0 ← 1 ← 2 ← ...
 *   NEXT: -2 → -1 → 0 → 1 → 2 → ... → novo
 */
void processarEditorFita(int botao) {
  if (botao == 1) {  // PREV - mover para esquerda
    posicaoEditor--;

    // Limite mínimo: posição -2 (voltar)
    if (posicaoEditor < -2) {
      posicaoEditor = -2;
    }

    desenharEditorFita();

  } else if (botao == 3) {  // NEXT - mover para direita
    // Permitir avançar se:
    // 1. Estiver nas posições especiais (-2, -1, 0)
    // 2. OU estiver em uma célula com símbolo (não '+')
    if (posicaoEditor < 0 || posicaoEditor <= entradaUsuario.length()) {
      posicaoEditor++;
    }
    // Se estiver em '+', não avança (precisa definir símbolo primeiro)

    desenharEditorFita();

  } else if (botao == 2) {  // SELECT - ação conforme posição
    if (posicaoEditor == -2) {
      // VOLTAR ao menu principal
      Serial.println("Voltando ao menu principal");
      estadoAtual = MENU_PRINCIPAL;
      opcaoSelecionada = 0;
      desenharMenuPrincipal();

    } else if (posicaoEditor == -1) {
      // RESET fita
      resetarFita();
      desenharEditorFita();

    } else if (posicaoEditor == 0) {
      // INICIAR MT
      if (entradaUsuario.length() == 0) {
        displayMessage("Aviso", "Fita vazia!", 1500);
        desenharEditorFita();
        return;
      }

      // Iniciar conforme modo selecionado
      if (opcaoSelecionada == 0) {
        iniciarExecucaoAutomatica();
      } else {
        iniciarExecucaoPasso();
      }

    } else if (posicaoEditor > 0 && posicaoEditor <= entradaUsuario.length()) {
      // ALTERNAR SÍMBOLO em célula existente
      alternarSimboloEditor();
      desenharEditorFita();

    } else if (posicaoEditor > entradaUsuario.length()) {
      // ADICIONAR NOVO SÍMBOLO
      adicionarSimboloEditor();
      desenharEditorFita();
    }
  }
}

/**
 * Inicia execução automática da MT
 *
 * IMPORTANTE: Usa a MESMA lógica do modo passo-a-passo,
 * mas executa em loop automático
 */
void iniciarExecucaoAutomatica() {
  Serial.println("=== Iniciando Execução Automática ===");
  Serial.printf("Entrada: %s\n", entradaUsuario.c_str());

  // Inicializar máquina (mesma inicialização do passo-a-passo)
  tm.tape = "^" + entradaUsuario;
  for (int i = 0; i < 20; i++) tm.tape += "_";

  tm.headPosition = 0;
  tm.stepCount = 0;

  // Extrair estado inicial do JSON
  docGlobal.clear();
  deserializeJson(docGlobal, configAtual);
  tm.currentState = docGlobal["initialState"] | "q0";

  Serial.printf("Estado inicial: %s\n", tm.currentState.c_str());
  Serial.printf("Fita inicial: %s\n", tm.tape.c_str());

  // Mostrar estado inicial com primeira transição
  displayMessage("Executando", "Iniciando...", 1000);

  char firstSymbol = tm.tape[tm.headPosition];
  TransitionInfo firstTrans = buscarTransicao(tm.currentState, firstSymbol);
  String firstTransInfo = "trans: ";

  if (isEstadoFinal(tm.currentState)) {
    firstTransInfo += tm.currentState + " (final)";
  } else if (firstTrans.found && isEstadoFinal(firstTrans.nextState)) {
    // A próxima transição levará a um estado final
    firstTransInfo += firstTrans.nextState + " (final)";
  } else if (firstTrans.found) {
    firstTransInfo += firstTrans.nextState + " | " + firstTrans.newSymbol + " | ";
    if (firstTrans.direction == "L") firstTransInfo += "E";
    else if (firstTrans.direction == "R") firstTransInfo += "D";
    else firstTransInfo += "S";
  } else {
    firstTransInfo += "-";
  }

  drawTape(tm.tape, tm.headPosition, tm.currentState, tm.stepCount, firstTransInfo);
  delay(1000);

  // Loop de execução automática
  bool continuar = true;
  int maxSteps = 1000;  // Proteção contra loop infinito

  while (continuar && tm.stepCount < maxSteps) {
    // Executar próximo passo usando a MESMA função do modo passo-a-passo
    continuar = executarPassoAutomatico();

    // Delay entre passos para visualização
    if (continuar) {
      delay(500);  // 500ms entre passos
    }
  }

  // Verificar resultado final
  bool accepted = isEstadoFinal(tm.currentState);
  String titulo = accepted ? "ACEITO" : "REJEITADO";
  String motivo = accepted ? "Estado final" : "Sem transicao";

  Serial.printf("\n=== Resultado: %s ===\n", titulo.c_str());
  Serial.printf("Motivo: %s\n", motivo.c_str());
  Serial.printf("Passos executados: %d\n", tm.stepCount);
  Serial.printf("Estado final: %s\n", tm.currentState.c_str());
  Serial.printf("Fita final: %s\n", tm.tape.c_str());

  mostrarResultadoFinal(titulo, motivo, tm.stepCount);

  // Voltar ao menu principal
  estadoAtual = MENU_PRINCIPAL;
  opcaoSelecionada = 0;
  desenharMenuPrincipal();
}


/**
 * Executa um passo no modo automático
 * Retorna: true se deve continuar, false se terminou
 */
bool executarPassoAutomatico() {
  docGlobal.clear();
  deserializeJson(docGlobal, configAtual);

  // Verificar se está em estado final
  if (isEstadoFinal(tm.currentState)) {
    Serial.println("Estado final atingido");
    return false;
  }

  // Validar posição da cabeça
  if (tm.headPosition < 0 || tm.headPosition >= tm.tape.length()) {
    Serial.println("Erro: Cabeça fora da fita");
    displayMessage("ERRO", "Cabeca fora da fita", 2000);
    return false;
  }

  // Buscar e validar transição
  char currentSymbol = tm.tape[tm.headPosition];
  Serial.printf("Passo %d: estado=%s, pos=%d, simbolo=%c\n",
                tm.stepCount, tm.currentState.c_str(), tm.headPosition, currentSymbol);

  TransitionInfo transition = buscarTransicao(tm.currentState, currentSymbol);

  // Se NÃO há transição: parar (será tratado no final)
  if (!transition.found) {
    Serial.println("Sem transição disponível");
    return false;
  }

  Serial.printf("Transição: (%s, %s, %s)\n",
                transition.nextState.c_str(), transition.newSymbol.c_str(), transition.direction.c_str());

  // Salvar estado antigo para animação
  String oldTape = tm.tape;
  int oldPosition = tm.headPosition;
  String oldState = tm.currentState;

  // Aplicar transição e animar
  int newPosition = aplicarTransicao(transition);
  animateTransition(oldTape, tm.tape, oldPosition, newPosition,
                     oldState, tm.currentState, tm.stepCount);

  tm.headPosition = newPosition;
  tm.stepCount++;

  // Parar se chegou em estado final
  if (isEstadoFinal(tm.currentState)) {
    return false;
  }

  return true;
}

/**
 * Inicia execução passo-a-passo da MT
 */
void iniciarExecucaoPasso() {
  Serial.println("=== Iniciando Execução Passo-a-Passo ===");
  Serial.printf("Entrada: %s\n", entradaUsuario.c_str());

  // Inicializar máquina (sem executar)
  tm.tape = "^" + entradaUsuario;
  for (int i = 0; i < 20; i++) tm.tape += "_";

  tm.headPosition = 0;
  tm.stepCount = 0;

  // Extrair estado inicial do JSON
  docGlobal.clear();
  deserializeJson(docGlobal, configAtual);
  tm.currentState = docGlobal["initialState"] | "q0";

  estadoAtual = EXECUTANDO_PASSO;
  aguardandoProximoPasso = true;

  // Buscar próxima transição e formatar
  char currentSymbol = tm.tape[tm.headPosition];
  TransitionInfo transition = buscarTransicao(tm.currentState, currentSymbol);

  String transInfo = "trans: ";

  // Se já está em estado final, mostrar isso
  if (isEstadoFinal(tm.currentState)) {
    transInfo += tm.currentState + " (final)";
  } else if (transition.found && isEstadoFinal(transition.nextState)) {
    // A próxima transição levará a um estado final
    transInfo += transition.nextState + " (final)";
  } else if (transition.found) {
    transInfo += transition.nextState + " | " + transition.newSymbol + " | ";
    if (transition.direction == "L") transInfo += "E";
    else if (transition.direction == "R") transInfo += "D";
    else transInfo += "S";
  } else {
    // Não há transição e não é estado final = será rejeitado
    transInfo += "-";
  }

  // Desenhar estado inicial com próxima transição
  drawTape(tm.tape, tm.headPosition, tm.currentState, tm.stepCount, transInfo);
}

/**
 * Processa botões durante execução passo-a-passo
 */
void processarExecucaoPasso(int botao) {
  if (botao == 2) {  // SELECT - executar próximo passo
    executarProximoPasso();
  }
}

/**
 * Executa um passo da MT no modo passo-a-passo
 */
void executarProximoPasso() {
  Serial.printf("Executando passo %d\n", tm.stepCount);

  docGlobal.clear();
  deserializeJson(docGlobal, configAtual);

  // Validar posição da cabeça
  if (tm.headPosition < 0 || tm.headPosition >= tm.tape.length()) {
    displayMessage("ERRO", "Cabeca fora da fita", 2000);
    estadoAtual = MENU_PRINCIPAL;
    opcaoSelecionada = 0;
    desenharMenuPrincipal();
    return;
  }

  // Buscar transição usando função auxiliar
  char currentSymbol = tm.tape[tm.headPosition];
  TransitionInfo transition = buscarTransicao(tm.currentState, currentSymbol);

  // Se está em estado final E não há transição: ACEITAR
  if (isEstadoFinal(tm.currentState) && !transition.found) {
    mostrarResultadoFinal("ACEITO", "Estado final", tm.stepCount);
    estadoAtual = MENU_PRINCIPAL;
    opcaoSelecionada = 0;
    desenharMenuPrincipal();
    return;
  }

  // Se NÃO está em estado final E não há transição: REJEITAR
  if (!transition.found) {
    mostrarResultadoFinal("REJEITADO", "Sem transicao", tm.stepCount);
    estadoAtual = MENU_PRINCIPAL;
    opcaoSelecionada = 0;
    desenharMenuPrincipal();
    return;
  }

  // Salvar estado antigo para animação
  String oldTape = tm.tape;
  int oldPosition = tm.headPosition;
  String oldState = tm.currentState;

  // Aplicar transição e animar
  int newPosition = aplicarTransicao(transition);
  animateTransition(oldTape, tm.tape, oldPosition, newPosition,
                     oldState, tm.currentState, tm.stepCount);

  tm.headPosition = newPosition;
  tm.stepCount++;

  // Verificar se chegou em estado final
  if (isEstadoFinal(tm.currentState)) {
    mostrarResultadoFinal("ACEITO", "Estado final", tm.stepCount);
    estadoAtual = MENU_PRINCIPAL;
    opcaoSelecionada = 0;
    desenharMenuPrincipal();
    return;
  }

  // Verificar se não há próxima transição
  char nextSymbol = tm.tape[tm.headPosition];
  TransitionInfo nextTrans = buscarTransicao(tm.currentState, nextSymbol);

  if (!nextTrans.found) {
    mostrarResultadoFinal("REJEITADO", "Sem transicao", tm.stepCount);
    estadoAtual = MENU_PRINCIPAL;
    opcaoSelecionada = 0;
    desenharMenuPrincipal();
    return;
  }
}

/**
 * Processa botões na tela de resultado final
 */
void processarResultadoFinal(int botao) {
  if (botao == 2) {  // SELECT - voltar ao menu
    estadoAtual = MENU_PRINCIPAL;
    opcaoSelecionada = 0;
    desenharMenuPrincipal();
  }
}
