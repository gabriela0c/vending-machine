from flask import Flask, jsonify, request
import face_recognition
import os
import cv2
import numpy as np
import time
import mediapipe as mp
import threading
import requests

app = Flask(__name__)

# 0 = Webcam do notebook
# 1 = DroidCam
VIDEO_SOURCE = 0

KNOWN_FACES_DIR = "known_faces"
TOLERANCE = 0.6
FRAME_THICKNESS = 3
FONT_THICKNESS = 2
MODEL = "hog" 

# Estados da Máquina de Câmera
STATE_GESTURE = "GESTOS"
STATE_FACE = "RECONHECIMENTO_FACIAL"
STATE_WAITING = "AGUARDANDO_ESP"

current_state = STATE_GESTURE

print("Carregando rostos conhecidos...")
known_faces = []
known_names = []

if not os.path.exists(KNOWN_FACES_DIR):
    os.makedirs(KNOWN_FACES_DIR)

for filename in os.listdir(KNOWN_FACES_DIR):
    if filename.startswith('.'):
        continue
    filepath = os.path.join(KNOWN_FACES_DIR, filename)
    if os.path.isfile(filepath):
        image = face_recognition.load_image_file(filepath)
        encodings = face_recognition.face_encodings(image)
        if len(encodings) > 0:
            known_faces.append(encodings[0])
            name = os.path.splitext(filename)[0]
            known_names.append(name)
            print(f"Rosto de {name} carregado.")

def executarReconhecimento():
    print("Iniciando a webcam para RECONHECIMENTO FACIAL")
    video_capture = cv2.VideoCapture(VIDEO_SOURCE)
    match = "Desconhecido" 
    timeout = time.time() + 10 

    while time.time() < timeout:
        ret, frame = video_capture.read()
        if not ret:
            break

        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        locations = face_recognition.face_locations(rgb_frame, model=MODEL)
        encodings = face_recognition.face_encodings(rgb_frame, locations)

        for face_encoding, face_location in zip(encodings, locations):
            results = face_recognition.compare_faces(known_faces, face_encoding, TOLERANCE)
            color = [0, 0, 255]    
            face_distances = face_recognition.face_distance(known_faces, face_encoding)
            if len(face_distances) > 0:
                best_match_index = np.argmin(face_distances)
                if results[best_match_index]:
                    match = known_names[best_match_index]
                    color = [0, 255, 0] 

            top, right, bottom, left = face_location
            cv2.rectangle(frame, (left, top), (right, bottom), color, FRAME_THICKNESS)
            cv2.rectangle(frame, (left, bottom - 25), (right, bottom), color, cv2.FILLED)
            cv2.putText(frame, match, (left + 6, bottom - 6), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), FONT_THICKNESS)

        cv2.imshow("Vending Machine - Reconhecimento Facial", frame)
        if match != "Desconhecido" or cv2.waitKey(1) & 0xFF == ord('q'):
            break
        
    video_capture.release()
    cv2.destroyAllWindows()
    return match

def executarCadastro(nome):
    print(f"Aguardando posicionamento do rosto para: {nome}")
    video_capture = cv2.VideoCapture(VIDEO_SOURCE)
    timeout = time.time() + 15  
    sucesso = False

    while time.time() < timeout:
        ret, frame = video_capture.read()
        if not ret:
            break

        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        locations = face_recognition.face_locations(rgb_frame, model=MODEL)
        frame_instrucao = frame.copy()

        if len(locations) == 1:
            top, right, bottom, left = locations[0]
            cv2.rectangle(frame_instrucao, (left, top), (right, bottom), (0, 255, 0), FRAME_THICKNESS)
            cv2.putText(frame_instrucao, "Rosto Detectado! Capturando...", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), FONT_THICKNESS)
            cv2.imshow("Cadastro de Face", frame_instrucao)
            cv2.waitKey(1000) 

            filepath = os.path.join(KNOWN_FACES_DIR, f"{nome}.jpg")
            cv2.imwrite(filepath, frame)

            image = face_recognition.load_image_file(filepath)
            encodings = face_recognition.face_encodings(image)
            if len(encodings) > 0:
                known_faces.append(encodings[0])
                known_names.append(nome)
                sucesso = True
                break
        
        cv2.imshow("Cadastro de Face", frame_instrucao)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    video_capture.release()
    cv2.destroyAllWindows()
    return sucesso

