# ESP32 Turing Machine

## Visão Geral

Implementação de uma **Máquina de Turing** em ESP32, com interface web e controle físico via display OLED e botões. Desenvolvido como Trabalho de Conclusão de Curso (TCC) em Ciência da Computação.

### Características

- **Interface Web** - Configure e execute MTs pelo navegador
- **Display OLED** - Visualização da execução com animações
- **3 Modos de Execução**:
  - Automático (com delay entre passos)
  - Passo a passo (controle via botões)
  - Híbrido (configura na web, executa fisicamente)
- **Persistência** - Salve e carregue MTs no sistema de arquivos LittleFS
- **API REST** - Controle via requisições HTTP

---

## Estrutura do Projeto

```
turing_esp32/
├── turing_esp32.ino             # Código principal ESP32
├── data/                        # Arquivos para upload no LittleFS
│   ├── index.html               # Interface web
│   ├── styles.css               # Estilos CSS
│   ├── script.js                # Lógica JavaScript
│   └── exemplo_palindromo.json  # MT de exemplo
└── README.md                    # Este arquivo
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
GPIO 12  -->   [BACK] --> GND
GPIO 14  -->   [SELECT] --> GND
GPIO 2   -->   [NEXT] --> GND
```

### Software

**Arduino IDE 2.3.6**

1. **ESP32 Core**: File → Preferences → Additional Board Manager URLs:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```

2. **Bibliotecas** (via Library Manager):
   - `ArduinoJson` v7.4.2
   - `Adafruit GFX` 1.12.4
   - `Adafruit SSD1306` 2.5.15

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
2. Configure WiFi (linhas 46-47):
   ```cpp
   const char* WIFI_SSID = "SEU_SSID";
   const char* WIFI_PASSWORD = "SUA_SENHA";
   ```
3. Faça upload

### 2. Upload dos Arquivos Web

1. Tools → ESP32 Sketch Data Upload
2. Aguarde conclusão

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

**Fluxo**:

1. **Configurar Alfabetos**:
   - Alfabeto de entrada (ex: `01`)
   - Alfabeto auxiliar (ex: `xy`)
   - Clique **Gerar Tabela de Transições**

2. **Adicionar Estados**:
   - **+ Estado Normal** ou **+ Estado Final**

3. **Preencher Transições**:
   - Tabela com: Próximo Estado | Novo Símbolo | Direção (E/D)
   - Estados finais não aparecem na tabela

4. **Executar**:
   - Digite entrada (ex: `101`)
   - **Executar no Servidor**: Resultado JSON
   - **Executar no Display OLED**: Animação no display físico
   - **Iniciar Modo Passo a Passo**: Controle pelos botões

5. **Salvar/Carregar**:
   - Salvar no ESP32 (LittleFS)
   - Download/Upload JSON
   - Copiar para clipboard

### Menu Físico (Display OLED)

```
[BACK]    [SELECT]    [NEXT]
  ◄          ✓          ►
```

**Menu Principal**:
1. Modo Automático - Execução com delay 500ms
2. Modo Passo-a-Passo - Um passo por botão
3. Selecionar MT - Escolher arquivo salvo

**Editor de Fita**:
```
[<<] [X] [>] [1] [0] [1] [+]
```
- `<<`: Voltar
- `X`: Limpar fita
- `>`: Iniciar execução
- Símbolos: Editar (SELECT alterna)
- `+`: Adicionar símbolo

**Durante Execução**:
- Exibe: estado atual, fita, próxima transição
- Formato: `trans: q1 | 0 | D`
- Resultado: ACEITO/REJEITADO com passos

---

## API REST

| Método | Endpoint | Descrição |
|--------|----------|-----------|
| GET | `/` | Interface web |
| GET | `/status` | Status do ESP32 |
| POST | `/api/save` | Salvar MT |
| GET | `/api/load?filename=X` | Carregar MT |
| GET | `/api/files` | Listar arquivos |
| DELETE | `/api/delete?filename=X` | Deletar arquivo |
| POST | `/api/execute` | Executar MT |
| POST | `/api/execute-display` | Executar com display |
| POST | `/api/start-step-mode` | Modo passo a passo |

### Exemplo

```bash
curl -X POST http://192.168.0.111/api/execute \
  -H "Content-Type: application/json" \
  -d '{
    "input": "101",
    "config": {
      "alphabet": ["0","1"],
      "tapeAlphabet": ["0","1","^","_"],
      "states": ["q0","q_accept"],
      "initialState": "q0",
      "finalStates": ["q_accept"],
      "transitions": {
        "q0": {
          "0": {"nextState":"q0","newSymbol":"0","direction":"R"},
          "1": {"nextState":"q_accept","newSymbol":"1","direction":"R"}
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
  "finalTape": "^101___...",
  "history": [...]
}
```

---

## Arquitetura

### Funções Centralizadas

O código usa funções helper compartilhadas por todos os modos:

```cpp
TransitionInfo buscarTransicao(String state, char symbol);
int aplicarTransicao(TransitionInfo trans, String &tape, int &pos, String &state);
bool isEstadoFinal(String state);
```

### Fluxos de Execução

1. **Menu Automático**: `iniciarExecucaoAutomatica()` → `executarPassoAutomatico()`
2. **Menu Passo a Passo**: `iniciarExecucaoPasso()` → `executarProximoPasso()`
3. **API Web**: `TuringMachine::execute()`

Todos usam as mesmas funções, garantindo comportamento consistente.

---

## Formato JSON da MT

```json
{
  "nome": "Nome da MT",
  "descricao": "Descrição opcional",
  "alphabet": ["0", "1"],
  "tapeAlphabet": ["0", "1", "x", "^", "_"],
  "states": ["q0", "q1", "q_accept"],
  "initialState": "q0",
  "finalStates": ["q_accept"],
  "transitions": {
    "q0": {
      "^": {"nextState": "q0", "newSymbol": "^", "direction": "R"},
      "0": {"nextState": "q1", "newSymbol": "x", "direction": "R"}
    },
    "q1": {
      "1": {"nextState": "q_accept", "newSymbol": "1", "direction": "R"}
    }
  }
}
```

### Campos

- `alphabet`: Símbolos de entrada (usuário pode digitar)
- `tapeAlphabet`: Todos os símbolos (inclui `^` início e `_` vazio)
- `states`: Lista de todos os estados
- `initialState`: Estado inicial
- `finalStates`: Estados de aceitação
- `transitions`: Tabela de transições por estado e símbolo
- `direction`: `"L"` (esquerda) ou `"R"` (direita)

---

## Troubleshooting

### Display não liga
- Verifique conexões SDA/SCL
- Confirme endereço I2C (0x3C)

### WiFi não conecta
- Verifique SSID/senha no código
- Modo AP ativa automaticamente como fallback

### Página não carrega
- Refaça upload LittleFS
- Use `http://` (não `https://`)

### MT rejeita incorretamente
- Verifique se todas as transições estão preenchidas
- Estados finais devem estar marcados
- Use API `/api/execute` para ver histórico de passos

---

## Recursos

- [Turing Machine - Wikipedia](https://en.wikipedia.org/wiki/Turing_machine)
- [ESP32 Docs](https://docs.espressif.com/projects/arduino-esp32/)
- [ArduinoJson](https://arduinojson.org/)
- [Adafruit SSD1306](https://learn.adafruit.com/monochrome-oled-breakouts/)

---

## Licença

Projeto educacional - TCC em Ciência da Computação.

**Autor**: Marcio Lima
**Ano**: 2025
