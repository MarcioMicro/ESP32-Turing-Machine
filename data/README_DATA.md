# Pasta data/ - Arquivos para LittleFS

## 📁 Sobre Esta Pasta

Esta pasta contém **todos os arquivos que serão enviados para o ESP32** através do sistema de arquivos LittleFS.

### O que é LittleFS?

LittleFS (Little File System) é um sistema de arquivos otimizado para dispositivos embarcados com memória flash limitada. Ele substitui o antigo SPIFFS.

## 📄 Arquivos Nesta Pasta

### Arquivos de Interface Web
Estes arquivos são servidos pelo servidor web do ESP32:

- **index.html** (6.4 KB)
  - Interface principal da aplicação
  - HTML5 semântico e responsivo
  - Estruturado em seções (cards)

- **styles.css** (11 KB)
  - Estilos modernos com CSS3
  - Design responsivo
  - Variáveis CSS para fácil customização
  - Tema claro/profissional

- **script.js** (16 KB)
  - Lógica da interface
  - Comunicação com API do ESP32 (fetch)
  - Gerenciamento de estado da MT
  - Funções parcialmente implementadas (será completado nas etapas)

### Arquivos de Configuração

- **config.json** (206 bytes)
  - Configurações de rede WiFi
  - SSID e senha para modo Station
  - Configurações do Access Point
  - Hostname para mDNS

- **exemplo_palindromo.json** (2.7 KB)
  - Exemplo funcional de Máquina de Turing
  - Verifica palíndromos binários
  - Inclui notas e exemplos de teste
  - Use como referência para criar suas próprias MTs

## 🚀 Como Usar

### Durante Desenvolvimento

#### Opção 1: Testar Interface Localmente (Recomendado)

Você pode abrir `index.html` diretamente no navegador usando Live Server:

1. **VS Code**: Instale extensão "Live Server"
2. Clique direito em `index.html` → "Open with Live Server"
3. Interface abrirá em `http://localhost:5500`

**Vantagens:**
- ✅ Desenvolvimento rápido
- ✅ Hot reload automático
- ✅ Debug no navegador (F12)

**Limitações:**
- ❌ APIs do ESP32 não funcionarão (erro de fetch)
- ❌ Apenas teste visual da interface

#### Opção 2: Após Upload no ESP32

Quando estiver nas **ETAPAS 5-6** do plano:

1. Arduino IDE → Tools → **ESP32 Sketch Data Upload**
2. Aguardar upload completar (~30 segundos)
3. Acessar `http://IP_DO_ESP32`

**Vantagens:**
- ✅ Teste completo (frontend + backend)
- ✅ APIs funcionando
- ✅ Ambiente real

**Desvantagem:**
- ❌ Mais lento (precisa fazer upload a cada mudança)

### Fluxo de Trabalho Recomendado

```
1. Desenvolver HTML/CSS/JS localmente
   ↓ (testar visual com Live Server)
2. Quando estiver satisfeito
   ↓
3. Fazer upload para ESP32
   ↓ (Tools → ESP32 Sketch Data Upload)
4. Testar integração completa
   ↓
5. Ajustar se necessário
   ↓
6. Repetir 1-5
```

## 📝 Estrutura dos Arquivos JSON

### config.json
```json
{
  "wifi": {
    "ssid": "SUA_REDE",           // Nome da rede WiFi
    "password": "SUA_SENHA"       // Senha da rede
  },
  "ap": {
    "ssid": "ESP32_TuringMachine", // Nome do AP criado pelo ESP32
    "password": "turingmachine"    // Senha do AP
  },
  "hostname": "turingmachine"      // Acesso via http://turingmachine.local
}
```

### Máquina de Turing (exemplo_palindromo.json)
```json
{
  "nome": "Nome descritivo",
  "alphabet": ["0", "1"],                    // Símbolos de entrada
  "tapeAlphabet": ["0", "1", "x", "^", "_"], // Todos os símbolos
  "states": ["q0", "q1", "q2"],              // Estados
  "initialState": "q0",                      // Sempre q0
  "finalStates": ["q_accept"],               // Estados finais
  "transitions": {
    "q0": {
      "^": {
        "nextState": "q1",
        "newSymbol": "^",
        "direction": "R"              // L=esquerda, R=direita, S=parado
      }
    }
  }
}
```

## 🛠️ Modificando os Arquivos

### HTML (index.html)
- Estrutura em `<section class="card">`
- IDs importantes:
  - `#alphabet` - Input do alfabeto
  - `#states-container` - Container dos estados
  - `#transition-table` - Tabela de transições
  - `#toast-container` - Notificações

### CSS (styles.css)
- Variáveis CSS em `:root` (fácil customização)
- Classes utilitárias: `.btn`, `.card`, `.input-group`
- Responsivo (breakpoint: 768px)

### JavaScript (script.js)
- Estado global em objeto `state`
- Funções de utilidade: `showToast()`, `isValidAlphabet()`
- Event listeners configurados no `DOMContentLoaded`
- APIs do ESP32 serão completadas nas ETAPAs finais

## ⚠️ IMPORTANTE

### NÃO modifique estes arquivos se:
- Você ainda não passou pelas ETAPAs 1-5 do plano
- Não entende a estrutura do projeto
- Está apenas explorando

### MODIFIQUE se:
- Está seguindo o plano de implementação
- Quer customizar a interface
- Está testando funcionalidades

### SEMPRE:
- Faça backup antes de modificar
- Teste localmente primeiro (Live Server)
- Depois faça upload para ESP32
- Commit suas mudanças (se usar Git)

## 🎓 Para o TCC

Estes arquivos são excelentes para:
- **Demonstração visual** - Interface profissional
- **Discussão de tecnologias** - HTML5, CSS3, JavaScript moderno
- **Arquitetura cliente-servidor** - Separação frontend/backend
- **UX/UI** - Design responsivo e acessível

## 📚 Recursos

### Testar Responsividade
- Abra DevTools (F12)
- Toggle Device Toolbar (Ctrl+Shift+M)
- Teste em diferentes tamanhos de tela

### Debug JavaScript
- Console do navegador (F12 → Console)
- `console.log()` já implementado em pontos-chave
- Network tab para ver requisições HTTP

### Validação HTML/CSS
- [W3C HTML Validator](https://validator.w3.org/)
- [W3C CSS Validator](https://jigsaw.w3.org/css-validator/)

---

**Arquivos prontos para desenvolvimento! 🚀**

*Lembre-se: Estes são os ÚNICOS arquivos web que importam. Qualquer arquivo HTML/CSS/JS fora desta pasta não será usado pelo ESP32.*
