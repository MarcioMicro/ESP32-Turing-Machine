# ESP32 Turing Machine

## Visão Geral

Implementação de uma **Máquina de Turing** em ESP32, com interface web completa e controle físico via display OLED e botões. Desenvolvido como Trabalho de Conclusão de Curso (TCC) em Ciência da Computação.

### Características

- **Interface Web Completa** - Configure, execute e visualize MTs pelo navegador
- **Display OLED** - Visualização da execução com animações e fita final
- **5 Modos de Execução**:
  - API web (rápido, sem display)
  - API web com display (animações)
  - Automático local (OLED, delay entre passos)
  - Passo a passo local (OLED, controle via botões)
  - Passo a passo remoto (API, controle via web)
- **Persistência** - Salve e carregue MTs no LittleFS
- **Upload JSON** - Importe configurações do computador
- **API REST** - Controle total via requisições HTTP
- **Histórico Detalhado** - Visualização passo a passo com transições

---

## Estrutura do Projeto

```
turing_esp32/
├── turing_esp32.ino             # Código principal ESP32
├── data/                        # Arquivos para upload no LittleFS
│   ├── index.html               # Interface web
│   ├── styles.css               # Estilos CSS
│   ├── script.js                # Lógica JavaScript
│   ├── apostila_1.json          # Exemplo: exercício apostila
│   └── exemplo_palindromo.json  # Exemplo: verificador de palíndromo
├── README.md                    # Este arquivo

```

---

## Requisitos

### Hardware

| Componente | Quantidade | Especificação |
|------------|------------|---------------|
| ESP32 DevKit | 1x | WEMOS LOLIN32 (30 pinos) |
| Display OLED | 1x | SSD1306 128x64 I2C (Integrado ao ESP32) |
| Push Buttons | 3x | Botão momentâneo |
| Protoboard | 1x | 830 pontos |
| Jumpers | ~15 | Macho-macho |
| Cabo USB | 1x | Para programação |

### Conexões

```
ESP32          OLED SSD1306
GPIO 5   -->   SDA
GPIO 4   -->   SCL
3.3V     -->   VCC
GND      -->   GND

ESP32          Botões
GPIO 12  -->   [PREV] --> GND
GPIO 14  -->   [SELECT] --> GND
GPIO 2   -->   [NEXT] --> GND
```

### Software

**Arduino IDE 2.3.6+**

1. **ESP32 Core**: File → Preferences → Additional Board Manager URLs:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```

2. **Bibliotecas** (via Library Manager):
   - `ArduinoJson` v7.4.2+
   - `Adafruit GFX` 1.12.4+
   - `Adafruit SSD1306` 2.5.15+

3. **Plugin LittleFS**: [ESP32 LittleFS Plugin](https://github.com/lorol/arduino-esp32littlefs-plugin/releases)

**Configurações da Placa**:
```
Board: ESP32 WEMOS LOLIN32
Partition Scheme: Default 4MB with spiffs
Upload Speed: 115200
```

---

## Instalação

### 1. Upload do Firmware

1. Abra `turing_esp32.ino` no Arduino IDE
2. Configure WiFi nas linhas indicadas:
   ```cpp
   const char* WIFI_SSID = "SEU_SSID";
   const char* WIFI_PASSWORD = "SUA_SENHA";
   ```
3. Selecione a placa e porta
4. Faça upload do sketch

### 2. Upload dos Arquivos Web

1. Tools → ESP32 Sketch Data Upload
2. Aguarde conclusão (~10-30 segundos)

### 3. Primeiro Acesso

O ESP32 tentará conectar ao WiFi configurado:
- **Sucesso**: IP exibido no display e Serial Monitor
- **Falha**: Ativa modo AP (SSID: `ESP32_TuringMachine`, Senha: `123`, IP: `192.168.4.1`)

O display mostra:
```
Pronto!
Acesse pelo ip:
192.168.0.XXX
Pressione botao...
```

Pressione qualquer botão para ir ao menu principal.

---

## Uso

### Interface Web

Acesse `http://[IP_DO_ESP32]` no navegador.

**Fluxo de Configuração**:

1. **Configurar Alfabetos**:
   - Nome da máquina (opcional)
   - Descrição (opcional)
   - Alfabeto de entrada (ex: `01`)
   - Alfabeto auxiliar (ex: `xy`)
   - Clique **Gerar Tabela de Transições**

2. **Adicionar Estados**:
   - **+ Estado Normal** (q1, q2, ...)
   - **+ Estado Final** (estado de aceitação)
   - **Deletar Último** (remove último estado adicionado)

