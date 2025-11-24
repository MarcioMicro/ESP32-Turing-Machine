/**
 * ESP32 Turing Machine - JavaScript Frontend
 * Versão: 2.0 (Reconstrução)
 *
 * Este arquivo será construído incrementalmente conforme o PLANO_IMPLEMENTACAO.md
 * Por enquanto, contém apenas a estrutura básica e funções de utilidade.
 */

// ============================================================================
// Estado Global da Aplicação
// ============================================================================

const state = {
    turingMachine: {
        alphabet: [],
        tapeAlphabet: [],
        states: ['q0'],
        initialState: 'q0',
        finalStates: [],
        transitions: {}
    },
    stateCounter: 1,
    tableGenerated: false
};

// ============================================================================
// Utilitários
// ============================================================================

/**
 * Exibe notificação toast
 * @param {string} message - Mensagem a exibir
 * @param {string} type - Tipo: 'success', 'error', 'info'
 */
function showToast(message, type = 'info') {
    const container = document.getElementById('toast-container');

    const toast = document.createElement('div');
    toast.className = `toast ${type}`;

    const icon = {
        success: '✓',
        error: '✗',
        info: 'ℹ'
    }[type] || 'ℹ';

    toast.innerHTML = `
        <span style="font-size: 1.5rem; font-weight: bold;">${icon}</span>
        <span>${message}</span>
    `;

    container.appendChild(toast);

    // Auto-remover após 3 segundos
    setTimeout(() => {
        toast.style.animation = 'slideIn 0.3s ease-out reverse';
        setTimeout(() => toast.remove(), 300);
    }, 3000);
}

/**
 * Valida alfabeto (apenas caracteres alfanuméricos)
 * @param {string} alphabet - Alfabeto a validar
 * @returns {boolean}
 */
function isValidAlphabet(alphabet) {
    return /^[a-zA-Z0-9]+$/.test(alphabet);
}

/**
 * Atualiza status de conexão no footer
 * @param {boolean} connected - Se está conectado
 */
function updateConnectionStatus(connected) {
    const statusEl = document.getElementById('connection-status');
    if (connected) {
        statusEl.textContent = '🟢 Conectado';
        statusEl.style.color = 'var(--success-color)';
    } else {
        statusEl.textContent = '🔴 Desconectado';
        statusEl.style.color = 'var(--danger-color)';
    }
}

// ============================================================================
// Inicialização
// ============================================================================

document.addEventListener('DOMContentLoaded', function() {
    console.log('ESP32 Turing Machine - Frontend carregado');

    // Testar conexão com servidor
    testConnection();

    // Configurar event listeners
    setupEventListeners();

    // Carregar lista de arquivos salvos
    loadFilesList();

    showToast('Interface carregada! Configure os alfabetos para começar.', 'info');
});

/**
 * Testa conexão com o ESP32
 */
async function testConnection() {
    try {
        const response = await fetch('/status');
        if (response.ok) {
            const data = await response.json();
            updateConnectionStatus(true);
            console.log('Conectado ao ESP32:', data);
        }
    } catch (error) {
        updateConnectionStatus(false);
        console.log('Executando em modo local (sem ESP32)');
    }
}

/**
 * Configura todos os event listeners
 */
function setupEventListeners() {
    // Botão: Gerar Tabela
    document.getElementById('generate-table')?.addEventListener('click', generateTable);

    // Botão: Adicionar Estado Normal
    document.getElementById('add-state-btn')?.addEventListener('click', () => addState(false));

    // Botão: Adicionar Estado Final
    document.getElementById('add-final-state-btn')?.addEventListener('click', () => addState(true));

    // Botão: Deletar Último Estado
    document.getElementById('delete-last-state-btn')?.addEventListener('click', deleteLastState);

    // Botão: Salvar Máquina
    document.getElementById('save-machine-btn')?.addEventListener('click', saveMachine);

    // Botão: Carregar Máquina
    document.getElementById('load-machine-btn')?.addEventListener('click', loadMachine);

    // Botão: Copiar JSON
    document.getElementById('copy-json-btn')?.addEventListener('click', copyJSON);

    // Botão: Executar no Servidor
    document.getElementById('run-machine-btn')?.addEventListener('click', runMachine);

    // Botão: Executar no Display
    document.getElementById('run-display-btn')?.addEventListener('click', runOnDisplay);

    // Campo: Validação de entrada da fita
    const inputTapeField = document.getElementById('input-tape');
    if (inputTapeField) {
        inputTapeField.addEventListener('input', validateTapeInput);
    }
}

