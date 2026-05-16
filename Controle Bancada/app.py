from flask import Flask, request, jsonify, render_template

app = Flask(__name__, template_folder='.', static_folder='.', static_url_path='')

SENHA_MESTRE = "5555"
sistema_desbloqueado = False

@app.route('/')
def index():
    if sistema_desbloqueado:
        return render_template('painel.html')
    return render_template('index.html')

@app.route('/validar_senha', methods=['POST'])
def validar_senha():
    global sistema_desbloqueado
    dados = request.get_json()
    if dados.get('senha') == SENHA_MESTRE:
        sistema_desbloqueado = True
        return jsonify({"status": "sucesso"})
    return jsonify({"status": "erro"}), 401

@app.route('/painel')
def painel():
    if not sistema_desbloqueado:
        return render_template('index.html') # Se não tiver logado, joga pro index
    return render_template('painel.html')

# --- NOVAS ROTAS DA API DE CONTROLE ---

@app.route('/api/ligar', methods=['POST'])
def api_ligar():
    global sistema_desbloqueado
    if not sistema_desbloqueado: return "Acesso Negado", 403
    
    dados = request.get_json()
    minutos = dados.get('minutos')
    segundos = dados.get('segundos')
    print(f"[COMANDO] Ligar relé por {minutos} min e {segundos} seg.")
    # No futuro, aqui vai o código MQTT que envia a mensagem pro ESP32
    return jsonify({"status": "ligado"})

@app.route('/api/desligar', methods=['POST'])
def api_desligar():
    global sistema_desbloqueado
    if not sistema_desbloqueado: return "Acesso Negado", 403
    
    print("[COMANDO] Desligar relé.")
    # No futuro, aqui vai o código MQTT que manda o ESP32 parar
    return jsonify({"status": "desligado"})

@app.route('/api/bloquear', methods=['POST'])
def api_bloquear():
    global sistema_desbloqueado
    dados = request.get_json()
    senha_digitada = dados.get('senha')

    # Confere se a senha para bloquear é a mesma
    if senha_digitada == SENHA_MESTRE:
        sistema_desbloqueado = False
        print("[SISTEMA] Sistema Bloqueado.")
        return jsonify({"status": "sucesso"})
    else:
        return jsonify({"status": "erro", "mensagem": "Senha Incorreta"}), 401

@app.route('/api/alterar_senha', methods=['POST'])
def api_alterar_senha():
    # Precisamos da instrução "global" para conseguir alterar a variável original
    global SENHA_MESTRE, sistema_desbloqueado
    
    if not sistema_desbloqueado:
        return jsonify({"status": "erro", "mensagem": "Acesso Negado"}), 403

    dados = request.get_json(silent=True)
    if not dados:
        return jsonify({"status": "erro", "mensagem": "Dados inválidos"}), 400

    senha_atual = dados.get('senha_atual')
    nova_senha = dados.get('nova_senha')

    # Validações de segurança
    if senha_atual != SENHA_MESTRE:
        return jsonify({"status": "erro", "mensagem": "Senha atual incorreta."}), 401
    
    if len(nova_senha) != 4 or not nova_senha.isdigit():
        return jsonify({"status": "erro", "mensagem": "A nova senha deve ter 4 dígitos numéricos."}), 400

    # Atualiza a senha no sistema
    SENHA_MESTRE = nova_senha
    print(f"[SISTEMA] Senha mestre alterada para: {SENHA_MESTRE}")
    return jsonify({"status": "sucesso"})

if __name__ == '__main__':
    porta = int(os.environ.get("PORT", 5000))
    app.run(host='0.0.0.0', port=porta)