document.addEventListener('DOMContentLoaded', () => {
    const inputMin = document.getElementById('input-min');
    const inputSec = document.getElementById('input-sec');
    const displayTempo = document.getElementById('display-tempo');
    const btnLigar = document.getElementById('btn-ligar');
    const btnDesligar = document.getElementById('btn-desligar');
    const areaConfig = document.getElementById('area-config');
    
    // Elementos do Modal
    const btnBloquear = document.getElementById('btn-bloquear');
    const modalBloqueio = document.getElementById('modal-bloqueio');
    const btnConfirmarBloqueio = document.getElementById('btn-confirmar-bloqueio');
    const btnCancelarBloqueio = document.getElementById('btn-cancelar-bloqueio');
    const inputsLock = document.querySelectorAll('.pin-box-lock');

    let intervaloTimer = null;
    let tempoRestante = 0;

    // --- LÓGICA DE FORMATAÇÃO DE TEMPO (2 DÍGITOS) ---
    function formatarTempo(segundosTotais) {
        let min = Math.floor(segundosTotais / 60);
        let sec = segundosTotais % 60;
        return `${min.toString().padStart(2, '0')}:${sec.toString().padStart(2, '0')}`;
    }

    function forcarDoisDigitos(e) {
        // Remove tudo que não for número
        let valLimpo = e.target.value.replace(/[^0-9]/g, '');
        let val = parseInt(valLimpo);
        if (isNaN(val)) val = 0;
        
        // Aplica limites de 0 a 30 (min) e 0 a 59 (sec)
        if (e.target.id === 'input-min' && val > 30) val = 30;
        if (e.target.id === 'input-sec' && val > 59) val = 59;
        
        // Força sempre ter 2 dígitos preenchendo com zero à esquerda
        e.target.value = val.toString().padStart(2, '0');
        atualizarDisplayPeloInput();
    }

    // Formata o campo quando o usuário clica fora dele (blur)
    inputMin.addEventListener('blur', forcarDoisDigitos);
    inputSec.addEventListener('blur', forcarDoisDigitos);

    function atualizarDisplayPeloInput() {
        let min = parseInt(inputMin.value) || 0;
        let sec = parseInt(inputSec.value) || 0;
        tempoRestante = (min * 60) + sec;
        displayTempo.innerText = formatarTempo(tempoRestante);
    }

    // --- LÓGICA DO RELÉ (Ligar/Desligar) ---
    btnLigar.addEventListener('click', () => {
        if (tempoRestante <= 0) return alert("Defina um tempo maior que zero!");

        areaConfig.style.display = 'none';
        btnLigar.style.display = 'none';
        btnDesligar.style.display = 'inline-block';

        fetch('/api/ligar', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ minutos: Math.floor(tempoRestante/60), segundos: tempoRestante%60 })
        });

        intervaloTimer = setInterval(() => {
            tempoRestante--;
            displayTempo.innerText = formatarTempo(tempoRestante);
            if (tempoRestante <= 0) desligarSistema();
        }, 1000);
    });

    function desligarSistema() {
        clearInterval(intervaloTimer);
        areaConfig.style.display = 'flex';
        btnLigar.style.display = 'inline-block';
        btnDesligar.style.display = 'none';
        fetch('/api/desligar', { method: 'POST' });
        atualizarDisplayPeloInput();
    }

    btnDesligar.addEventListener('click', desligarSistema);

    // --- LÓGICA DO MODAL DE BLOQUEIO (Senha) ---
    btnBloquear.addEventListener('click', () => {
        modalBloqueio.style.display = 'flex'; // Mostra o modal
        inputsLock[0].focus();
    });

    btnCancelarBloqueio.addEventListener('click', () => {
        modalBloqueio.style.display = 'none'; // Esconde o modal
        inputsLock.forEach(input => input.value = ''); // Limpa os campos
    });

    // Navegação nas caixas de senha do Modal
    inputsLock.forEach((input, index) => {
        input.addEventListener('input', (e) => {
            e.target.value = e.target.value.replace(/[^0-9]/g, '');
            if (e.target.value !== '' && index < inputsLock.length - 1) inputsLock[index + 1].focus();
        });
        input.addEventListener('keydown', (e) => {
            if (e.key === 'Backspace' && e.target.value === '' && index > 0) inputsLock[index - 1].focus();
        });
    });

    // Envio da senha de bloqueio para o Python
    btnConfirmarBloqueio.addEventListener('click', () => {
        let senhaCompleta = '';
        inputsLock.forEach(input => senhaCompleta += input.value);

        if (senhaCompleta.length === 4) {
            fetch('/api/bloquear', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ senha: senhaCompleta })
            })
            .then(res => res.json())
            .then(data => {
                if (data.status === 'sucesso') {
                    window.location.href = '/'; // Volta pra tela de login principal
                } else {
                    alert('Senha incorreta! Não foi possível bloquear.');
                    inputsLock.forEach(input => input.value = '');
                    inputsLock[0].focus();
                }
            });
        } else {
            alert('Por favor, preencha os 4 dígitos da senha.');
        }
    });

    // Inicialização
    atualizarDisplayPeloInput();
    // --- LÓGICA DO MODAL DE ALTERAR SENHA ---
    const btnAbrirAlterar = document.getElementById('btn-abrir-alterar');
    const modalAlterar = document.getElementById('modal-alterar-senha');
    const btnConfirmarAlteracao = document.getElementById('btn-confirmar-alteracao');
    const btnCancelarAlteracao = document.getElementById('btn-cancelar-alteracao');
    const inputSenhaAtual = document.getElementById('input-senha-atual');
    const inputNovaSenha = document.getElementById('input-nova-senha');

    // Abre o modal
    btnAbrirAlterar.addEventListener('click', () => {
        modalAlterar.style.display = 'flex';
        inputSenhaAtual.value = '';
        inputNovaSenha.value = '';
        inputSenhaAtual.focus();
    });

    // Fecha o modal
    btnCancelarAlteracao.addEventListener('click', () => {
        modalAlterar.style.display = 'none';
    });

    // Força apenas números nos inputs de alteração
    [inputSenhaAtual, inputNovaSenha].forEach(input => {
        input.addEventListener('input', (e) => {
            e.target.value = e.target.value.replace(/[^0-9]/g, '');
        });
    });

    // Confirma a alteração e envia para o Python
    btnConfirmarAlteracao.addEventListener('click', () => {
        const atual = inputSenhaAtual.value;
        const nova = inputNovaSenha.value;

        if (atual.length !== 4 || nova.length !== 4) {
            alert("As senhas devem ter exatos 4 dígitos.");
            return;
        }

        fetch('/api/alterar_senha', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ senha_atual: atual, nova_senha: nova })
        })
        .then(res => res.json())
        .then(data => {
            if (data.status === 'sucesso') {
                alert('Senha alterada com sucesso!');
                modalAlterar.style.display = 'none';
            } else {
                alert('Erro: ' + data.mensagem);
            }
        });
    });
    // Verifica a cada 1 segundo como estão as coisas no ESP32 físico
    setInterval(() => {
        fetch('/api/status')
        .then(res => res.json())
        .then(data => {
            // Se alguém bloqueou no físico, o site expulsa o usuário de volta pro Login
            if(!data.desbloqueado) {
                window.location.href = "/";
                return;
            }

            // Sincroniza os botões (Ligar/Desligar)
            if(data.ligado) {
                document.getElementById('btn-ligar').style.display = 'none';
                document.getElementById('btn-desligar').style.display = 'inline-block';
            } else {
                document.getElementById('btn-ligar').style.display = 'inline-block';
                document.getElementById('btn-desligar').style.display = 'none';
            }

            // Sincroniza o relógio (só atualiza se o usuário não estiver com o mouse clicado digitando algo)
            if (document.activeElement !== document.getElementById('input-min') && document.activeElement !== document.getElementById('input-sec')) {
                let m = data.minutos.toString().padStart(2, '0');
                let s = data.segundos.toString().padStart(2, '0');
                
                document.getElementById('input-min').value = m;
                document.getElementById('input-sec').value = s;
                document.getElementById('display-tempo').innerText = `${m}:${s}`;
            }
        })
        .catch(err => console.log("Aguardando servidor..."));
    }, 1000);
});