// ============================================================================
// Funções de Configuração de Alfabetos
// ============================================================================

/**
 * Toggle da área de descrição
 */
function toggleDescription() {
    const textarea = document.getElementById('machine-description');
    const button = event.target;

    if (textarea.style.display === 'none') {
        textarea.style.display = 'block';
        button.textContent = '▲ Ocultar';
    } else {
        textarea.style.display = 'none';
        button.textContent = '▼ Mostrar';
    }
}

/**
 * Gera a tabela de transições
 */
function generateTable() {
    const alphabetInput = document.getElementById('alphabet').value.trim();
    const tapeAlphabetInput = document.getElementById('tape-alphabet').value.trim();

    // Validações
    if (!alphabetInput) {
        showToast('Por favor, defina o alfabeto de entrada!', 'error');
        return;
    }

    if (!isValidAlphabet(alphabetInput)) {
        showToast('Alfabeto deve conter apenas caracteres alfanuméricos!', 'error');
        return;
    }

    if (tapeAlphabetInput && !isValidAlphabet(tapeAlphabetInput)) {
        showToast('Alfabeto auxiliar deve conter apenas caracteres alfanuméricos!', 'error');
        return;
    }

    // Capturar nome e descrição
    const machineName = document.getElementById('machine-name').value.trim();
    const machineDescription = document.getElementById('machine-description').value.trim();

    if (machineName) {
        state.turingMachine.nome = machineName;
    }

    if (machineDescription) {
        state.turingMachine.descricao = machineDescription;
    }

    // Processar alfabetos
    state.turingMachine.alphabet = [...new Set(alphabetInput.split(''))];

    const auxiliarySymbols = tapeAlphabetInput ? [...new Set(tapeAlphabetInput.split(''))] : [];
    state.turingMachine.tapeAlphabet = [
        ...state.turingMachine.alphabet,
        ...auxiliarySymbols,
        '^',
        '_'
    ];

    // Remover duplicatas
    state.turingMachine.tapeAlphabet = [...new Set(state.turingMachine.tapeAlphabet)];

    // Mostrar seções
    document.getElementById('states-section').style.display = 'block';
    document.getElementById('transitions-section').style.display = 'block';
    document.getElementById('visualization-section').style.display = 'block';
    document.getElementById('execution-section').style.display = 'block';

    // Gerar tabela de transições
    buildTransitionTable();

    state.tableGenerated = true;

    showToast('Tabela de transições gerada com sucesso!', 'success');
}

// ============================================================================
// Funções de Gerenciamento de Estados
// ============================================================================

/**
 * Adiciona novo estado
 * @param {boolean} isFinal - Se é estado final
 */
function addState(isFinal) {
    const stateName = `q${state.stateCounter++}`;

    state.turingMachine.states.push(stateName);

    if (isFinal) {
        state.turingMachine.finalStates.push(stateName);
    }

    // Adicionar ao DOM
    const statesContainer = document.getElementById('states-container');
    const stateDiv = document.createElement('div');
    stateDiv.className = `state-item ${isFinal ? 'state-final' : ''}`;
    stateDiv.dataset.state = stateName;

    stateDiv.innerHTML = `
        <span class="state-name">${stateName}</span>
        ${isFinal ? '<span class="state-label">(final)</span>' : ''}
    `;

    statesContainer.appendChild(stateDiv);

    // Atualizar displays
    updateStateDisplays();

    // Se tabela já foi gerada, adicionar linhas
    if (state.tableGenerated) {
        addStateToTable(stateName);
    }

    showToast(`Estado ${stateName} adicionado!`, 'success');
}

/**
 * Remove o último estado
 */
