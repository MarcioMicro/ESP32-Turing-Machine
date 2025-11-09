# 🤖 ESP32 Turing Machine

## Visão Geral

Implementação completa de uma **Máquina de Turing Universal** em ESP32, com interface web moderna e controle físico via display OLED e botões. Desenvolvido como Trabalho de Conclusão de Curso (TCC) com foco em código didático e arquitetura limpa.

### Características Principais

- ✅ **Interface Web Completa** - Configure e execute MTs graficamente
- ✅ **Display OLED** - Visualização física da execução passo a passo
- ✅ **3 Modos de Execução**:
  - Automático (via menu físico ou API)
  - Passo a passo (controle total via botões)
  - Híbrido (configura na web, executa fisicamente)
- ✅ **Arquitetura Unificada** - Funções centralizadas garantem consistência
- ✅ **Persistência** - Salve e carregue MTs no sistema de arquivos LittleFS
- ✅ **API REST** - Controle total via HTTP

---

## Estrutura do Projeto

```
novo_projeto/
├── novo_projeto.ino             # Código principal ESP32 (~2500 linhas)
├── data/                        # Arquivos para LittleFS (upload separado)
│   ├── index.html               # Interface web responsiva
│   ├── styles.css               # Estilos CSS modernos
│   ├── script.js                # Lógica JavaScript (~1000 linhas)
│   ├── config.json              # Configurações do sistema
│   └── exemplo_palindromo.json  # MT de exemplo (palíndromo binário)
├── README.md                    # Este arquivo
├── MAPA_FUNCOES.md             # Documentação detalhada de funções
└── CLAUDE.md                    # Informações para assistente IA
```

---

## Requisitos

### Hardware

| Componente | Quantidade | Especificação |
|------------|------------|---------------|
| ESP32 DevKit | 1x | Qualquer modelo com WiFi |
| Display OLED | 1x | SSD1306 128x64 I2C (0x3C) |
| Push Buttons | 3x | Momentary switch |
| Resistores | 3x | 10kΩ (opcional, se usar pull-up interno) |
| Breadboard | 1x | 830 pontos |
| Jumpers | ~15 | Macho-macho |
| Cabo USB | 1x | Micro-USB para programação |

### Conexões

```
ESP32          OLED SSD1306
GPIO 5   <-->  SDA
GPIO 4   <-->  SCL
3.3V     <-->  VCC
GND      <-->  GND

ESP32          Botões (pull-up interno habilitado)
GPIO 12  <-->  [BACK] --> GND
GPIO 14  <-->  [SELECT] --> GND
GPIO 2   <-->  [NEXT] --> GND
```

### Software

#### Arduino IDE 2.x

1. **Instalar ESP32 Core**:
   - File → Preferences → Additional Board Manager URLs:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Tools → Board → Boards Manager → Buscar "ESP32" → Instalar

2. **Bibliotecas Necessárias** (via Library Manager):
   - `ArduinoJson` (by Benoit Blanchon) v6.x
   - `Adafruit GFX Library`
   - `Adafruit SSD1306`
   - **IMPORTANTE**: Usar `WebServer.h` (nativo do ESP32 - não instalar)