3. **Preencher Transições**:
   - Tabela com dropdowns: Próximo Estado | Novo Símbolo | Direção
   - **Direção**: E (← esquerda) ou D (→ direita)
   - Estados finais não aparecem na tabela (não precisam de transições)

4. **Visualizar Tabela**:
   - Matriz completa de transições
   - Formato: `(próx_estado, símbolo, direção)`

5. **Executar**:
   - Digite entrada (ex: `101`)
   - **▶️ Executar no Servidor**: Resultado detalhado com histórico
   - **📺 Executar no Display OLED**: Animação no display físico
   - **🔄 Iniciar Modo Passo a Passo**: Controle pelos botões físicos

6. **Gerenciar Configurações**:
   - **💾 Salvar no ESP32**: Persiste no LittleFS
   - **📂 Carregar do ESP32**: Lista arquivos salvos
   - **📋 Copiar JSON**: Copia para clipboard
   - **📤 Upload JSON**: Importa arquivo JSON do computador

### Histórico de Execução (Interface Web)

O resultado mostra cada passo com detalhes:

```
[Passo 0] Estado Inicial
  Estado: q0 | Posição: 0 | Símbolo lido: ^
  Fita: ^101____________________
        ↑

[Passo 1] Transição: (q0, ^) → (q0, ^, D (→))
  Estado: q0 | Posição: 1 | Símbolo lido: 1
  Fita: ^101____________________
         ↑

[Passo 2] Transição: (q0, 1) → (q1, x, D (→))
  Estado: q1 | Posição: 2 | Símbolo lido: 0
  Fita: ^x01____________________
          ↑

[Passo 5] ✓ ESTADO FINAL ATINGIDO
  Estado: q_accept (FINAL)
  Fita: ^xxx____________________
```

### Menu Físico (Display OLED)

```
[PREV]    [SELECT]    [NEXT]
  ◄          ✓           ►
```

**Menu Principal**:
1. Modo Automático - Execução com delay 500ms
2. Modo Passo-a-Passo - Um passo por botão SELECT
3. Selecionar MT - Escolher arquivo salvo no LittleFS

**Editor de Fita**:
```
[<<] [X] [>] [1] [0] [1] [+]
```
- `<<`: Voltar ao menu
- `X`: Limpar fita
- `>`: Iniciar execução
- Símbolos: SELECT alterna valores
- `+`: Adicionar novo símbolo

**Durante Execução**:
- Exibe: estado atual, fita com seta, próxima transição
- Formato: `trans: q1 | x | D`
- Animação mostra escrita, movimento e próxima transição
- Resultado final:
  ```
    ACEITO

    Estado final
    Passos: 7
    Fita: ^xxx_______
  ```

---

## API REST

### Endpoints Disponíveis

| Método | Endpoint | Descrição |
|--------|----------|-----------|
| GET | `/` | Interface web principal |
| GET | `/status` | Status do ESP32 (WiFi, memória, uptime) |
| POST | `/api/save` | Salvar MT no LittleFS |
| GET | `/api/load?filename=X` | Carregar MT do LittleFS |
| GET | `/api/files` | Listar arquivos salvos |
| DELETE | `/api/delete?filename=X` | Deletar arquivo |
| POST | `/api/execute` | Executar MT (sem display) |
| POST | `/api/execute-display` | Executar MT com animações OLED |
| POST | `/api/start-step-mode` | Iniciar modo passo a passo |

### Exemplo: Executar MT

```bash
curl -X POST http://192.168.0.111/api/execute \
  -H "Content-Type: application/json" \
  -d '{
    "input": "101",
    "config": {
      "alphabet": ["0","1"],
      "tapeAlphabet": ["0","1","x","^","_"],
      "states": ["q0","q_accept"],
      "initialState": "q0",
      "finalStates": ["q_accept"],
      "transitions": {
        "q0": {
          "^": {"nextState":"q0","newSymbol":"^","direction":"D"},
          "0": {"nextState":"q0","newSymbol":"0","direction":"D"},
          "1": {"nextState":"q_accept","newSymbol":"1","direction":"D"}
        }
      }
    }
  }'
```

**Resposta**:
```json
{
  "accepted": true,
  "message": "ACEITO - Estado final",
  "steps": 3,
  "finalTape": "^101____________________",
  "history": [
    {
      "step": 0,
      "state": "q0",
      "position": 0,
      "symbol": "^",
      "tape": "^101____________________"
    },
    {
      "step": 1,
      "state": "q0",
      "position": 1,
      "symbol": "1",
      "tape": "^101____________________"
    },
    {
      "step": 2,
      "state": "q_accept",
      "position": 2,
      "symbol": "0",
      "tape": "^101____________________"
    }
  ]
}
```