function deleteLastState() {
    if (state.turingMachine.states.length <= 1) {
        showToast('Não é possível remover o estado inicial q0!', 'error');
        return;
    }

    const lastState = state.turingMachine.states[state.turingMachine.states.length - 1];

    // Remover do array
    state.turingMachine.states = state.turingMachine.states.filter(s => s !== lastState);
    state.turingMachine.finalStates = state.turingMachine.finalStates.filter(s => s !== lastState);

    // Remover do DOM
    document.querySelector(`[data-state="${lastState}"]`)?.remove();

    // Remover da tabela de transições
    if (state.tableGenerated) {
        removeStateFromTable(lastState);
    }

    state.stateCounter--;

    updateStateDisplays();

    showToast(`Estado ${lastState} removido!`, 'success');
}

/**
 * Atualiza displays de estados
 */
function updateStateDisplays() {
    document.getElementById('initial-state-display').textContent = state.turingMachine.initialState;

    const finalStatesDisplay = document.getElementById('final-states-display');
    if (state.turingMachine.finalStates.length > 0) {
        finalStatesDisplay.textContent = state.turingMachine.finalStates.join(', ');
    } else {
        finalStatesDisplay.textContent = 'Nenhum';
    }
}

// ============================================================================
// Funções da Tabela de Transições
// ============================================================================

/**
 * Constrói a tabela de transições completa
 */
function buildTransitionTable() {
    const tbody = document.getElementById('transition-body');
    tbody.innerHTML = '';

    // Ordenar alfabeto da fita (^ sempre primeiro)
    const orderedAlphabet = [...state.turingMachine.tapeAlphabet];
    const caretIndex = orderedAlphabet.indexOf('^');
    if (caretIndex > -1) {
        orderedAlphabet.splice(caretIndex, 1);
        orderedAlphabet.unshift('^');
    }

    // Criar linhas para cada (estado, símbolo), excluindo estados finais
    state.turingMachine.states.forEach(stateName => {
        // Pular estados finais na tabela de transições
        if (state.turingMachine.finalStates.includes(stateName)) {
            return;
        }

        orderedAlphabet.forEach(symbol => {
            addTransitionRow(tbody, stateName, symbol);
        });
    });

    updateVisualization();
}

/**
 * Adiciona uma linha de transição
 */
function addTransitionRow(tbody, stateName, symbol) {
    const row = tbody.insertRow();

    // Coluna 1: Estado atual
    const stateCell = row.insertCell();
    let stateText = stateName;
    if (stateName === state.turingMachine.initialState) stateText = '→ ' + stateText;
    if (state.turingMachine.finalStates.includes(stateName)) stateText += ' *';
    stateCell.textContent = stateText;

    // Coluna 2: Símbolo lido
    const symbolCell = row.insertCell();
    symbolCell.textContent = symbol;

    // Coluna 3: Novo estado (select)
    const newStateCell = row.insertCell();
    const stateSelect = createSelect(state.turingMachine.states, stateName, symbol, 'state');
    newStateCell.appendChild(stateSelect);

    // Coluna 4: Novo símbolo (select)
    const newSymbolCell = row.insertCell();
    const symbolSelect = createSelect(state.turingMachine.tapeAlphabet, stateName, symbol, 'symbol');
    newSymbolCell.appendChild(symbolSelect);

    // Coluna 5: Direção (select)
    const directionCell = row.insertCell();
    const directionSelect = createSelect(['L', 'R', 'S'], stateName, symbol, 'direction');
    directionCell.appendChild(directionSelect);
}

/**
 * Cria um select com opções
 */
function createSelect(options, stateName, symbol, type) {
    const select = document.createElement('select');
    select.dataset.state = stateName;
    select.dataset.symbol = symbol;
    select.dataset.type = type;

    // Opção vazia
    const emptyOption = document.createElement('option');
    emptyOption.value = '';
    emptyOption.textContent = '-';
    select.appendChild(emptyOption);

    // Opções
    options.forEach(opt => {
        const option = document.createElement('option');
        option.value = opt;

        if (type === 'direction') {
            option.textContent = opt === 'L' ? '← L' : opt === 'R' ? '→ R' : '• S';
        } else {
            option.textContent = opt;
        }

        select.appendChild(option);
    });

    // Pré-selecionar valor se transição já existe
    const existingTransition = state.turingMachine.transitions[stateName]?.[symbol];
    if (existingTransition) {
        if (type === 'state' && existingTransition.nextState) {
            select.value = existingTransition.nextState;
        } else if (type === 'symbol' && existingTransition.newSymbol) {
            select.value = existingTransition.newSymbol;
        } else if (type === 'direction' && existingTransition.direction) {
            select.value = existingTransition.direction;
        }
    }

    // Event listener para atualizar transição
    select.addEventListener('change', (e) => updateTransition(e, stateName, symbol, type));

    return select;
}

