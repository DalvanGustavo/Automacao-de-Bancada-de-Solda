document.addEventListener('DOMContentLoaded', () => {
    // Lógica Formulário: Pág 1
    const form = document.getElementById('formConsultar');
    if (form) {
        const inputs = document.querySelectorAll('.pin-box');
        const btnEntrar = document.getElementById('btn-entrar');

        inputs.forEach((input, index) => {
        // Evento que dispara sempre que algo é digitado
        input.addEventListener('input', (e) => {
            
            // Limpa qualquer coisa que não seja número (0-9)
            e.target.value = e.target.value.replace(/[^0-9]/g, '');

            // Se o campo foi preenchido, pula para o próximo
            if (e.target.value !== '') {
            if (index < inputs.length - 1) {
                inputs[index + 1].focus();
            }
            }
        });

        // Evento para capturar a tecla "Backspace"
        input.addEventListener('keydown', (e) => {
            // Se apertar Backspace e a caixa atual já estiver vazia, volta o foco para a anterior
            if (e.key === 'Backspace' && e.target.value === '') {
            if (index > 0) {
                inputs[index - 1].focus();
            }
            }
        });
        });

        // Lógica para capturar a senha inteira quando clicar no botão
        btnEntrar.addEventListener('click', (e) => {
            e.preventDefault();
                
            let senhaCompleta = '';
            
            // Junta os valores das 4 caixas
            inputs.forEach(input => {
                senhaCompleta += input.value;
        });
        
        // Verifica se o usuário preencheu tudo
        if (senhaCompleta.length === 4) {
                console.log('Senha a ser enviada para o Python:', senhaCompleta);
                
                // --- NOVA INTEGRAÇÃO COM PYTHON AQUI ---
                fetch('/validar_senha', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ senha: senhaCompleta })
                })
                .then(response => response.json())
                .then(data => {
                    if(data.status === 'sucesso') {
                        // Se o Python disser que a senha tá certa, vai pro painel!
                        window.location.href = "/painel"; 
                    } else {
                        alert('Senha incorreta! Tente novamente.');
                        // Opcional: Limpar as caixas de senha aqui
                    }
                });
                // ---------------------------------------
                
            } else {
                alert('Por favor, preencha os 4 dígitos da senha.');
            }
        });
    }
});