3. **Plugin LittleFS**:
   - Download: [ESP32 LittleFS Plugin](https://github.com/lorol/arduino-esp32littlefs-plugin/releases)
   - Extrair em: `<ArduinoDir>/tools/`
   - Reiniciar IDE

#### Configurações da Placa

```
Tools → Board: "ESP32 Dev Module"
Tools → Partition Scheme: "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)"
Tools → Upload Speed: "115200"
Tools → Port: [Selecionar porta COM do ESP32]
```

---

## Instalação

### 1. Upload do Firmware

1. Abra `novo_projeto.ino` no Arduino IDE
2. Configure WiFi (opcional):
   ```cpp
   // Linha ~40 do código
   #define WIFI_SSID "SEU_SSID"
   #define WIFI_PASSWORD "SUA_SENHA"
   ```
3. Compile e faça upload: **Upload** (→)

### 2. Upload dos Arquivos Web

1. Certifique-se que a pasta `data/` está no mesmo diretório do `.ino`
2. No Arduino IDE: **Tools → ESP32 Sketch Data Upload**
3. Aguarde conclusão (~30 segundos)

### 3. Primeiro Acesso

Após upload completo, o ESP32 irá:

1. **Tentar conectar ao WiFi** configurado
   - Se conectar: Exibe IP no Serial Monitor
   - Acesse: `http://[IP_DO_ESP32]`

2. **Se falhar, inicia modo AP** (Access Point):
   - SSID: `ESP32_TuringMachine`
   - Senha: `123`
   - IP fixo: `http://192.168.4.1`

3. **Display OLED** mostra menu principal

---

## Uso

### Interface Web

Acesse via navegador: `http://[IP_DO_ESP32]`

#### Fluxo Típico

1. **Configurar Alfabetos**:
   - Nome da máquina (opcional)
   - Descrição (opcional, colapsável)
   - Alfabeto de entrada: ex `01`
   - Alfabeto auxiliar: ex `xy`
   - Clique **Gerar Tabela de Transições**

2. **Adicionar Estados**:
   - Clique **+ Estado Normal** ou **+ Estado Final**
   - Estados aparecem em cards coloridos

3. **Preencher Transições**:
   - Tabela gerada automaticamente
   - Selects para: Próximo Estado | Novo Símbolo | Direção
   - Estados finais **não aparecem** na tabela (por design)

4. **Executar**:
   - Digite entrada na fita: ex `101`
   - **▶️ Executar no Servidor**: Executa e mostra resultado JSON
   - **📺 Executar no Display OLED**: Executa com animação no display físico
   - **🔄 Iniciar Modo Passo a Passo**: Inicia no ESP32, controle pelos botões

5. **Salvar/Carregar**:
   - Nome do arquivo: ex `minha_mt`
   - **💾 Salvar no ESP32**: Grava no LittleFS
   - **Arquivos Salvos**: Lista com botões Carregar/Deletar
   - **📥 Download JSON**: Baixa para seu computador
   - **📋 Copiar JSON**: Copia para clipboard

### Menu Físico (Display OLED)

Navegação pelos botões:

```
[BACK]    [SELECT]    [NEXT]
  ◄          ✓          ►
```

**Menu Principal**:
1. **Executar (AUTO)** → Execução automática com delay 500ms
2. **Executar (PASSO)** → Controle passo a passo
3. **Editar Entrada** → Modifica string de entrada
4. **Sair** → Desliga display (economiza energia)

**Durante Execução**:
- Display mostra: Estado atual, Fita, Transição
- Formato: `trans: q1 | 0 | D` (estado | símbolo | direção)
- Tela final: ACEITO/REJEITADO com motivo e passos

---

## API REST

### Endpoints Disponíveis

| Método | Endpoint | Descrição |
|--------|----------|-----------|
| GET | `/` | Interface web principal |
| GET | `/status` | Status do ESP32 (JSON) |
| POST | `/api/save` | Salvar MT no LittleFS |
| GET | `/api/load?filename=X` | Carregar MT |
| GET | `/api/files` | Listar arquivos salvos |
| DELETE | `/api/delete?filename=X` | Deletar arquivo |
| POST | `/api/execute` | Executar MT (retorna resultado) |
| POST | `/api/execute-display` | Executar com visualização OLED |
| POST | `/api/start-step-mode` | Iniciar modo passo a passo |

### Exemplos de Uso

#### Executar MT

```bash
curl -X POST http://192.168.0.111/api/execute \
  -H "Content-Type: application/json" \
  -d '{
    "input": "101",
    "config": {
      "nome": "Teste",
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

#### Resposta

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

Todo o código compartilha **funções helper** centralizadas:

```cpp
// Núcleo da lógica da MT
TransitionInfo buscarTransicao(String state, char symbol, JsonObject config);
int aplicarTransicao(TransitionInfo trans, String &tape, int &pos, String &state);
bool isEstadoFinal(String state, JsonObject config);
```

### Fluxos de Execução

Todos os 3 modos usam as mesmas funções:

1. **Menu Físico Automático**: `iniciarExecucaoAutomatica()` → `executarPassoAutomatico()`
2. **Menu Físico Passo a Passo**: `iniciarExecucaoPasso()` → `executarProximoPasso()`
3. **API Web**: `TuringMachine::execute()` (classe)

**Resultado**: Comportamento **idêntico** e **previsível** em todos os modos.

Consulte [MAPA_FUNCOES.md](MAPA_FUNCOES.md) para documentação detalhada.

---

## Exemplos de MTs

### Palíndromo Binário (incluído)

Arquivo: `data/exemplo_palindromo.json`

- **Aceita**: `101`, `0110`, `1111`, `0000`, `1`
- **Rejeita**: `100`, `011`, `1010`, `001`

### Como Criar Novas MTs

1. Use a interface web (mais fácil)
2. Ou edite JSON manualmente:

```json
{
  "nome": "Minha MT",
  "descricao": "O que ela faz",
  "alphabet": ["a","b"],
  "tapeAlphabet": ["a","b","x","^","_"],
  "states": ["q0","q1","q_accept"],
  "initialState": "q0",
  "finalStates": ["q_accept"],
  "transitions": {
    "q0": {
      "a": {
        "nextState": "q1",
        "newSymbol": "x",
        "direction": "R"
      }
    }
  }
}
```

3. Faça upload via web ou copie para `data/` e refaça upload LittleFS

---

## Troubleshooting

### Display OLED não liga

1. Verifique conexões (SDA/SCL)
2. Teste endereço I2C (geralmente `0x3C`):
   ```cpp
   // Use I2C Scanner sketch
   ```
3. Confirme biblioteca Adafruit SSD1306 instalada

### WiFi não conecta

1. Verifique SSID/senha no código
2. Serial Monitor mostra tentativas
3. Se falhar, ativa modo AP automaticamente

### Página web não carrega

1. Ping no IP: `ping 192.168.0.111`
2. Verifique firewall
3. Certifique-se de usar `http://` (não `https://`)
4. Refaça upload LittleFS se necessário

### Erro 404 em endpoints da API

1. Código foi atualizado? Refaça upload do firmware
2. Verifique Serial Monitor durante boot
3. Rotas são case-sensitive

### MT rejeita quando deveria aceitar

1. Verifique se transições estão completas (sem campos vazios)
2. Estados finais devem estar marcados
3. Teste via API para ver histórico de passos
4. Consulte [MAPA_FUNCOES.md](MAPA_FUNCOES.md) para lógica de aceitação

---

## Documentação Adicional

- **[MAPA_FUNCOES.md](MAPA_FUNCOES.md)**: Explicação didática de cada função e fluxos
- **[CLAUDE.md](CLAUDE.md)**: Informações do projeto para assistente IA
- **[PLANO_IMPLEMENTACAO.md](../PLANO_IMPLEMENTACAO.md)**: Guia de desenvolvimento incremental (raiz do projeto)

---

## Recursos de Aprendizado

### Teoria de Computação

- [Turing Machine - Wikipedia](https://en.wikipedia.org/wiki/Turing_machine)
- [Introduction to Automata Theory (Hopcroft)](https://www.amazon.com/Introduction-Automata-Theory-Languages-Computation/dp/0321455363)

### ESP32

- [ESP32 Official Docs](https://docs.espressif.com/projects/arduino-esp32/)
- [Random Nerd Tutorials - ESP32](https://randomnerdtutorials.com/projects-esp32/)

### Bibliotecas

- [ArduinoJson Documentation](https://arduinojson.org/v6/doc/)
- [Adafruit SSD1306 Guide](https://learn.adafruit.com/monochrome-oled-breakouts/arduino-library-and-examples)

---

## Licença

Projeto desenvolvido para fins educacionais como parte do Trabalho de Conclusão de Curso (TCC) em Ciência da Computação.

**Autor**: Márcio
**Instituição**: [Sua Universidade]
**Ano**: 2024/2025

---

## Agradecimentos

- Professores orientadores
- Comunidade ESP32
- Adafruit por bibliotecas de qualidade
- Claude Code por assistência no desenvolvimento

---

**🎓 Objetivo**: Demonstrar implementação prática de conceitos teóricos de Ciência da Computação em hardware embarcado, com código limpo, didático e bem documentado.

**📚 Para estudantes**: Este projeto serve como referência de boas práticas em:
- Arquitetura de firmware embarcado
- Design de APIs REST
- Interfaces web responsivas
- Documentação técnica completa