/**
 * Atualiza transição quando select muda
 */
function updateTransition(event, stateName, symbol, type) {
    const value = event.target.value;

    // Inicializar transição se não existir
    if (!state.turingMachine.transitions[stateName]) {
        state.turingMachine.transitions[stateName] = {};
    }
    if (!state.turingMachine.transitions[stateName][symbol]) {
        state.turingMachine.transitions[stateName][symbol] = {
            nextState: '',
            newSymbol: '',
            direction: ''
        };
    }

    // Atualizar valor
    if (type === 'state') {
        state.turingMachine.transitions[stateName][symbol].nextState = value;
    } else if (type === 'symbol') {
        state.turingMachine.transitions[stateName][symbol].newSymbol = value;
    } else if (type === 'direction') {
        state.turingMachine.transitions[stateName][symbol].direction = value;
    }

    updateVisualization();
}

/**
 * Adiciona estado à tabela existente
 */
function addStateToTable(stateName) {
    const tbody = document.getElementById('transition-body');

    const orderedAlphabet = [...state.turingMachine.tapeAlphabet];
    const caretIndex = orderedAlphabet.indexOf('^');
    if (caretIndex > -1) {
        orderedAlphabet.splice(caretIndex, 1);
        orderedAlphabet.unshift('^');
    }

    orderedAlphabet.forEach(symbol => {
        addTransitionRow(tbody, stateName, symbol);
    });

    buildTransitionTable();
}

/**
 * Remove estado da tabela
 */
function removeStateFromTable(stateName) {
    // Implementar remoção de linhas da tabela
    // (Simplificado - na prática, reconstruir tabela completa)
    buildTransitionTable();
}

/**
 * Atualiza visualização da tabela no formato matricial
 */
function updateVisualization() {
    const thead = document.getElementById('visualization-head');
    const tbody = document.getElementById('visualization-body');

    // Limpar
    thead.innerHTML = '';
    tbody.innerHTML = '';

    if (state.turingMachine.states.length === 0) return;

    // Ordenar alfabeto da fita
    const orderedAlphabet = [...state.turingMachine.tapeAlphabet];
    const caretIndex = orderedAlphabet.indexOf('^');
    if (caretIndex > -1) {
        orderedAlphabet.splice(caretIndex, 1);
        orderedAlphabet.unshift('^');
    }

    // Criar cabeçalho
    const headerRow = thead.insertRow();
    const emptyCell = document.createElement('th');
    emptyCell.textContent = 'Estado \\ Símbolo';
    headerRow.appendChild(emptyCell);

    orderedAlphabet.forEach(symbol => {
        const th = document.createElement('th');
        th.textContent = symbol;
        headerRow.appendChild(th);
    });

    // Criar linhas para cada estado
    state.turingMachine.states.forEach(stateName => {
        const row = tbody.insertRow();

        // Primeira coluna: nome do estado
        const stateCell = row.insertCell();
        let stateText = stateName;
        if (stateName === state.turingMachine.initialState) stateText = '→ ' + stateText;
        if (state.turingMachine.finalStates.includes(stateName)) stateText += ' *';
        stateCell.textContent = stateText;
        stateCell.style.fontWeight = 'bold';

        // Colunas para cada símbolo
        orderedAlphabet.forEach(symbol => {
            const cell = row.insertCell();

            const trans = state.turingMachine.transitions[stateName]?.[symbol];

            if (trans && trans.nextState && trans.newSymbol && trans.direction) {
                const dirSymbol = trans.direction === 'L' ? '←' : trans.direction === 'R' ? '→' : '•';
                cell.textContent = `(${trans.nextState}, ${trans.newSymbol}, ${dirSymbol})`;
                cell.style.fontSize = '0.85rem';
            } else {
                cell.textContent = '-';
                cell.style.color = '#ccc';
            }
        });
    });
}