---

## Arquitetura

### Arquitetura Unificada

O código utiliza **métodos centralizados** compartilhados por todos os 5 modos de execução:

```cpp
// Método unificado de inicialização
void inicializar(String input);

// Método unificado de execução de passo
StepResult executarPassoUnico(bool useDisplay = false);
```

### Struct StepResult

```cpp
struct StepResult {
  bool canContinue;        // true = pode continuar executando
  bool reachedFinalState;  // true = atingiu estado final
  String errorMessage;     // mensagem de erro (se houver)
};
```

### Fluxos de Execução

Todos os modos usam os mesmos métodos centralizados:

1. **API Web (rápido)**: `execute(false)` → loop de `executarPassoUnico(false)`
2. **API Web (display)**: `execute(true)` → loop de `executarPassoUnico(true)`
3. **Menu Automático**: `iniciarExecucaoAutomatica()` → loop de `executarPassoUnico(true)`
4. **Menu Passo a Passo**: `iniciarExecucaoPasso()` → `executarProximoPasso()` → `executarPassoUnico(true)`
5. **API Passo a Passo**: API inicia, botão SELECT chama `executarProximoPasso()`

### Benefícios da Arquitetura

✅ **Consistência**: Mesmo comportamento em todos os modos
✅ **Manutenibilidade**: Código organizado em funções centralizadas
✅ **Simplicidade**: Arquitetura enxuta e eficiente
✅ **Testabilidade**: Lógica centralizada facilita validação

---

## Formato JSON da MT

### Estrutura Completa

```json
{
  "nome": "Verificador de Palíndromo Binário",
  "descricao": "Verifica se uma string binária é um palíndromo",
  "alphabet": ["0", "1"],
  "tapeAlphabet": ["0", "1", "x", "^", "_"],
  "states": ["q0", "q1", "q2", "q_accept"],
  "initialState": "q0",
  "finalStates": ["q_accept"],
  "transitions": {
    "q0": {
      "^": {"nextState": "q1", "newSymbol": "^", "direction": "D"},
      "0": {"nextState": "q0", "newSymbol": "0", "direction": "E"}
    },
    "q1": {
      "0": {"nextState": "q2", "newSymbol": "x", "direction": "D"},
      "1": {"nextState": "q2", "newSymbol": "x", "direction": "D"},
      "_": {"nextState": "q_accept", "newSymbol": "_", "direction": "D"}
    }
  }
}
```

### Campos Obrigatórios

- `alphabet` *(array)*: Símbolos de entrada válidos
- `tapeAlphabet` *(array)*: Todos os símbolos (inclui `^` início e `_` vazio)
- `states` *(array)*: Lista de todos os estados
- `initialState` *(string)*: Estado inicial (normalmente "q0")
- `finalStates` *(array)*: Estados de aceitação
- `transitions` *(object)*: Tabela de transições por estado e símbolo

### Campos Opcionais

- `nome` *(string)*: Nome descritivo da MT
- `descricao` *(string)*: Descrição do que a MT faz
- `exemplos` *(object)*: Exemplos de entrada aceitas/rejeitadas (não usado pelo sistema)
- `notas` *(array)*: Notas de implementação (não usado pelo sistema)

### Formato de Transição

```json
{
  "nextState": "q1",      // Próximo estado
  "newSymbol": "x",       // Símbolo a escrever na fita
  "direction": "D"        // Direção: "E" (esquerda) ou "D" (direita)
}
```

**Importante**: Use `"E"` para esquerda (←) e `"D"` para direita (→). Não use `"L"` ou `"R"`.

---

## Funcionalidades Especiais

### Contagem de Passos

A máquina conta **todos os movimentos**, incluindo:
- Transições bem-sucedidas
- Tentativas de transição que falham (erro conta como passo)
- Movimentos que levam a estado final

### Animação OLED

Quando executando com display, cada passo mostra:
1. **Fase 1**: Símbolo sendo escrito (fade out/in)
2. **Fase 2**: Cabeça se movendo para nova posição
3. **Fase 3**: Próxima transição disponível

Formato da informação de transição:
```
trans: q2 | x | D
       ↑    ↑   ↑
     estado novo direção
            símbolo
```

Se não houver transição: `trans: -`

### Truncamento de Fita

- **Interface web**: Mostra fita completa no histórico
- **OLED**: Trunca para 18 caracteres (`^cada________...`)
- Fita cresce automaticamente quando cabeça ultrapassa limite