# --- LOOP DE GESTOS EM BACKGROUND THREAD ---
def loop_reconhecimento_gestos():
    global current_state
    mp_maos = mp.solutions.hands
    maos = mp_maos.Hands(max_num_hands=1, min_detection_confidence=0.7)
    mp_desenho = mp.solutions.drawing_utils
    
    camera = None
    contador_frames_estaveis = 0
    ultimo_produto_detectado = 0

    print("Thread de Gestos iniciada. Aguardando modo GESTOS...")

    while True:
        if current_state == STATE_GESTURE:
            if camera is None or not camera.isOpened():
                camera = cv2.VideoCapture(VIDEO_SOURCE)
                print("Camera aberta para deteccao de GESTOS.")

            sucesso, frame = camera.read()
            if not sucesso:
                time.sleep(0.03)
                continue

            frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            resultados = maos.process(frame_rgb)
            produto_atual = 0

            if resultados.multi_hand_landmarks:
                for mao_pontos in resultados.multi_hand_landmarks:
                    mp_desenho.draw_landmarks(frame, mao_pontos, mp_maos.HAND_CONNECTIONS)
                    dedos_levantados = 0
                    pontas = [8, 12, 16, 20]
                    juntas = [6, 10, 14, 18]

                    for i in range(4):
                        if mao_pontos.landmark[pontas[i]].y < mao_pontos.landmark[juntas[i]].y:
                            dedos_levantados += 1

                    if 1 <= dedos_levantados <= 4:
                        produto_atual = dedos_levantados

            # Filtro de estabilidade para evitar falsos positivos rápidos
            if produto_atual != 0:
                if produto_atual == ultimo_produto_detectado:
                    contador_frames_estaveis += 1
                else:
                    ultimo_produto_detectado = produto_atual
                    contador_frames_estaveis = 1
            else:
                contador_frames_estaveis = 0
                ultimo_produto_detectado = 0

            # Se o mesmo gesto se mantiver por 15 frames (~0.5 segundos)
            if contador_frames_estaveis >= 100:
                print(f"Gesto Confirmado: Produto {ultimo_produto_detectado}!")
                
                # Feedback rápido na tela antes de fechar
                cv2.putText(frame, f"Enviando Produto {ultimo_produto_detectado}...", (50, 80), cv2.FONT_HERSHEY_SIMPLEX, 1.2, (0, 255, 0), 3)
                cv2.imshow("Vending Machine - Gestos", frame)
                cv2.waitKey(500)

                # FECHA A CÂMERA IMEDIATAMENTE para liberar para o reconhecimento facial
                camera.release()
                cv2.destroyAllWindows()
                camera = None

                # Muda o estado para não reabrir a câmera aqui
                current_state = STATE_WAITING

                # Envia o comando para o IP padrão do Access Point do ESP32
                def comunicar_esp(prod):
                    try:
                        requests.post("http://192.168.4.1/selecionar-produto-gesto", json={"produto": prod}, timeout=4)
                    except Exception as e:
                        print(f"Erro ao enviar para o ESP32: {e}")
                
                threading.Thread(target=comunicar_esp, args=(ultimo_produto_detectado,)).start()
                
                contador_frames_estaveis = 0
                ultimo_produto_detectado = 0
                continue

            cv2.putText(frame, "Mostre de 1 a 4 dedos", (30, 40), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 0, 0), 2)
            cv2.imshow("Vending Machine - Gestos", frame)
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
        else:
            # Se não estiver no estado de gestos, garante que a câmera está fechada
            if camera is not None and camera.isOpened():
                camera.release()
                cv2.destroyAllWindows()
                camera = None
            time.sleep(0.2) # Dorme um pouco para poupar CPU