// ============================================================================
// Funções de Persistência
// ============================================================================

/**
 * Salva máquina no ESP32
 */
async function saveMachine() {
    const fileName = document.getElementById('file-name').value.trim();

    if (!fileName) {
        showToast('Por favor, digite um nome para o arquivo!', 'error');
        return;
    }

    // Validar se tabela foi gerada
    if (!state.tableGenerated) {
        showToast('Gere a tabela de transições primeiro!', 'error');
        return;
    }

    // Preparar JSON
    const json = JSON.stringify(state.turingMachine, null, 2);

    try {
        const response = await fetch(`/api/save?filename=${fileName}`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: json
        });

        const data = await response.json();

        if (response.ok) {
            showToast(`✓ Salvo como ${data.file}`, 'success');
            loadFilesList(); // Atualizar lista de arquivos
        } else {
            showToast(`Erro: ${data.error}`, 'error');
        }
    } catch (error) {
        showToast('Erro ao conectar com ESP32. Salvando localmente...', 'error');
        // Fallback: download local
        downloadJSON(json, fileName + '.json');
    }
}

/**
 * Carrega máquina do ESP32
 */
async function loadMachine() {
    const fileName = document.getElementById('file-name').value.trim();

    if (!fileName) {
        showToast('Por favor, digite o nome do arquivo!', 'error');
        return;
    }

    try {
        const response = await fetch(`/api/load?filename=${fileName}`);

        if (response.ok) {
            const data = await response.json();

            // Restaurar estado
            state.turingMachine = data;
            state.stateCounter = data.states.length;
            state.tableGenerated = true;

            // Atualizar interface
            document.getElementById('alphabet').value = data.alphabet.join('');
            const auxiliarySymbols = data.tapeAlphabet.filter(s => !data.alphabet.includes(s) && s !== '^' && s !== '_');
            document.getElementById('tape-alphabet').value = auxiliarySymbols.join('');

            // Reconstruir interface
            rebuildInterface();

            showToast('✓ Configuração carregada com sucesso!', 'success');
        } else {
            const error = await response.json();
            showToast(`Erro: ${error.error}`, 'error');
        }
    } catch (error) {
        console.error('Erro ao carregar arquivo:', error);
        showToast('Erro ao conectar com ESP32: ' + error.message, 'error');
    }
}

/**
 * Carrega lista de arquivos salvos
 */
async function loadFilesList() {
    try {
        const response = await fetch('/api/files');

        if (response.ok) {
            const data = await response.json();

            const section = document.getElementById('saved-files-section');
            const list = document.getElementById('saved-files-list');

            if (data.files.length > 0) {
                section.style.display = 'block';
                list.innerHTML = '';

                data.files.forEach(file => {
                    const li = document.createElement('li');
                    li.innerHTML = `
                        <span>${file.name} (${file.size} bytes)</span>
                        <button onclick="loadFile('${file.name.replace('.json', '')}')" class="btn-small">Carregar</button>
                        <button onclick="deleteFile('${file.name.replace('.json', '')}')" class="btn-small btn-danger">Deletar</button>
                    `;
                    list.appendChild(li);
                });
            } else {
                section.style.display = 'none';
            }
        }
    } catch (error) {
        console.log('Erro ao carregar lista de arquivos:', error);
    }
}

/**
 * Carrega arquivo específico
 */
async function loadFile(fileName) {
    document.getElementById('file-name').value = fileName;
    await loadMachine();
}

/**
 * Deleta arquivo do ESP32
 */
async function deleteFile(fileName) {
    if (!confirm(`Deseja realmente deletar ${fileName}.json?`)) {
        return;
    }

    try {
        const response = await fetch(`/api/delete?filename=${fileName}`, {
            method: 'DELETE'
        });

        if (response.ok) {
            showToast('✓ Arquivo deletado!', 'success');
            loadFilesList();
        } else {
            const error = await response.json();
            showToast(`Erro: ${error.error}`, 'error');
        }
    } catch (error) {
        showToast('Erro ao deletar arquivo', 'error');
    }
}

/**
 * Download local de JSON
 */
