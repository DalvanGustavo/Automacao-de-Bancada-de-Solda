from flask import Flask, request, jsonify, render_template
import os
import paho.mqtt.client as mqtt

app = Flask(__name__, template_folder='.', static_folder='.', static_url_path='')

SENHA_MESTRE = "5555"
sistema_desbloqueado = False

# Variáveis para espelhar o status do mundo físico
esp_ligado = False
esp_minutos = 10
esp_segundos = 0

# --- CONFIGURAÇÃO MQTT ---
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_TOPIC_COMANDOS = "meu_esp32_c3_supermini/comandos"
MQTT_TOPIC_STATUS = "meu_esp32_c3_supermini/status" # Novo tópico que o ESP vai enviar

def on_connect(client, userdata, flags, rc):
    print("[MQTT] Conectado ao Broker!")
    client.subscribe(MQTT_TOPIC_STATUS)

def on_message(client, userdata, msg):
    global sistema_desbloqueado, esp_ligado, esp_minutos, esp_segundos
    payload = msg.payload.decode('utf-8')
    
    # Se recebeu uma mensagem de STATUS do ESP32
    # Formato esperado: STATUS 1 1 09 59 (Desbloqueado, Ligado, 9 min, 59 seg)
    if payload.startswith("STATUS"):
        partes = payload.split()
        if len(partes) == 5:
            # Atualiza o Python de acordo com o que o ESP32 informou
            sistema_desbloqueado = (partes[1] == '1')
            esp_ligado = (partes[2] == '1')
            esp_minutos = int(partes[3])
            esp_segundos = int(partes[4])

mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
mqtt_client.loop_start()
# -------------------------

@app.route('/')
def index():
    if sistema_desbloqueado:
        return render_template('painel.html')
    return render_template('index.html')

# --- NOVA ROTA PARA O SITE CONSULTAR O STATUS REAL ---
@app.route('/api/status', methods=['GET'])
def api_status():
    return jsonify({
        "desbloqueado": sistema_desbloqueado,
        "ligado": esp_ligado,
        "minutos": esp_minutos,
        "segundos": esp_segundos
    })

@app.route('/validar_senha', methods=['POST'])
def validar_senha():
    global sistema_desbloqueado
    dados = request.get_json()
    if dados.get('senha') == SENHA_MESTRE:
        sistema_desbloqueado = True
        mqtt_client.publish(MQTT_TOPIC_COMANDOS, "DESBLOQUEAR")
        return jsonify({"status": "sucesso"})
    return jsonify({"status": "erro"}), 401

@app.route('/painel')
def painel():
    if not sistema_desbloqueado:
        return render_template('index.html')
    return render_template('painel.html')

@app.route('/api/ligar', methods=['POST'])
def api_ligar():
    global sistema_desbloqueado
    if not sistema_desbloqueado: return "Acesso Negado", 403
    dados = request.get_json()
    min = dados.get('minutos')
    sec = dados.get('segundos')
    mqtt_client.publish(MQTT_TOPIC_COMANDOS, f"LIGAR {min} {sec}")
    return jsonify({"status": "ligado"})

@app.route('/api/desligar', methods=['POST'])
def api_desligar():
    global sistema_desbloqueado
    if not sistema_desbloqueado: return "Acesso Negado", 403
    mqtt_client.publish(MQTT_TOPIC_COMANDOS, "DESLIGAR")
    return jsonify({"status": "desligado"})

@app.route('/api/bloquear', methods=['POST'])
def api_bloquear():
    global sistema_desbloqueado
    dados = request.get_json()
    if dados.get('senha') == SENHA_MESTRE:
        sistema_desbloqueado = False
        mqtt_client.publish(MQTT_TOPIC_COMANDOS, "BLOQUEAR")
        return jsonify({"status": "sucesso"})
    return jsonify({"status": "erro", "mensagem": "Senha Incorreta"}), 401

@app.route('/api/alterar_senha', methods=['POST'])
def api_alterar_senha():
    global SENHA_MESTRE, sistema_desbloqueado
    if not sistema_desbloqueado: return jsonify({"status": "erro"}), 403
    dados = request.get_json(silent=True)
    senha_atual = dados.get('senha_atual')
    nova_senha = dados.get('nova_senha')

    if senha_atual != SENHA_MESTRE:
        return jsonify({"status": "erro", "mensagem": "Senha atual incorreta."}), 401
    
    SENHA_MESTRE = nova_senha
    mqtt_client.publish(MQTT_TOPIC_COMANDOS, f"SENHA {SENHA_MESTRE}")
    return jsonify({"status": "sucesso"})

if __name__ == '__main__':
    porta = int(os.environ.get("PORT", 5000))
    app.run(host='0.0.0.0', port=porta)