---

## Troubleshooting

### Display não liga
- Verifique conexões SDA (GPIO 5) e SCL (GPIO 4)
- Confirme endereço I2C com scanner (normalmente 0x3C)
- Verifique alimentação 3.3V

### WiFi não conecta
- Verifique SSID/senha no código
- Modo AP ativa automaticamente como fallback
- SSID do AP: `ESP32_TuringMachine`, senha: `123`

### Página não carrega
- Refaça upload LittleFS (Tools → ESP32 Sketch Data Upload)
- Use `http://` (não `https://`)
- Limpe cache do navegador
- Verifique Serial Monitor para possíveis erros

### MT rejeita incorretamente
- Verifique se todas as transições necessárias estão preenchidas
- Estados finais devem estar marcados em `finalStates`
- Use API `/api/execute` para ver histórico detalhado
- Verifique direção das transições (E ou D)

### Botões não respondem
- Verifique conexões com GND
- Use resistores pull-up internos (configurado no código)
- Teste com Serial Monitor (mensagens de debug)

### Upload LittleFS falha
- Feche Serial Monitor antes do upload
- Use cabo USB de dados (não apenas carregamento)
- Tente velocidade menor (115200)

### Caracteres corruptos no OLED
- Evite caracteres acentuados em nomes de estados
- Use apenas ASCII padrão (a-z, 0-9, _, -, ^)
- Biblioteca Adafruit_GFX não suporta UTF-8

---

## Exemplos Incluídos

### apostila_1.json
Exercício 1 da apostila de compiladores (Página 63).
- **Entrada**: strings no formato `a^n b^(n+1)`
- **Exemplo aceito**: `abb`, `aabbbb`
- **Exemplo rejeitado**: `ab`, `aabbb`

### exemplo_palindromo.json
Verificador de palíndromo binário.
- **Entrada**: strings binárias (0 e 1)
- **Aceita**: `101`, `0110`, `1111`, `0000`
- **Rejeita**: `100`, `011`, `1010`

---

## Recursos

### Documentação Oficial
- [Turing Machine - Wikipedia](https://en.wikipedia.org/wiki/Turing_machine)
- [ESP32 Arduino Core](https://docs.espressif.com/projects/arduino-esp32/)
- [ArduinoJson Documentation](https://arduinojson.org/)
- [Adafruit SSD1306 Guide](https://learn.adafruit.com/monochrome-oled-breakouts/)

### Ferramentas Úteis
- [JSON Validator](https://jsonlint.com/) - Validar arquivos JSON
- [I2C Scanner](https://playground.arduino.cc/Main/I2cScanner/) - Encontrar endereço I2C
- [ESP32 Pinout Reference](https://randomnerdtutorials.com/esp32-pinout-reference-gpios/)

---

## Desenvolvimento

### Estrutura do Código

```cpp
// Estruturas de dados
struct TransitionInfo { ... };
struct ExecutionResult { ... };
struct StepResult { ... };

// Classe principal
class TuringMachine {
  String tape;
  int headPosition;
  String currentState;
  int stepCount;

  void inicializar(String input);
  StepResult executarPassoUnico(bool useDisplay);
  ExecutionResult execute(String input, bool useDisplay);
};

// Funções auxiliares
TransitionInfo buscarTransicao(String state, char symbol);
int aplicarTransicao(TransitionInfo trans, ...);
bool isEstadoFinal(String state);
```

### Extensibilidade

A arquitetura permite extensões através de:
1. `executarPassoUnico()` - lógica de execução central
2. `inicializar()` - configuração inicial da máquina
3. `setupServer()` - registro de endpoints da API
4. `script.js` - interface web interativa

---

## Versão

**Versão atual**: 2.0 (2025)

### Funcionalidades

- ✅ Interface web completa e responsiva
- ✅ Upload de arquivos JSON do computador
- ✅ Histórico detalhado com transições visualizadas
- ✅ Indicador visual de posição da cabeça (seta ↑)
- ✅ Exibição da fita final no display OLED
- ✅ Sistema de direção em português (E/D)
- ✅ Contagem precisa de passos incluindo erros
- ✅ Arquitetura unificada e centralizada
- ✅ 5 modos completos de execução
- ✅ Persistência de configurações em LittleFS
- ✅ API REST completa

---

## Licença

Projeto educacional desenvolvido como Trabalho de Conclusão de Curso (TCC) em Ciência da Computação.

**Autor**: Marcio Lima
**Instituição**: URI Erechim
**Ano**: 2025
**Orientador**: Fábio Zanin