function downloadJSON(json, fileName) {
    const blob = new Blob([json], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = fileName;
    a.click();
    URL.revokeObjectURL(url);
    showToast('✓ Arquivo baixado localmente', 'success');
}

/**
 * Reconstrói interface após carregar configuração
 */
function rebuildInterface() {
    // Preencher nome e descrição (se existirem)
    if (state.turingMachine.nome) {
        document.getElementById('machine-name').value = state.turingMachine.nome;
    }

    if (state.turingMachine.descricao) {
        document.getElementById('machine-description').value = state.turingMachine.descricao;
    }

    // Reconstruir lista de estados
    const statesContainer = document.getElementById('states-container');
    statesContainer.innerHTML = '';

    state.turingMachine.states.forEach(stateName => {
        const isFinal = state.turingMachine.finalStates.includes(stateName);
        const isInitial = stateName === state.turingMachine.initialState;

        const stateDiv = document.createElement('div');
        stateDiv.className = `state-item ${isFinal ? 'state-final' : ''} ${isInitial ? 'state-initial' : ''}`;
        stateDiv.dataset.state = stateName;

        stateDiv.innerHTML = `
            <span class="state-name">${isInitial ? '→ ' : ''}${stateName}</span>
            ${isFinal ? '<span class="state-label">(final)</span>' : ''}
            ${isInitial ? '<span class="state-label">(inicial)</span>' : ''}
        `;

        statesContainer.appendChild(stateDiv);
    });

    // Mostrar seções
    document.getElementById('states-section').style.display = 'block';
    document.getElementById('transitions-section').style.display = 'block';
    document.getElementById('visualization-section').style.display = 'block';
    document.getElementById('execution-section').style.display = 'block';

    // Reconstruir tabelas
    buildTransitionTable();
    updateStateDisplays();
}

/**
 * Copia JSON para clipboard
 */
function copyJSON() {
    const json = JSON.stringify(state.turingMachine, null, 2);

    // Fallback para contextos sem HTTPS (como ESP32 AP mode)
    if (navigator.clipboard && navigator.clipboard.writeText) {
        navigator.clipboard.writeText(json)
            .then(() => showToast('JSON copiado para área de transferência!', 'success'))
            .catch(() => copyJSONFallback(json));
    } else {
        copyJSONFallback(json);
    }
}

/**
 * Fallback para copiar texto quando clipboard API não está disponível
 */
function copyJSONFallback(text) {
    const textarea = document.createElement('textarea');
    textarea.value = text;
    textarea.style.position = 'fixed';
    textarea.style.opacity = '0';
    document.body.appendChild(textarea);
    textarea.select();

    try {
        document.execCommand('copy');
        showToast('JSON copiado para área de transferência!', 'success');
    } catch (err) {
        showToast('Erro ao copiar JSON. Copie manualmente do console.', 'error');
        console.log('JSON da Máquina de Turing:');
        console.log(text);
    }

    document.body.removeChild(textarea);
}

/**
 * Valida entrada da fita - aceita apenas símbolos do alfabeto
 */
function validateTapeInput(event) {
    const input = event.target;
    const currentValue = input.value;

    // Se não há alfabeto configurado, não validar
    if (!state.turingMachine.alphabet || state.turingMachine.alphabet.length === 0) {
        return;
    }

    // Filtrar apenas símbolos válidos do alfabeto
    const validChars = currentValue.split('').filter(char =>
        state.turingMachine.alphabet.includes(char)
    ).join('');

    // Se houve mudança, atualizar campo e mostrar aviso
    if (validChars !== currentValue) {
        input.value = validChars;

        const invalidChars = currentValue.split('').filter(char =>
            !state.turingMachine.alphabet.includes(char)
        );

        if (invalidChars.length > 0) {
            const uniqueInvalid = [...new Set(invalidChars)].join(', ');
            showToast(`Símbolos inválidos removidos: ${uniqueInvalid}`, 'warning');
        }
    }
}

// ============================================================================
// Funções de Execução
// ============================================================================

/**
 * Executa máquina no servidor
 */
async function runMachine() {
    const input = document.getElementById('input-tape').value.trim();

    if (!input) {
        showToast('Por favor, digite uma entrada para processar!', 'error');
        return;
    }

    if (!state.tableGenerated) {
        showToast('Configure a máquina primeiro!', 'error');
        return;
    }

    // Preparar dados para envio
    const payload = {
        input: input,
        config: state.turingMachine
    };

    try {
        showToast('Executando...', 'info');

        const response = await fetch('/api/execute', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(payload)
        });

        if (response.ok) {
            const result = await response.json();

            // Exibir resultado
            displayExecutionResult(result);

            if (result.accepted) {
                showToast('✓ ' + result.message, 'success');
            } else {
                showToast('✗ ' + result.message, 'error');
            }
        } else {
            const errorText = await response.text();
            console.error('Resposta de erro:', errorText);
            showToast('Erro ao executar máquina: ' + response.status, 'error');
        }
    } catch (error) {
        console.error('Erro ao executar máquina:', error);
        showToast('Erro ao conectar com ESP32: ' + error.message, 'error');
    }
}

