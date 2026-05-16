document.addEventListener('DOMContentLoaded', () => {
    const inputMin = document.getElementById('input-min');
    const inputSec = document.getElementById('input-sec');
    const displayTempo = document.getElementById('display-tempo');
    const btnLigar = document.getElementById('btn-ligar');
    const btnDesligar = document.getElementById('btn-desligar');
    
    // Elementos dos Modais
    const btnBloquear = document.getElementById('btn-bloquear');
    const modalBloqueio = document.getElementById('modal-bloqueio');
    const btnConfirmarBloqueio = document.getElementById('btn-confirmar-bloqueio');
    const btnCancelarBloqueio = document.getElementById('btn-cancelar-bloqueio');
    const inputsLock = document.querySelectorAll('.pin-box-lock');

    const btnAlterar = document.getElementById('btn-alterar-senha');
    const modalAlterar = document.getElementById('modal-alterar-senha');
    const btnConfirmarAlteracao = document.getElementById('btn-confirmar-alteracao');
    const btnCancelarAlteracao = document.getElementById('btn-cancelar-alteracao');
    const inputSenhaAtual = document.getElementById('input-senha-atual');
    const inputNovaSenha = document.getElementById('input-nova-senha');

    // Forçar 2 dígitos nos inputs de tempo
    function forcarDoisDigitos(e) {
        let valLimpo = e.target.value.replace(/[^0-9]/g, '');
        if (valLimpo.length > 2) valLimpo = valLimpo.slice(0, 2);
        e.target.value = valLimpo;
    }
    inputMin.addEventListener('input', forcarDoisDigitos);
    inputSec.addEventListener('input', forcarDoisDigitos);

    function formatarAoSair(e) {
        e.target.value = e.target.value.padStart(2, '0');
    }
    inputMin.addEventListener('blur', formatarAoSair);
    inputSec.addEventListener('blur', formatarAoSair);

    // Ações dos Botões principais (Enviam comandos para o Python -> ESP32)
    btnLigar.addEventListener('click', () => {
        const m = parseInt(inputMin.value) || 0;
        const s = parseInt(inputSec.value) || 0;
        
        fetch('/api/ligar', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ minutos: m, segundos: s })
        });
    });

    btnDesligar.addEventListener('click', () => {
        fetch('/api/desligar', { method: 'POST' });
    });

    // --- MODAL DE BLOQUEIO ---
    btnBloquear.addEventListener('click', () => {
        modalBloqueio.style.display = 'flex';
        inputsLock.forEach(input => input.value = '');
        inputsLock[0].focus();
    });

    btnCancelarBloqueio.addEventListener('click', () => {
        modalBloqueio.style.display = 'none';
    });

    inputsLock.forEach((input, index) => {
        input.addEventListener('input', (e) => {
            e.target.value = e.target.value.replace(/[^0-9]/g, '');
            if (e.target.value !== '' && index < inputsLock.length - 1) {
                inputsLock[index + 1].focus();
            }
        });
        input.addEventListener('keydown', (e) => {
            if (e.key === 'Backspace' && e.target.value === '' && index > 0) {
                inputsLock[index - 1].focus();
            }
        });
    });

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
                    window.location.href = "/";
                } else {
                    alert('Senha incorreta para bloquear!');
                }
            });
        } else {
            alert('Preencha os 4 dígitos.');
        }
    });

    // --- MODAL ALTERAR SENHA ---
    btnAlterar.addEventListener('click', () => {
        modalAlterar.style.display = 'flex';
        inputSenhaAtual.value = '';
        inputNovaSenha.value = '';
        inputSenhaAtual.focus();
    });

    btnCancelarAlteracao.addEventListener('click', () => {
        modalAlterar.style.display = 'none';
    });

    [inputSenhaAtual, inputNovaSenha].forEach(input => {
        input.addEventListener('input', (e) => {
            e.target.value = e.target.value.replace(/[^0-9]/g, '');
        });
    });

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
                alert("Senha alterada com sucesso!");
                modalAlterar.style.display = 'none';
            } else {
                alert(data.mensagem || "Erro ao alterar a senha.");
            }
        });
    });

    // --- SINCRONIZAÇÃO EM TEMPO REAL COM O ESP32 (Evita o Bug) ---
    setInterval(() => {
        fetch('/api/status')
        .then(res => res.json())
        .then(data => {
            // Se o ESP32 foi bloqueado fisicamente, desloga o site
            if (!data.desbloqueado) {
                window.location.href = "/";
                return;
            }

            // Altera os botões Ligar/Desligar com base no Relé real do ESP32
            if (data.ligado) {
                btnLigar.style.display = 'none';
                btnDesligar.style.display = 'inline-block';
            } else {
                btnLigar.style.display = 'inline-block';
                btnDesligar.style.display = 'none';
            }

            // Atualiza os números na tela do site (Apenas se o usuário não estiver digitando)
            if (document.activeElement !== inputMin && document.activeElement !== inputSec) {
                let m = data.minutos.toString().padStart(2, '0');
                let s = data.segundos.toString().padStart(2, '0');
                
                inputMin.value = m;
                inputSec.value = s;
                displayTempo.innerText = `${m}:${s}`;
            }
        })
        .catch(err => console.log("Buscando dados do ESP32..."));
    }, 1000);
});