# --- ROTAS FLASK ---
@app.route("/get-usuario", methods=["POST"])
def disparar_camera():
    global current_state
    print("\n[HTTP] Requisicao de RECONHECIMENTO recebida!")
    current_state = STATE_FACE
    
    usuario = executarReconhecimento()
    
    current_state = STATE_GESTURE  # Devolve o controle para o loop de gestos
    return jsonify({"usuario": usuario})

@app.route("/cadastrar-rosto", methods=["POST"])
def cadastrar_rosto():
    global current_state
    print("\n[HTTP] Requisicao de CADASTRO recebida!")
    dados = request.get_json()
    nome = dados.get("nome")
    
    if not nome:
        return jsonify({"status": "erro", "mensagem": "Nome nao enviado"}), 400
        
    current_state = STATE_FACE
    sucesso = executarCadastro(nome)
    current_state = STATE_GESTURE
    
    if sucesso:
        return jsonify({"status": "sucesso", "mensagem": f"Rosto de {nome} cadastrado!"}), 200
    else:
        return jsonify({"status": "erro", "mensagem": "Nao foi possivel detectar o rosto ou deu Timeout"}), 500
    
@app.route("/deletar-rosto", methods=["DELETE"])
def deletar_rosto():
    nome = request.args.get("nome")
    if not nome:
        return jsonify({"sucesso": False, "mensagem": "Nome nao fornecido"}), 400

    eliminado = False
    # Procura pelas extensões de imagem mais comuns (.jpg, .jpeg, .png)
    for ext in [".jpg", ".jpeg", ".png"]:
        filepath = os.path.join(KNOWN_FACES_DIR, f"{nome}{ext}")
        if os.path.exists(filepath):
            try:
                os.remove(filepath)
                eliminado = True
                print(f"[SISTEMA] Arquivo {filepath} deletado com sucesso.")
                break
            except Exception as e:
                return jsonify({"sucesso": False, "mensagem": f"Erro ao deletar arquivo físico: {str(e)}"}), 500

    if eliminado:
        # CRUCIAL: Limpa as listas da memória RAM e recarrega os rostos restantes
        global known_faces, known_names
        print(f"[SISTEMA] Sincronizando e recarregando rostos conhecidos na RAM...")
        known_faces = []
        known_names = []
        
        for filename in os.listdir(KNOWN_FACES_DIR):
            if filename.startswith('.'):
                continue
            filepath = os.path.join(KNOWN_FACES_DIR, filename)
            if os.path.isfile(filepath):
                try:
                    image = face_recognition.load_image_file(filepath)
                    encodings = face_recognition.face_encodings(image)
                    if len(encodings) > 0:
                        known_faces.append(encodings[0])
                        known_names.append(os.path.splitext(filename)[0])
                except Exception as e:
                    print(f"Erro ao recarregar {filename}: {str(e)}")
                    
        return jsonify({"sucesso": True, "mensagem": f"Rosto de {nome} removido completamente."}), 200

    return jsonify({"sucesso": False, "mensagem": "Imagem do usuario nao encontrada na pasta"}), 404

@app.route("/cancelar-operacao", methods=["POST"])
def cancelar_operacao():
    global current_state
    print("\n[HTTP] Operação cancelada pelo ESP32. Voltando para modo GESTOS.")
    current_state = STATE_GESTURE
    return jsonify({"status": "sucesso", "mensagem": "Voltou para modo gestos"}), 200

if __name__ == "__main__":
    # Inicia a thread de gestos antes do Flask rodar
    t = threading.Thread(target=loop_reconhecimento_gestos, daemon=True)
    t.start()
    
    # Roda o Flask escutando no IP configurado
    app.run(host="192.168.4.2", port=5000)