/**
 * Exibe resultado da execução
 */
function displayExecutionResult(result) {
    const resultBox = document.getElementById('result-box');
    const resultContent = document.getElementById('result-content');

    let output = '';
    output += `Status: ${result.accepted ? '✓ ACEITO' : '✗ REJEITADO'}\n`;
    output += `Mensagem: ${result.message}\n`;
    output += `Passos executados: ${result.steps}\n`;
    output += `Fita final: ${result.finalTape}\n\n`;

    // Exibir histórico de execução
    if (result.history && result.history.length > 0) {
        output += 'Histórico de Execução:\n';
        output += '═══════════════════════════════════\n\n';

        result.history.forEach((step) => {
            output += `Passo ${step.step}:\n`;
            output += `  Estado: ${step.state}\n`;
            output += `  Posição: ${step.position}\n`;
            output += `  Símbolo: ${step.symbol}\n`;
            output += `  Fita: ${step.tape}\n\n`;
        });
    }

    resultContent.textContent = output;
    resultBox.style.display = 'block';

    // Scroll suave até o resultado
    resultBox.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
}

/**
 * Executa máquina no display OLED
 */
async function runOnDisplay() {
    if (!state.tableGenerated) {
        showToast('Gere a tabela de transições primeiro!', 'error');
        return;
    }

    const input = document.getElementById('input-tape').value.trim();

    if (!input) {
        showToast('Digite uma entrada para executar!', 'error');
        return;
    }

    try {
        const payload = {
            input: input,
            config: state.turingMachine,
            delay: 500  // Delay entre passos (ms)
        };

        const response = await fetch('/api/execute-display', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(payload)
        });

        if (response.ok) {
            showToast('✓ Executando no display OLED!', 'success');
        } else if (response.status === 503) {
            showToast('Display OLED não está ativo', 'error');
        } else {
            const errorText = await response.text();
            console.error('Resposta de erro:', errorText);
            showToast('Erro ao executar no display: ' + response.status, 'error');
        }
    } catch (error) {
        console.error('Erro ao executar no display:', error);
        showToast('Erro ao conectar com ESP32: ' + error.message, 'error');
    }
}

/**
 * Inicia modo passo a passo no ESP32
 * Após iniciar, usuário controla pelos botões físicos
 */
async function startStepMode() {
    const input = document.getElementById('input-tape').value.trim();

    if (!input) {
        showToast('Por favor, digite uma entrada para processar!', 'error');
        return;
    }

    if (!state.tableGenerated) {
        showToast('Configure a máquina primeiro!', 'error');
        return;
    }

    // Preparar dados
    const payload = {
        input: input,
        config: state.turingMachine
    };

    try {
        showToast('Iniciando modo passo a passo...', 'info');

        const response = await fetch('/api/start-step-mode', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(payload)
        });

        if (response.ok) {
            const data = await response.json();
            showToast('✓ ' + data.message, 'success');
        } else if (response.status === 503) {
            showToast('Display OLED não está ativo', 'error');
        } else {
            const error = await response.json();
            showToast('Erro: ' + error.error, 'error');
        }
    } catch (error) {
        console.error('Erro ao iniciar modo passo a passo:', error);
        showToast('Erro ao conectar com ESP32: ' + error.message, 'error');
    }
}

// ============================================================================
// Fim do arquivo
// ============================================================================

console.log('Script carregado - Aguardando interação do usuário');
