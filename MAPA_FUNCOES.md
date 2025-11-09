# MAPA DE FUNÇÕES - ESP32 Turing Machine

Este documento explica **cada função** do código e mapeia **todos os fluxos de execução** do projeto. Pensado como material didático para compreensão completa do sistema.

---

## ÍNDICE

1. [Visão Geral da Arquitetura](#visão-geral-da-arquitetura)
2. [Estruturas de Dados](#estruturas-de-dados)
3. [Funções Core (Núcleo da Máquina de Turing)](#funções-core-núcleo-da-máquina-de-turing)
4. [Funções de Display OLED](#funções-de-display-oled)
5. [Funções de Menu e Navegação](#funções-de-menu-e-navegação)
6. [Funções de Execução da MT](#funções-de-execução-da-mt)
7. [Funções de Entrada (Botões)](#funções-de-entrada-botões)
8. [Funções de Sistema de Arquivos](#funções-de-sistema-de-arquivos)
9. [Funções de Rede (WiFi e AP)](#funções-de-rede-wifi-e-ap)
10. [Funções de Servidor Web e API](#funções-de-servidor-web-e-api)
11. [Fluxos de Execução Completos](#fluxos-de-execução-completos)

---

## Visão Geral da Arquitetura

```
┌─────────────────────────────────────────────────────────────┐
│                     ESP32 TURING MACHINE                     │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────┐      ┌──────────────┐                    │
│  │  Interface   │      │   Interface  │                    │
│  │     Web      │◄────►│    Física    │                    │
│  │  (Navegador) │      │(Display+Btn) │                    │
│  └──────┬───────┘      └──────┬───────┘                    │
│         │                     │                             │
│         │   ┌─────────────────┴──────────────┐             │
│         │   │   FUNÇÕES HELPER CENTRALIZADAS │             │
│         │   │  • buscarTransicao()           │             │
│         └──►│  • aplicarTransicao()          │◄────────┐   │
│             │  • isEstadoFinal()             │         │   │
│             └─────────────────┬──────────────┘         │   │
│                               │                        │   │
│         ┌─────────────────────┴──────────────┐         │   │
│         │      CAMADAS DE EXECUÇÃO           │         │   │
│         ├────────────────────────────────────┤         │   │
│         │ 1. Menu Físico Automático          │─────────┤   │
│         │ 2. Menu Físico Passo a Passo       │─────────┤   │
│         │ 3. API Web - TuringMachine::execute│─────────┤   │
│         │ 4. API Web - Execute Display       │─────────┤   │
│         │ 5. API Web - Start Step Mode       │─────────┘   │
│         └────────────────────────────────────┘             │
│                                                              │
│  ┌──────────────┐      ┌──────────────┐                    │
│  │  LittleFS    │      │   WiFi/AP    │                    │
│  │(Persistência)│      │  (Servidor)  │                    │
│  └──────────────┘      └──────────────┘                    │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

**PRINCÍPIO FUNDAMENTAL**: Todos os 5 modos de execução usam as **mesmas 3 funções helper** para garantir comportamento idêntico e previsível.

---

## Estruturas de Dados

### `TransitionInfo`

**Localização**: Linhas 164-169

**Propósito**: Armazena informações de uma transição da MT.

```cpp
struct TransitionInfo {
  bool found;          // Transição foi encontrada?
  String nextState;    // Próximo estado (ex: "q1")
  String newSymbol;    // Símbolo a escrever (ex: "x")
  String direction;    // Direção: "L", "R" ou "STOP"
};
```

**Uso**:
- Retornada por `buscarTransicao()`
- Consumida por `aplicarTransicao()`
- Permite separar busca de aplicação de transições

---

### `ExecutionResult`

**Localização**: Linhas 174-180

**Propósito**: Armazena resultado completo da execução de uma MT.

```cpp
struct ExecutionResult {
  bool accepted;     // MT aceitou a entrada?
  int steps;         // Quantidade de passos executados
  String finalTape;  // Estado final da fita
  String message;    // Mensagem de resultado
  String history;    // JSON com histórico de todos os passos
};
```

**Uso**:
- Retornado por `TuringMachine::execute()`
- Convertido para JSON nas APIs
- Permite análise pós-execução

---

## Funções Core (Núcleo da Máquina de Turing)

Estas são as **funções centralizadas** que implementam a lógica da MT. Todos os modos de execução usam estas funções.

---

### `buscarTransicao()` - Versão 1 (Com JsonObject)

**Localização**: Linhas 464-508

**Assinatura**:
```cpp
TransitionInfo buscarTransicao(String currentState, char currentSymbol, JsonObject config)
```

**Parâmetros**:
- `currentState`: Estado atual da MT (ex: "q0")
- `currentSymbol`: Símbolo lido da fita (ex: '1')
- `config`: Objeto JSON com a configuração da MT

**Retorno**: `TransitionInfo` com os dados da transição (ou `found=false` se não existir)

**O que faz**:

1. Busca o estado atual no objeto `transitions` do JSON
2. Dentro do estado, busca o símbolo atual
3. Extrai `nextState`, `newSymbol` e `direction`
4. Retorna struct preenchida

**Exemplo de busca**:

```
Estado atual: "q0"
Símbolo atual: '1'

JSON:
{
  "transitions": {
    "q0": {
      "1": {
        "nextState": "q1",
        "newSymbol": "x",
        "direction": "R"
      }
    }
  }
}

Resultado:
TransitionInfo {
  found: true,
  nextState: "q1",
  newSymbol: "x",
  direction: "R"
}
```

**Mensagens de debug**:
```
✓ Transição encontrada: q0 --1--> q1 (escrever: x, mover: R)
✗ Transição não encontrada para estado q0, símbolo 1
```

---

### `buscarTransicao()` - Versão 2 (Com variáveis globais)

**Localização**: Linhas 510-515

**Assinatura**:
```cpp
TransitionInfo buscarTransicao(String currentState, char currentSymbol)
```

**Parâmetros**:
- `currentState`: Estado atual da MT
- `currentSymbol`: Símbolo lido da fita

**O que faz**:

Simplesmente chama a versão completa usando as variáveis globais:
```cpp
return buscarTransicao(currentState, currentSymbol, docGlobal.as<JsonObject>());
```

**Por que existe**: Permite que funções do menu físico (que usam estado global `docGlobal`) chamem a função de forma mais simples, sem precisar passar o JSON explicitamente.

---

### `isEstadoFinal()` - Versão 1 (Com variáveis globais)

**Localização**: Linhas 517-528

**Assinatura**:
```cpp
bool isEstadoFinal(String state)
```

**Parâmetros**:
- `state`: Nome do estado a verificar (ex: "q_accept")

**Retorno**: `true` se o estado está na lista de estados finais, `false` caso contrário

**O que faz**:

1. Busca array `finalStates` no JSON global `docGlobal`
2. Itera pelos estados finais
3. Compara cada um com o estado fornecido
4. Retorna `true` se encontrar match

**Exemplo**:
```
Estado: "q_accept"

JSON:
{
  "finalStates": ["q_accept", "q_halt"]
}

Resultado: true
```

---

### `isEstadoFinal()` - Versão 2 (Com JsonObject)

**Localização**: Linhas 530-548

**Assinatura**:
```cpp
bool isEstadoFinal(String state, JsonObject config)
```

**Parâmetros**:
- `state`: Nome do estado a verificar
- `config`: Objeto JSON com a configuração da MT

**O que faz**: Idêntico à versão 1, mas aceita JSON explícito ao invés de usar `docGlobal`.

**Por que existem 2 versões**: Mesma razão de `buscarTransicao()` - flexibilidade para menu físico (usa global) e API web (usa JSON da requisição).

---

### `aplicarTransicao()` - Versão 1 (Com parâmetros explícitos)

**Localização**: Linhas 550-580

**Assinatura**:
```cpp
int aplicarTransicao(TransitionInfo transition, String &tape, int headPosition, String &currentState)
```

**Parâmetros**:
- `transition`: Transição a aplicar (retornada por `buscarTransicao()`)
- `tape`: Referência à fita (será modificada)
- `headPosition`: Posição atual da cabeça
- `currentState`: Referência ao estado atual (será modificado)

**Retorno**: Nova posição da cabeça (int)

**O que faz**:

1. **Escrever símbolo**: Substitui o símbolo na posição atual da fita
   ```cpp
   tape[headPosition] = transition.newSymbol[0];
   ```

2. **Atualizar estado**: Muda o estado atual
   ```cpp
   currentState = transition.nextState;
   ```

3. **Mover cabeça**: Move left (L), right (R) ou para (STOP)
   ```cpp
   if (direction == "L") return headPosition - 1;
   else if (direction == "R") return headPosition + 1;
   else return headPosition;
   ```

**Exemplo de aplicação**:

```
Antes:
  Fita: "^101___"
  Posição: 1
  Estado: "q0"

Transição:
  nextState: "q1"
  newSymbol: "x"
  direction: "R"

Depois:
  Fita: "^x01___"
  Posição: 2
  Estado: "q1"
  Retorno: 2
```

**Mensagens de debug**:
```
→ Aplicando transição: escrever 'x', mover R, novo estado: q1
→ Nova posição da cabeça: 2
```

---

### `aplicarTransicao()` - Versão 2 (Com variáveis globais)

**Localização**: Linhas 582-592

**Assinatura**:
```cpp
int aplicarTransicao(TransitionInfo transition)
```

**Parâmetros**:
- `transition`: Transição a aplicar

**Retorno**: Nova posição da cabeça

**O que faz**:

Chama versão completa usando variáveis globais do menu físico:
```cpp
return aplicarTransicao(transition, fitaGlobal, posicaoGlobal, estadoAtualGlobal);
```

**Por que existe**: Simplifica código do menu físico que já trabalha com estado global.

---

### Funções Auxiliares de Símbolo

#### `isSimboloValidoParaEntrada()`

**Localização**: Linhas 594-599

**O que faz**: Verifica se um símbolo pertence ao alfabeto de entrada (não aos símbolos auxiliares `^` e `_`).

#### `obterPrimeiroSimboloValido()`

**Localização**: Linhas 601-620

**O que faz**: Retorna o primeiro símbolo do alfabeto de entrada (usado para inicializar editor de fita).

#### `obterSimbolosValidosString()`

**Localização**: Linhas 622-648

**O que faz**: Retorna string com todos os símbolos válidos para entrada (usado em mensagens de erro).

---

## Funções de Display OLED

Responsáveis por toda a interface visual no display SSD1306.

---

### `inicializarDisplay()`

**Localização**: Linhas 665-703

**O que faz**:

1. Inicia comunicação I2C com pinos SDA=5, SCL=4
2. Tenta inicializar display no endereço 0x3C
3. Se sucesso:
   - Configura cor branca
   - Define rotação (se necessário)
   - Exibe splash screen "TURING MACHINE / ESP32"
   - Marca `displayAtivo = true`
4. Se falha:
   - Marca `displayAtivo = false`
   - Sistema continua funcionando (apenas sem display)

**Retorno**: `true` se display foi inicializado, `false` caso contrário

**Mensagens Serial**:
```
✓ Display OLED inicializado (128x64)
✗ Falha ao inicializar display OLED
```

---

### `drawTape()`

**Localização**: Linhas 727-808

**Assinatura**:
```cpp
void drawTape(String tape, int headPos, String currentState, int stepCount, String statusMsg)
```

**Parâmetros**:
- `tape`: Conteúdo da fita (string completa)
- `headPos`: Posição da cabeça de leitura
- `currentState`: Estado atual da MT
- `stepCount`: Número de passos executados
- `statusMsg`: Mensagem de status (ex: "Executando...")

**O que faz**:

1. **Limpa tela**
2. **Desenha header** (linha superior):
   ```
   q0 | #3
   ```
   (Estado atual e contador de passos)

3. **Desenha fita** (células visíveis):
   - Calcula células visíveis centralizadas na cabeça
   - Desenha 7 células (constante `VISIBLE_CELLS`)
   - Célula sob a cabeça é destacada (retângulo invertido)

4. **Desenha seta indicadora** (▲) sob a cabeça

5. **Desenha status** (linha inferior):
   ```
   Executando...
   ```

**Visualização**:
```
┌──────────────────────┐
│ q0 | #3               │
│                       │
│ ┌──┬──┬──┬──┬──┬──┬─ │
│ │^ │1 │0 │1 │_ │_ │_ │
│ └──┴──┴──┴──┴──┴──┴─ │
│       ▲               │
│ Executando...         │
└──────────────────────┘
```

---

### `animateTransition()`

**Localização**: Linhas 810-974

**Assinatura**:
```cpp
void animateTransition(String oldTape, String newTape, int oldPos, int newPos,
                      String oldState, String newState, int stepCount)
```

**Parâmetros**:
- `oldTape`, `newTape`: Fita antes e depois
- `oldPos`, `newPos`: Posição da cabeça antes e depois
- `oldState`, `newState`: Estado antes e depois
- `stepCount`: Número do passo atual

**O que faz**:

Cria uma **animação suave** mostrando a transição:

1. **Frame 1**: Estado antes da transição (200ms)
2. **Frame 2**: Pisca célula que está sendo modificada (200ms)
3. **Frame 3**: Estado depois da transição (200ms)

**Uso**: Chamada durante execução automática para visualização didática.

---

### `displayMessage()`

**Localização**: Linhas 976-1003

**Assinatura**:
```cpp
void displayMessage(String title, String message, int delayMs = 2000)
```

**Parâmetros**:
- `title`: Título da mensagem (linha superior)
- `message`: Corpo da mensagem (centralizada)
- `delayMs`: Tempo de exibição em milissegundos

**O que faz**:

Exibe mensagem centralizada no display, útil para:
- Avisos ("Carregando...")
- Erros ("Arquivo não encontrado")
- Status ("WiFi conectado")

**Layout**:
```
┌──────────────────────┐
│ TÍTULO               │
│                       │
│     MENSAGEM         │
│   CENTRALIZADA       │
│                       │
└──────────────────────┘
```

---

### `mostrarResultadoFinal()`

**Localização**: Linhas 1005-1041

**Assinatura**:
```cpp
void mostrarResultadoFinal(String titulo, String motivo, int passos, bool waitForButton = true)
```

**Parâmetros**:
- `titulo`: "ACEITO" ou "REJEITADO"
- `motivo`: Razão (ex: "Estado final", "Sem transição")
- `passos`: Quantidade de passos executados
- `waitForButton`: Se `true`, aguarda botão; se `false`, delay de 3s

**O que faz**:

Exibe tela final padronizada com resultado da execução:

```
┌──────────────────────┐
│ ACEITO               │
│                       │
│ Estado final         │
│ Passos: 47           │
│                       │
│ [Pressione botão]    │ (ou aguarda 3s)
└──────────────────────┘
```

**IMPORTANTE**: Esta é a função usada por **TODOS** os modos de execução para garantir tela final idêntica:
- Menu físico: `waitForButton = true` (padrão)
- API web: `waitForButton = false` (não bloqueia servidor)

---

## Funções de Menu e Navegação

Implementam a navegação pela interface física do ESP32.

---

### `desenharMenuPrincipal()`

**Localização**: Linhas 1305-1347

**O que faz**:

Desenha o menu principal com 4 opções:

```
┌──────────────────────┐
│ ┌──────────────────┐ │
│ │► Executar (AUTO) │ │  ← opcaoSelecionada = 0
│ └──────────────────┘ │
│   Executar (PASSO)   │
│   Editar Entrada     │
│   Sair               │
└──────────────────────┘
```

**Variáveis usadas**:
- `opcaoSelecionada`: Índice da opção destacada (0-3)

**Opções**:
0. Executar (AUTO) - Execução automática com delay
1. Executar (PASSO) - Controle passo a passo
2. Editar Entrada - Modificar string de entrada
3. Sair - Desligar display

---

### `desenharMenuSelecaoMT()`

**Localização**: Linhas 1349-1406

**O que faz**:

Exibe lista de MTs salvas no LittleFS para seleção:

```
┌──────────────────────┐
│ Selecionar MT:       │
│                       │
│ ┌──────────────────┐ │
│ │► palindromo.json │ │
│ └──────────────────┘ │
│   exemplo.json       │
└──────────────────────┘
```

**Variáveis usadas**:
- `arquivosMT[]`: Array com nomes dos arquivos
- `totalArquivosMT`: Quantidade de arquivos
- `arquivoSelecionado`: Índice do arquivo destacado

---

### `desenharEditorFita()`

**Localização**: Linhas 1408-1498

**O que faz**:

Mostra editor de string de entrada:

```
┌──────────────────────┐
│ Editar Entrada:      │
│                       │
│ Fita: 101            │
│       ▲              │ (cursor piscante)
│                       │
│ BACK  OK   NEXT      │
└──────────────────────┘
```

**Controles**:
- BACK: Retorna ao menu sem salvar
- SELECT (OK): Confirma e salva entrada
- NEXT: Cicla entre símbolos do alfabeto na posição atual

**Variáveis usadas**:
- `fitaEditor`: String sendo editada
- `cursorEditor`: Posição do cursor

---

## Funções de Execução da MT

Implementam os diferentes modos de execução.

---

### `iniciarExecucaoAutomatica()`

**Localização**: Linhas 2377-2456

**O que faz**:

Prepara e inicia execução automática (com delay entre passos):

1. **Inicializar estado**:
   ```cpp
   estadoAtualGlobal = estadoInicial;
   fitaGlobal = "^" + entradaUsuario + "____...";
   posicaoGlobal = 0;
   passoGlobal = 0;
   ```

2. **Marcar modo**:
   ```cpp
   estadoAtual = EXECUTANDO_AUTO;
   ```

3. **Mostrar fita inicial** no display

**Próximo passo**: O `loop()` chama `executarPassoAutomatico()` repetidamente.

---

### `executarPassoAutomatico()`

**Localização**: Linhas 2458-2513

**Retorno**: `true` se execução continua, `false` se terminou

**O que faz em cada passo**:

1. **Ler símbolo atual**:
   ```cpp
   char simboloAtual = fitaGlobal[posicaoGlobal];
   ```

2. **Buscar transição** (usando função helper):
   ```cpp
   TransitionInfo trans = buscarTransicao(estadoAtualGlobal, simboloAtual);
   ```

3. **Se não encontrou transição**:
   - Mostrar "REJEITADO - Sem transição"
   - Mudar para `estadoAtual = RESULTADO_FINAL`
   - Retornar `false`

4. **Se encontrou transição**:
   - Salvar estado antigo (para animação)
   - **Aplicar transição** (usando função helper)
   - Mostrar animação da transição
   - Incrementar contador de passos

5. **Verificar se chegou em estado final**:
   ```cpp
   if (isEstadoFinal(estadoAtualGlobal)) {
     mostrarResultadoFinal("ACEITO", "Estado final", passoGlobal);
     estadoAtual = RESULTADO_FINAL;
     return false;
   }
   ```

6. **Verificar limite de passos**:
   ```cpp
   if (passoGlobal >= 1000) {
     mostrarResultadoFinal("REJEITADO", "Limite de passos", passoGlobal);
     return false;
   }
   ```

7. **Delay e continuar**:
   ```cpp
   delay(500);
   return true;
   ```

---

### `iniciarExecucaoPasso()`

**Localização**: Linhas 2515-2561

**O que faz**:

Prepara execução passo a passo (controle manual via botões):

1. **Inicializar estado** (idêntico ao modo automático)
2. **Marcar modo**:
   ```cpp
   estadoAtual = EXECUTANDO_PASSO;
   ```
3. **Mostrar fita inicial**
4. **Mostrar instruções**:
   ```
   BACK=Menu
   SELECT=Passo
   ```

**Próximo passo**: Aguarda usuário pressionar SELECT (processado em `processarExecucaoPasso()`).

---

### `executarProximoPasso()`

**Localização**: Linhas 2572-2645

**O que faz**:

Executa **UM ÚNICO PASSO** da MT (chamado quando usuário pressiona SELECT):

**Lógica idêntica** a `executarPassoAutomatico()`, mas:
- ❌ Sem delay automático
- ❌ Sem loop contínuo
- ✅ Retorna controle ao usuário após cada passo
- ✅ Aguarda próximo pressionamento de botão

---

## Funções de Entrada (Botões)

---

### `inicializarBotoes()`

**Localização**: Linhas 705-725

**O que faz**:

Configura os 3 botões como entrada com pull-up interno:

```cpp
pinMode(BTN_PREV, INPUT_PULLUP);   // GPIO 12
pinMode(BTN_SELECT, INPUT_PULLUP); // GPIO 14
pinMode(BTN_NEXT, INPUT_PULLUP);   // GPIO 2
```

**Pull-up interno**: Pino fica em HIGH por padrão, vai para LOW quando botão é pressionado.

---

### `lerBotao()`

**Localização**: Linhas 1162-1194

**Retorno**:
- `1` = PREV (◄)
- `2` = SELECT (✓)
- `3` = NEXT (►)
- `0` = Nenhum botão

**O que faz**:

1. **Lê estado dos 3 botões**
2. **Implementa debounce** (ignora ruído):
   - Aguarda 50ms
   - Relê botões
   - Só considera pressionado se ambas leituras forem iguais
3. **Aguarda soltura** (evita repetição):
   - Loop até todos os botões voltarem a HIGH
4. **Delay anti-bounce** final (150ms)

**Mensagens de debug**:
```
Botão pressionado: PREV
Botão pressionado: SELECT
Botão pressionado: NEXT
```

---

### Processadores de Botões (por Estado)

Cada estado do sistema tem seu próprio processador:

#### `processarMenuPrincipal()`
**Localização**: Linhas 2169-2221

- PREV/NEXT: Muda `opcaoSelecionada` (0-3)
- SELECT: Executa ação da opção selecionada

#### `processarMenuSelecaoMT()`
**Localização**: Linhas 2223-2260

- PREV/NEXT: Navega entre arquivos
- SELECT: Carrega MT selecionada

#### `processarEditorFita()`
**Localização**: Linhas 2308-2375

- PREV: Cancela edição
- SELECT: Salva entrada editada
- NEXT: Alterna símbolo na posição do cursor

#### `processarExecucaoPasso()`
**Localização**: Linhas 2563-2570

- PREV: Volta ao menu
- SELECT: Executa próximo passo
- NEXT: (não usado)

#### `processarResultadoFinal()`
**Localização**: Linhas 2647-2655

- Qualquer botão: Retorna ao menu principal

---

## Funções de Sistema de Arquivos

---

### `inicializarLittleFS()`

**Localização**: Linhas 1546-1555

**O que faz**:

1. Monta sistema de arquivos LittleFS
2. Se falhar, exibe erro no Serial
3. Se sucesso, lista arquivos disponíveis

**Retorno**: `true` se montou com sucesso, `false` caso contrário

---

### `listarArquivos()`

**Localização**: Linhas 1557-1565

**O que faz**:

Percorre diretório raiz (`/`) e imprime lista de arquivos no Serial Monitor:

```
Arquivos no LittleFS:
  /index.html (4532 bytes)
  /styles.css (2341 bytes)
  /script.js (8765 bytes)
  /config.json (234 bytes)
  /palindromo.json (1203 bytes)
```

---

### `carregarListaArquivos()`

**Localização**: Linhas 1196-1227

**O que faz**:

Carrega lista de arquivos `.json` (MTs salvas) no array `arquivosMT[]`:

1. Abre diretório raiz
2. Percorre arquivos
3. Filtra apenas `.json` **exceto** `config.json`
4. Armazena nomes em `arquivosMT[]`
5. Atualiza `totalArquivosMT`

**Uso**: Chamada antes de exibir menu de seleção de MT.

---

### `carregarConfiguracao()`

**Localização**: Linhas 1229-1303

**Assinatura**:
```cpp
bool carregarConfiguracao(String nomeArquivo)
```

**O que faz**:

1. **Abre arquivo** JSON do LittleFS
2. **Lê conteúdo** completo
3. **Parseia JSON** para `docGlobal`
4. **Extrai campos**:
   - `estadoInicial`: Estado inicial da MT
   - `alphabet`: Símbolos de entrada
   - `transitions`: Tabela de transições
   - `finalStates`: Estados de aceitação
5. **Valida** presença de campos obrigatórios
6. **Exibe resumo** no Serial:
   ```
   ✓ Configuração carregada: palindromo.json
   Estado inicial: q0
   Alfabeto: 0, 1
   Estados: 7
   ```

**Retorno**: `true` se carregou com sucesso, `false` em caso de erro

**Variáveis preenchidas**:
- `docGlobal`: JSON completo da configuração
- `estadoInicial`: String com estado inicial
- `entradaUsuario`: String de entrada (vazia inicialmente)

---

## Funções de Rede (WiFi e AP)

---

### `conectarWiFi()`

**Localização**: Linhas 1567-1588

**O que faz**:

1. **Inicia conexão** com SSID e senha configurados
2. **Aguarda até 10 segundos** (TIMEOUT_WIFI)
3. **A cada 500ms**: Imprime "." e verifica conexão
4. **Se conectou**:
   - Imprime IP local
   - Retorna `true`
5. **Se timeout**:
   - Retorna `false`

**Mensagens**:
```
Conectando ao WiFi: MARCIO-WIFI...
✓ WiFi conectado!
IP: 192.168.0.111
```

**Retorno**: `true` se conectou, `false` se falhou

---

### `iniciarAP()`

**Localização**: Linhas 1590-1611

**O que faz**:

Cria Access Point (rede própria do ESP32):

1. **Inicia modo AP** com SSID e senha configurados
2. **IP fixo**: `192.168.4.1`
3. **Inicia DNS server** (captive portal):
   - Redireciona todas as requisições DNS para o IP do ESP32
   - Faz com que qualquer URL digitada abra a interface

**Mensagens**:
```
✓ Modo AP ativado
SSID: ESP32_TuringMachine
Senha: 123
IP: 192.168.4.1
```

**Uso**: Chamada quando conexão WiFi falha.

---

## Funções de Servidor Web e API

---

### `addCORSHeaders()`

**Localização**: Linhas 651-663

**O que faz**:

Adiciona cabeçalhos CORS (Cross-Origin Resource Sharing) à resposta HTTP:

```cpp
server.sendHeader("Access-Control-Allow-Origin", "*");
server.sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
```

**Por que**: Permite que páginas web de outras origens acessem a API do ESP32.

---

### `servirArquivo()`

**Localização**: Linhas 1613-1624

**Assinatura**:
```cpp
void servirArquivo(const char* caminho, const char* contentType)
```

**O que faz**:

1. Abre arquivo do LittleFS
2. Se não existir, retorna 404
3. Se existir, envia conteúdo com `streamFile()`
4. Fecha arquivo

**Uso**: Servir HTML, CSS, JS, JSON.

---

### Handlers de Arquivos Estáticos

#### `handleRoot()` - Linha 1626
Serve `/index.html`

#### `handleCSS()` - Linha 1630
Serve `/styles.css`

#### `handleJS()` - Linha 1634
Serve `/script.js`

---

### `handleStatus()`

**Localização**: Linhas 1638-1656

**Endpoint**: `GET /status`

**O que faz**:

Retorna status do ESP32 em JSON:

```json
{
  "status": "ok",
  "freeHeap": 245632,
  "chipModel": "ESP32-D0WDQ6",
  "cpuFreq": 240,
  "wifiMode": "AP",
  "ip": "192.168.4.1"
}
```

**Uso**: Monitoramento e debug via API.

---

### `handleSave()`

**Localização**: Linhas 1667-1704

**Endpoint**: `POST /api/save`

**Body esperado**:
```json
{
  "filename": "minha_mt",
  "config": { ... }
}
```

**O que faz**:

1. **Parseia JSON** da requisição
2. **Adiciona extensão** `.json` ao filename se necessário
3. **Abre arquivo** no LittleFS para escrita
4. **Serializa config** para o arquivo
5. **Fecha arquivo**
6. **Retorna resposta**:
   ```json
   {"success": true, "message": "Arquivo salvo: minha_mt.json"}
   ```

**Tratamento de erros**:
- JSON inválido → 400 Bad Request
- Erro ao escrever → 500 Internal Server Error

---

### `handleLoad()`

**Localização**: Linhas 1706-1735

**Endpoint**: `GET /api/load?filename=palindromo.json`

**O que faz**:

1. **Extrai parâmetro** `filename` da URL
2. **Adiciona extensão** `.json` se necessário
3. **Abre arquivo** do LittleFS
4. **Lê conteúdo** completo
5. **Retorna JSON**:
   ```json
   {
     "success": true,
     "config": { ... }
   }
   ```

**Tratamento de erros**:
- Arquivo não existe → `{"success": false, "message": "Arquivo não encontrado"}`

---

### `handleListFiles()`

**Localização**: Linhas 1737-1763

**Endpoint**: `GET /api/files`

**O que faz**:

Retorna lista de arquivos salvos:

```json
{
  "files": [
    {"name": "palindromo.json", "size": 1203},
    {"name": "exemplo.json", "size": 856}
  ]
}
```

**Observação**: Filtra apenas arquivos `.json`, excluindo `config.json`.

---

### `handleDelete()`

**Localização**: Linhas 1765-1785

**Endpoint**: `DELETE /api/delete?filename=exemplo.json`

**O que faz**:

1. **Extrai parâmetro** `filename`
2. **Remove arquivo** do LittleFS
3. **Retorna resposta**:
   ```json
   {"success": true, "message": "Arquivo deletado"}
   ```

**Tratamento de erros**:
- Falha ao deletar → `{"success": false}`

---

### `handleExecute()`

**Localização**: Linhas 1787-1855

**Endpoint**: `POST /api/execute`

**Body esperado**:
```json
{
  "input": "101",
  "config": { ... }
}
```

**O que faz**:

1. **Parseia requisição**
2. **Cria instância** de `TuringMachine`
3. **Executa MT**:
   ```cpp
   ExecutionResult result = tm.execute(input, configStr, false, 0);
   ```
   - `useDisplay = false` (sem visualização OLED)
4. **Retorna resultado** em JSON:
   ```json
   {
     "accepted": true,
     "message": "ACEITO - Estado final",
     "steps": 47,
     "finalTape": "^xxx___...",
     "history": [...]
   }
   ```

**Uso**: Execução via API web sem visualização física.

---

### `handleExecuteDisplay()`

**Localização**: Linhas 1857-1913

**Endpoint**: `POST /api/execute-display`

**Body**: Idêntico a `/api/execute`

**O que faz**:

Idêntico a `handleExecute()`, **MAS**:

```cpp
ExecutionResult result = tm.execute(input, configStr, true, 500);
```

- `useDisplay = true` (mostra no OLED)
- `stepDelay = 500` (delay de 500ms entre passos)

**Uso**: Execução via web **COM** visualização física no display.

---

### `handleStartStepMode()`

**Localização**: Linhas 1915-1978

**Endpoint**: `POST /api/start-step-mode`

**Body esperado**:
```json
{
  "input": "101",
  "config": { ... }
}
```

**O que faz**:

**MODO HÍBRIDO**: Configura via web, controla via botões físicos.

1. **Parseia requisição**
2. **Armazena em variáveis globais**:
   ```cpp
   entradaUsuario = input;
   serializeJson(config, configAtual);
   deserializeJson(docGlobal, configAtual);
   estadoInicial = config["initialState"];
   ```
3. **Responde imediatamente**:
   ```json
   {
     "status": "step_mode_started",
     "message": "Modo passo a passo iniciado. Use os botões físicos."
   }
   ```
4. **Inicia modo passo a passo**:
   ```cpp
   iniciarExecucaoPasso();
   ```

**Fluxo após essa chamada**:
- Usuário vê display do ESP32 mostrando fita
- Usuário pressiona SELECT para executar cada passo
- Controle **100% pelos botões físicos**

---

### `handleOptions()`

**Localização**: Linhas 1980-1983

**Endpoint**: `OPTIONS` (qualquer rota)

**O que faz**:

Responde a requisições preflight do CORS:
```cpp
addCORSHeaders();
server.send(204);
```

**Por que**: Navegadores fazem requisição OPTIONS antes de POST/DELETE para verificar permissões CORS.

---

### `configurarServidor()`

**Localização**: Linhas 1985-2023

**O que faz**:

Registra **todas as rotas** do servidor web:

```cpp
// Arquivos estáticos
server.on("/", HTTP_GET, handleRoot);
server.on("/styles.css", HTTP_GET, handleCSS);
server.on("/script.js", HTTP_GET, handleJS);

// API
server.on("/status", HTTP_GET, handleStatus);
server.on("/api/save", HTTP_POST, handleSave);
server.on("/api/load", HTTP_GET, handleLoad);
server.on("/api/files", HTTP_GET, handleListFiles);
server.on("/api/delete", HTTP_DELETE, handleDelete);
server.on("/api/execute", HTTP_POST, handleExecute);
server.on("/api/execute-display", HTTP_POST, handleExecuteDisplay);
server.on("/api/start-step-mode", HTTP_POST, handleStartStepMode);

// CORS preflight
server.on("/api/save", HTTP_OPTIONS, handleOptions);
server.on("/api/execute", HTTP_OPTIONS, handleOptions);
server.on("/api/execute-display", HTTP_OPTIONS, handleOptions);
server.on("/api/start-step-mode", HTTP_OPTIONS, handleOptions);

// 404
server.onNotFound(handleNotFound);

// Iniciar servidor
server.begin();
```

---

## Fluxos de Execução Completos

Agora vamos mapear **como as funções são chamadas** em cada cenário de uso.

---

## FLUXO 1: Menu Físico - Execução Automática

**Objetivo**: Executar MT com delay automático entre passos, visualizando no OLED.

```
┌─────────────────────────────────────────────────────────────────┐
│                    INICIALIZAÇÃO (acontece 1x)                   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
                         setup() [2025]
                              │
        ┌─────────────────────┼─────────────────────┐
        ▼                     ▼                     ▼
inicializarDisplay()   inicializarBotoes()  inicializarLittleFS()
     [665]                 [705]                [1546]
        │                     │                     │
        └─────────────────────┼─────────────────────┘
                              ▼
                    conectarWiFi() [1567]
                              │
                       ┌──────┴──────┐
                  Falhou?        Conectou?
                       │              │
                       ▼              ▼
                  iniciarAP()    (continua)
                    [1590]
                       │              │
                       └──────┬───────┘
                              ▼
                  configurarServidor() [1985]
                              │
                              ▼
                    carregarListaArquivos() [1196]
                              │
                              ▼
                    desenharMenuPrincipal() [1305]
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    LOOP PRINCIPAL (infinito)                     │
└─────────────────────────────────────────────────────────────────┘
                              │
                         loop() [2102]
                              │
                              ▼
                      server.handleClient()
                              │
                              ▼
                      dnsServer.processNextRequest()
                              │
                              ▼
                       displayAtivo?
                              │
                           Sim│
                              ▼
                      processarBotoes() [2132]
                              │
                              ▼
                  switch(estadoAtual) {
                    case MENU_PRINCIPAL:
                      processarMenuPrincipal()
                    case MENU_SELECAO_MT:
                      processarMenuSelecaoMT()
                    case MENU_EDICAO_ENTRADA:
                      processarEditorFita()
                    case EXECUTANDO_AUTO:
                      executarPassoAutomatico()
                    case EXECUTANDO_PASSO:
                      processarExecucaoPasso()
                    case RESULTADO_FINAL:
                      processarResultadoFinal()
                  }
                              │
                              ▼
                         delay(50)
                              │
                    (volta para loop())

┌─────────────────────────────────────────────────────────────────┐
│           FLUXO ESPECÍFICO: Selecionou "Executar (AUTO)"        │
└─────────────────────────────────────────────────────────────────┘
                              │
            Usuario navega menu principal
                              │
                              ▼
                  processarMenuPrincipal(botao=SELECT) [2169]
                              │
                    (opcaoSelecionada == 0)
                              │
                              ▼
                     estadoAtual = MENU_SELECAO_MT
                              │
                              ▼
                   carregarListaArquivos() [1196]
                              │
                              ▼
                   desenharMenuSelecaoMT() [1349]
                              │
            ┌─────────────────┴─────────────────┐
            │  Usuario navega entre arquivos    │
            │  pressiona PREV/NEXT              │
            └─────────────────┬─────────────────┘
                              ▼
            processarMenuSelecaoMT(botao=SELECT) [2223]
                              │
                              ▼
               carregarConfiguracao(arquivo) [1229]
                              │
                    ┌─────────┴─────────┐
                Sucesso?            Falha?
                    │                   │
                    ▼                   ▼
        iniciarExecucaoAutomatica()  (mostra erro)
                 [2377]
                    │
        ┌───────────┴───────────┐
        │  PREPARAÇÃO           │
        ├───────────────────────┤
        │ 1. fitaGlobal = "^" + entrada + "___..."
        │ 2. posicaoGlobal = 0
        │ 3. estadoAtualGlobal = estadoInicial
        │ 4. passoGlobal = 0
        │ 5. estadoAtual = EXECUTANDO_AUTO
        └───────────┬───────────┘
                    ▼
              drawTape(...) [727]
                    │
                    ▼
┌─────────────────────────────────────────────────────────────────┐
│            LOOP DE EXECUÇÃO AUTOMÁTICA (a cada 50ms)            │
└─────────────────────────────────────────────────────────────────┘
                    │
               loop() [2102]
                    │
                    ▼
          processarBotoes() [2132]
                    │
         (estadoAtual == EXECUTANDO_AUTO)
                    │
                    ▼
          executarPassoAutomatico() [2458]
                    │
        ┌───────────┴───────────┐
        │  LÓGICA DO PASSO      │
        ├───────────────────────┤
        │ 1. simboloAtual = fitaGlobal[posicaoGlobal]
        │ 2. trans = buscarTransicao(estadoAtualGlobal, simboloAtual)
        │                 │
        │                 ▼
        │           buscarTransicao(estado, simbolo) [510]
        │                 │
        │                 ▼
        │           buscarTransicao(estado, simbolo, docGlobal) [464]
        │                 │
        │           ┌─────┴─────┐
        │       Encontrou?   Não encontrou?
        │           │               │
        │           ▼               ▼
        │      return {          return {
        │        found:true,       found:false
        │        nextState,        ...
        │        newSymbol,      }
        │        direction       │
        │      }                 │
        │           │            │
        │ ◄─────────┴────────────┘
        │           │
        │     trans.found?
        │           │
        │      Não  │  Sim
        │      ┌────┴────┐
        │      │         │
        │      ▼         ▼
        │  mostrarResultadoFinal(   Salvar estado antigo
        │   "REJEITADO",            │
        │   "Sem transição",        ▼
        │   passoGlobal)      aplicarTransicao(trans) [582]
        │      │                    │
        │      ▼                    ▼
        │  estadoAtual =       aplicarTransicao(trans, fitaGlobal,
        │  RESULTADO_FINAL         posicaoGlobal, estadoAtualGlobal) [550]
        │      │                    │
        │      ▼              ┌─────┴─────┐
        │  return false       │ 1. fitaGlobal[pos] = trans.newSymbol[0]
        │                     │ 2. estadoAtualGlobal = trans.nextState
        │                     │ 3. if (direction == "L") pos--
        │                     │    if (direction == "R") pos++
        │                     │ 4. return nova_posicao
        │                     └─────┬─────┘
        │                           │
        │      ┌────────────────────┘
        │      ▼
        │  posicaoGlobal = nova_posicao
        │      │
        │      ▼
        │  animateTransition(fitaAntiga, fitaGlobal, ...) [810]
        │      │
        │      ▼
        │  passoGlobal++
        │      │
        │      ▼
        │  isEstadoFinal(estadoAtualGlobal) [517]
        │      │
        │      ▼
        │  isEstadoFinal(estadoAtualGlobal, docGlobal) [530]
        │      │
        │  ┌───┴───┐
        │ Sim?   Não?
        │  │       │
        │  ▼       ▼
        │ mostrarResultadoFinal(  passoGlobal >= 1000?
        │  "ACEITO",                    │
        │  "Estado final",          ┌───┴───┐
        │  passoGlobal)           Sim?   Não?
        │  │                        │       │
        │  ▼                        ▼       ▼
        │ estadoAtual =    mostrarResultadoFinal(  delay(500)
        │ RESULTADO_FINAL   "REJEITADO",           │
        │  │                "Limite",               ▼
        │  ▼                passoGlobal)      return true
        │ return false            │               │
        │                         ▼               │
        │                    return false         │
        └─────────────────────────────────────────┘
                                  │
                   ┌──────────────┴──────────────┐
              return false?              return true?
                   │                          │
                   ▼                          ▼
         (execução terminou)        (continua executando)
                   │                          │
                   └──────────────┬───────────┘
                                  │
                             (volta para loop)

┌─────────────────────────────────────────────────────────────────┐
│                    TELA FINAL (após execução)                    │
└─────────────────────────────────────────────────────────────────┘
                              │
                    mostrarResultadoFinal(
                      "ACEITO" ou "REJEITADO",
                      motivo,
                      passoGlobal,
                      waitForButton=true  ◄─── Menu físico
                    ) [1005]
                              │
                    ┌─────────┴─────────┐
                    │  EXIBIR TELA      │
                    ├───────────────────┤
                    │  ┌──────────────┐ │
                    │  │   ACEITO     │ │
                    │  │              │ │
                    │  │Estado final  │ │
                    │  │Passos: 47    │ │
                    │  │              │ │
                    │  │[Botão]       │ │
                    │  └──────────────┘ │
                    └─────────┬─────────┘
                              ▼
                      while(waitForButton) {
                        botao = lerBotao()
                        if (botao != 0) break
                      }
                              │
                              ▼
                    estadoAtual = RESULTADO_FINAL
                              │
                    (loop aguarda qualquer botão)
                              │
                              ▼
              processarResultadoFinal(botao) [2647]
                              │
                    (qualquer botão pressionado)
                              │
                              ▼
                    estadoAtual = MENU_PRINCIPAL
                              │
                              ▼
                  desenharMenuPrincipal() [1305]
                              │
                    (volta ao início)
```

---

## FLUXO 2: Menu Físico - Execução Passo a Passo

**Objetivo**: Controle total do usuário via botões, executando um passo por vez.

```
┌─────────────────────────────────────────────────────────────────┐
│     INÍCIO (usuário seleciona "Executar (PASSO)" no menu)       │
└─────────────────────────────────────────────────────────────────┘
                              │
                (mesma navegação inicial do FLUXO 1)
                              │
            carregarConfiguracao(arquivo) [1229]
                              │
                              ▼
              iniciarExecucaoPasso() [2515]
                              │
        ┌───────────┴───────────┐
        │  PREPARAÇÃO           │
        ├───────────────────────┤
        │ 1. fitaGlobal = "^" + entrada + "___..."
        │ 2. posicaoGlobal = 0
        │ 3. estadoAtualGlobal = estadoInicial
        │ 4. passoGlobal = 0
        │ 5. estadoAtual = EXECUTANDO_PASSO  ◄─── Diferença!
        └───────────┬───────────┘
                    ▼
              drawTape(...) [727]
                    │
                    ▼
              displayMessage(
                "Modo Passo a Passo",
                "BACK=Menu\nSELECT=Passo"
              ) [976]

┌─────────────────────────────────────────────────────────────────┐
│               LOOP AGUARDANDO BOTÕES (a cada 50ms)              │
└─────────────────────────────────────────────────────────────────┘
                    │
               loop() [2102]
                    │
                    ▼
          processarBotoes() [2132]
                    │
         (estadoAtual == EXECUTANDO_PASSO)
                    │
                    ▼
          processarExecucaoPasso(botao) [2563]
                    │
          ┌─────────┴─────────┐
          │  Qual botão?      │
          ├───────────────────┤
      PREV│        SELECT│       NEXT│
          ▼           ▼           ▼
    Voltar ao   executarProximoPasso()  (nada)
      menu           [2572]
      │               │
      ▼               ▼
 estadoAtual=  ┌─────────────────┐
MENU_PRINCIPAL │ EXECUTA 1 PASSO │  ◄─── Diferença do FLUXO 1
      │        └─────────────────┘
      │               │
      │               ▼
      │    (MESMA LÓGICA de executarPassoAutomatico)
      │               │
      │        ┌──────┴──────┐
      │        │ buscarTransicao()
      │        │ aplicarTransicao()
      │        │ isEstadoFinal()
      │        │ animateTransition()
      │        └──────┬──────┘
      │               │
      │           Terminou?
      │          (estado final OU
      │           sem transição)
      │               │
      │            Sim│  Não│
      │               ▼     ▼
      │    mostrarResultadoFinal(  drawTape(...)
      │     ...,                        │
      │     waitForButton=true)         ▼
      │               │          (aguarda próximo SELECT)
      │               ▼
      │    estadoAtual=RESULTADO_FINAL
      │               │
      └───────────────┴───────────────►│
                                       │
                              (volta para loop)
```

**Diferença chave**:
- ❌ Não há delay automático
- ❌ Não há loop contínuo
- ✅ Cada passo só executa quando usuário pressiona SELECT
- ✅ Permite análise detalhada de cada transição

---

## FLUXO 3: API Web - Execute (JSON)

**Objetivo**: Executar MT via requisição HTTP, retornar resultado em JSON.

```
┌─────────────────────────────────────────────────────────────────┐
│          REQUISIÇÃO HTTP (do navegador ou curl)                 │
└─────────────────────────────────────────────────────────────────┘
                              │
   POST http://192.168.0.111/api/execute
   Content-Type: application/json
   {
     "input": "101",
     "config": { ... }
   }
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    PROCESSAMENTO NO ESP32                        │
└─────────────────────────────────────────────────────────────────┘
                              │
                         loop() [2102]
                              │
                              ▼
                    server.handleClient()
                              │
                   (rota /api/execute encontrada)
                              │
                              ▼
                   handleExecute() [1787]
                              │
        ┌─────────────────────┴─────────────────────┐
        │  1. Ler body da requisição                │
        │  2. deserializeJson(requestDoc, body)     │
        │  3. Extrair input e config                │
        │  4. Serializar config para string         │
        └─────────────────────┬─────────────────────┘
                              ▼
              TuringMachine tm;  // Criar instância
                              │
                              ▼
              result = tm.execute(
                input,
                configStr,
                useDisplay = false,  ◄─── SEM visualização OLED
                stepDelay = 0
              ) [224]
                              │
┌─────────────────────────────────────────────────────────────────┐
│          EXECUÇÃO DENTRO DA CLASSE TuringMachine                │
└─────────────────────────────────────────────────────────────────┘
                              │
        TuringMachine::execute(...) [224]
                              │
        ┌─────────────────────┴─────────────────────┐
        │  1. deserializeJson(doc, configJSON)      │
        │  2. tape = "^" + input + "___..."         │
        │  3. headPosition = 0                      │
        │  4. currentState = initialState           │
        │  5. stepCount = 0                         │
        │  6. result.history = "["                  │
        └─────────────────────┬─────────────────────┘
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    LOOP DE EXECUÇÃO                              │
└─────────────────────────────────────────────────────────────────┘
              while (stepCount < MAX_STEPS)
                              │
                              ▼
             currentSymbol = tape[headPosition]
                              │
                              ▼
         transition = buscarTransicao(
           currentState,
           currentSymbol,
           doc.as<JsonObject>()  ◄─── Usa JSON da requisição
         ) [464]
                              │
                    ┌─────────┴─────────┐
                Não encontrou?      Encontrou?
                    │                   │
                    ▼                   ▼
           result.message =      newPosition = aplicarTransicao(
             "Sem transição"        transition,
           result.accepted=false    tape,
           return result            headPosition,
                    │               currentState
                    │             ) [550]
                    │                   │
                    │                   ▼
                    │             stepCount++
                    │                   │
                    │                   ▼
                    │             Adicionar ao history:
                    │             {
                    │               "step": n,
                    │               "state": "q0",
                    │               "position": 1,
                    │               "symbol": "1",
                    │               "tape": "^101___"
                    │             }
                    │                   │
                    │                   ▼
                    │             isEstadoFinal(
                    │               currentState,
                    │               doc.as<JsonObject>()
                    │             ) [530]
                    │                   │
                    │              ┌────┴────┐
                    │          É final?   Não é?
                    │              │          │
                    │              ▼          ▼
                    │      result.accepted=true  (continua loop)
                    │      result.message="ACEITO"
                    │      return result
                    │              │
                    └──────────────┴──────────────┐
                                                  │
                                                  ▼
                                    result.finalTape = tape
                                    result.steps = stepCount
                                    result.history += "]"
                                                  │
                              ◄───────────────────┘
                              │
                    (retorna para handleExecute)

┌─────────────────────────────────────────────────────────────────┐
│                     RESPOSTA HTTP (JSON)                         │
└─────────────────────────────────────────────────────────────────┘
                              │
                   handleExecute() [1787]
                              │
        ┌─────────────────────┴─────────────────────┐
        │  Construir JSON de resposta:              │
        │  {                                         │
        │    "accepted": result.accepted,            │
        │    "message": result.message,              │
        │    "steps": result.steps,                  │
        │    "finalTape": result.finalTape,          │
        │    "history": result.history               │
        │  }                                         │
        └─────────────────────┬─────────────────────┘
                              ▼
              server.send(200, "application/json", jsonResponse)
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                   RESPOSTA RECEBIDA NO NAVEGADOR                │
└─────────────────────────────────────────────────────────────────┘
                              │
        script.js: executeOnServer() [linha ~900]
                              │
                    ┌─────────┴─────────┐
                    │ Exibir resultado: │
                    ├───────────────────┤
                    │ ✓ ACEITO          │
                    │ Estado final      │
                    │ Passos: 47        │
                    │ Fita: ^xxx___     │
                    │                   │
                    │ [Histórico...]    │
                    └───────────────────┘
```

**Características**:
- ✅ Execução completa **sem visualização OLED**
- ✅ Retorna histórico completo de passos em JSON
- ✅ Não bloqueia o servidor (execução rápida)
- ✅ Usa **mesmas funções helper** que menu físico

---

## FLUXO 4: API Web - Execute Display

**Objetivo**: Executar MT via requisição HTTP **COM** visualização no display OLED.

```
┌─────────────────────────────────────────────────────────────────┐
│          REQUISIÇÃO HTTP (do navegador)                         │
└─────────────────────────────────────────────────────────────────┘
                              │
   POST http://192.168.0.111/api/execute-display
   Content-Type: application/json
   {
     "input": "101",
     "config": { ... }
   }
                              │
                              ▼
                   handleExecuteDisplay() [1857]
                              │
        ┌─────────────────────┴─────────────────────┐
        │  (mesma preparação de handleExecute)      │
        └─────────────────────┬─────────────────────┘
                              ▼
              result = tm.execute(
                input,
                configStr,
                useDisplay = true,   ◄─── COM visualização OLED
                stepDelay = 500      ◄─── Delay de 500ms
              ) [224]
                              │
┌─────────────────────────────────────────────────────────────────┐
│          EXECUÇÃO COM DISPLAY (dentro de execute)               │
└─────────────────────────────────────────────────────────────────┘
                              │
              if (useDisplay && displayAtivo)
                              │
                              ▼
              displayMessage("Executando", "Iniciando...") [976]
                              │
                              ▼
              drawTape(tape, headPosition, ...) [727]
                              │
                              ▼
              delay(1000)  // Pausa inicial
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    LOOP COM VISUALIZAÇÃO                         │
└─────────────────────────────────────────────────────────────────┘
            while (stepCount < MAX_STEPS)
                              │
                    ┌─────────┴─────────┐
                    │ (mesma lógica de  │
                    │  buscar/aplicar)  │
                    └─────────┬─────────┘
                              ▼
              if (useDisplay && displayAtivo)
                              │
                              ▼
              animateTransition(
                oldTape, newTape,
                oldPos, newPos,
                oldState, newState,
                stepCount
              ) [810]
                              │
        ┌─────────────────────┴─────────────────────┐
        │  Animação em 3 frames:                    │
        │  1. Estado antes (200ms)                  │
        │  2. Pisca célula modificada (200ms)       │
        │  3. Estado depois (200ms)                 │
        └─────────────────────┬─────────────────────┘
                              ▼
                        delay(stepDelay)  // 500ms
                              │
                    (continua próximo passo)
                              │
                        ┌─────┴─────┐
                   Terminou?      Continua?
                        │             │
                        ▼             │
              mostrarResultadoFinal(  │
                titulo,               │
                motivo,               │
                stepCount,            │
                waitForButton=false ◄─┘  API mode
              ) [1005]
                        │
        ┌───────────────┴───────────────┐
        │  Exibe tela final:            │
        │  ┌──────────────┐             │
        │  │   ACEITO     │             │
        │  │              │             │
        │  │Estado final  │             │
        │  │Passos: 47    │             │
        │  └──────────────┘             │
        │                               │
        │  delay(3000)  // Sem aguardar │
        └───────────────┬───────────────┘
                        │
              (retorna result para handleExecuteDisplay)
                        │
                        ▼
              server.send(200, "application/json", ...)
                        │
                        ▼
            (resposta recebida no navegador)
```

**Diferença do FLUXO 3**:
- ✅ **Mostra cada passo no display OLED**
- ✅ **Animações** entre transições
- ✅ **Delay de 500ms** entre passos
- ✅ **Tela final idêntica** ao menu físico (mas sem aguardar botão)
- ⚠️ **Bloqueia servidor** durante execução (~500ms × número de passos)

---

## FLUXO 5: API Web - Start Step Mode (Híbrido)

**Objetivo**: Configurar MT pela web, mas controlar execução fisicamente.

```
┌─────────────────────────────────────────────────────────────────┐
│     PARTE 1: REQUISIÇÃO HTTP (configuração via web)             │
└─────────────────────────────────────────────────────────────────┘
                              │
   POST http://192.168.0.111/api/start-step-mode
   Content-Type: application/json
   {
     "input": "101",
     "config": { ... }
   }
                              │
                              ▼
                handleStartStepMode() [1915]
                              │
        ┌─────────────────────┴─────────────────────┐
        │  1. deserializeJson(requestDoc, body)     │
        │  2. input = requestDoc["input"]           │
        │  3. config = requestDoc["config"]         │
        └─────────────────────┬─────────────────────┘
                              ▼
        ┌─────────────────────────────────────────────┐
        │  ARMAZENAR EM VARIÁVEIS GLOBAIS:            │
        ├─────────────────────────────────────────────┤
        │  entradaUsuario = input                     │
        │  serializeJson(config, configAtual)         │
        │  deserializeJson(docGlobal, configAtual)    │
        │  estadoInicial = config["initialState"]     │
        └─────────────────────┬─────────────────────┘
                              ▼
        ┌─────────────────────────────────────────────┐
        │  RESPONDER IMEDIATAMENTE:                   │
        │  {                                           │
        │    "status": "step_mode_started",            │
        │    "message": "Use os botões físicos."       │
        │  }                                           │
        └─────────────────────┬─────────────────────┘
                              ▼
          server.send(200, "application/json", ...)
                              │
                    ┌─────────┴─────────┐
                    │  Requisição HTTP  │
                    │   FINALIZADA      │
                    │                   │
                    │  (navegador       │
                    │   recebe resposta)│
                    └─────────┬─────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│     PARTE 2: INICIAR MODO PASSO NO ESP32 (após resposta)       │
└─────────────────────────────────────────────────────────────────┘
                              │
                handleStartStepMode() [1915]
           (continuação após server.send)
                              │
                              ▼
                  iniciarExecucaoPasso() [2515]
                              │
        ┌─────────────────────┴─────────────────────┐
        │  (MESMA LÓGICA do FLUXO 2)                │
        │  1. Preparar fita, estado, posição        │
        │  2. estadoAtual = EXECUTANDO_PASSO        │
        │  3. Mostrar fita no display               │
        │  4. Aguardar botões                       │
        └─────────────────────┬─────────────────────┘
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│            PARTE 3: CONTROLE VIA BOTÕES FÍSICOS                 │
│            (idêntico ao FLUXO 2 daqui em diante)                │
└─────────────────────────────────────────────────────────────────┘
                              │
                         loop() [2102]
                              │
                              ▼
                    processarBotoes() [2132]
                              │
                   (estadoAtual == EXECUTANDO_PASSO)
                              │
                              ▼
                 processarExecucaoPasso(botao) [2563]
                              │
                    ┌─────────┴─────────┐
                PREV│       SELECT│        NEXT│
                    ▼           ▼            ▼
              Volta ao    executarProximoPasso()  (nada)
               menu            [2572]
                    │           │
                    │           ▼
                    │    (executa 1 passo usando funções helper)
                    │           │
                    │           ▼
                    │    buscarTransicao() [510] → [464]
                    │    aplicarTransicao() [582] → [550]
                    │    isEstadoFinal() [517] → [530]
                    │    animateTransition() [810]
                    │           │
                    │      ┌────┴────┐
                    │  Terminou?  Continua?
                    │      │          │
                    │      ▼          ▼
                    │ mostrarResultadoFinal(  drawTape(...)
                    │  ...,                        │
                    │  waitForButton=true)         ▼
                    │      │              (aguarda SELECT)
                    │      ▼
                    │ estadoAtual=RESULTADO_FINAL
                    │      │
                    └──────┴──────────────────────────┐
                                                      │
                                            (volta para loop)
```

**Características únicas**:
- ✅ **Configuração via web** (interface gráfica)
- ✅ **Controle via botões físicos** (passo a passo)
- ✅ **Resposta HTTP imediata** (não bloqueia navegador)
- ✅ **Melhor de 2 mundos**: Facilidade de configurar + controle detalhado

---

## Resumo das Funções Helper Centralizadas

Todos os 5 fluxos usam **exatamente as mesmas 3 funções**:

### `buscarTransicao()`
- **Entrada**: Estado atual + Símbolo atual + Config JSON
- **Saída**: TransitionInfo (nextState, newSymbol, direction)
- **Usada por**:
  - Menu físico automático → versão global
  - Menu físico passo → versão global
  - API execute → versão com JsonObject
  - API execute-display → versão com JsonObject
  - API start-step-mode → versão global (após iniciar)

### `aplicarTransicao()`
- **Entrada**: TransitionInfo + Fita + Posição + Estado
- **Saída**: Nova posição da cabeça
- **Modifica**: Fita (escreve símbolo) + Estado (atualiza)
- **Usada por**: Todos os 5 fluxos

### `isEstadoFinal()`
- **Entrada**: Estado atual + Config JSON
- **Saída**: true/false
- **Usada por**: Todos os 5 fluxos para decidir aceitação

---

## Tabela Comparativa dos Fluxos

| Aspecto | Menu Auto | Menu Passo | API Execute | API Display | API Step Mode |
|---------|-----------|------------|-------------|-------------|---------------|
| **Configuração** | Arquivo LittleFS | Arquivo LittleFS | JSON da requisição | JSON da requisição | JSON da requisição |
| **Controle** | Automático | Manual (botões) | Automático | Automático | Manual (botões) |
| **Visualização OLED** | ✅ Sim | ✅ Sim | ❌ Não | ✅ Sim | ✅ Sim |
| **Delay entre passos** | 500ms | N/A (manual) | 0ms | 500ms | N/A (manual) |
| **Animações** | ✅ Sim | ✅ Sim | ❌ Não | ✅ Sim | ✅ Sim |
| **Tela final** | `waitForButton=true` | `waitForButton=true` | (não usa) | `waitForButton=false` | `waitForButton=true` |
| **Retorna JSON** | ❌ Não | ❌ Não | ✅ Sim | ✅ Sim | ✅ Sim (status) |
| **Bloqueia servidor** | ✅ Sim | ✅ Sim | ❌ Não | ✅ Sim (~500ms×passos) | ❌ Não |
| **Funções helper** | buscar/aplicar/isFinal (global) | buscar/aplicar/isFinal (global) | buscar/aplicar/isFinal (JsonObject) | buscar/aplicar/isFinal (JsonObject) | buscar/aplicar/isFinal (global após init) |

---

## Variáveis Globais Importantes

### Estado do Sistema
```cpp
enum Estado {
  MENU_PRINCIPAL,
  MENU_SELECAO_MT,
  MENU_EDICAO_ENTRADA,
  EXECUTANDO_AUTO,
  EXECUTANDO_PASSO,
  RESULTADO_FINAL
};
Estado estadoAtual = MENU_PRINCIPAL;
```

### Configuração da MT (Menu Físico)
```cpp
StaticJsonDocument<8192> docGlobal;  // Config JSON completa
String estadoInicial;                 // Ex: "q0"
String entradaUsuario;                // Ex: "101"
String configAtual;                   // JSON serializado
```

### Estado de Execução (Menu Físico)
```cpp
String fitaGlobal;           // Ex: "^101___..."
int posicaoGlobal;           // Ex: 2
String estadoAtualGlobal;    // Ex: "q1"
int passoGlobal;             // Ex: 15
```

### Menu e Navegação
```cpp
int opcaoSelecionada = 0;           // Opção destacada no menu
String arquivosMT[10];              // Lista de arquivos .json
int totalArquivosMT = 0;            // Quantidade de arquivos
int arquivoSelecionado = 0;         // Índice do arquivo destacado
```

### Editor de Fita
```cpp
String fitaEditor;          // String sendo editada
int cursorEditor;           // Posição do cursor
```

---

## Convenções de Nomenclatura

### Funções
- **Verbos no infinitivo**: `buscarTransicao()`, `aplicarTransicao()`, `desenharMenu()`
- **Prefixos**:
  - `inicializar`: Setup de componentes
  - `desenhar`: Renderização no display
  - `processar`: Lógica de botões
  - `executar`: Lógica da MT
  - `handle`: Handlers HTTP
  - `carregar`: Leitura de arquivos

### Variáveis
- **camelCase**: `estadoAtual`, `fitaGlobal`, `arquivoSelecionado`
- **Sufixo `Global`**: Indica variáveis compartilhadas entre menu físico
- **UPPER_CASE**: Constantes (`MAX_STEPS`, `OLED_WIDTH`, `BTN_PREV`)

---

## Conclusão

Este mapa documenta **todas as funções** e **todos os fluxos de execução** do projeto ESP32 Turing Machine.

**Princípios arquiteturais**:

1. **Centralização**: Lógica da MT em 3 funções reutilizadas por todos os modos
2. **Consistência**: Comportamento idêntico independente do modo de execução
3. **Flexibilidade**: Overloading permite código simples (global) e explícito (parâmetros)
4. **Didática**: Código comentado, estruturado e com mensagens de debug

**Para navegar o código**:
1. Leia `novo_projeto.ino` linhas 0-61 (índice)
2. Consulte este MAPA para entender cada função
3. Use diagramas de fluxo para seguir execução passo a passo
4. Teste diferentes modos e compare comportamento

**Para modificar o código**:
1. **Nunca** altere as funções helper sem testar **todos** os 5 fluxos
2. Mantenha overloading (versões global + JsonObject)
3. Use `Serial.println()` para debug em cada etapa
4. Documente mudanças neste arquivo

---

**Última atualização**: 2025-01-09
**Autor**: Márcio
**Projeto**: TCC - Ciência da Computação
