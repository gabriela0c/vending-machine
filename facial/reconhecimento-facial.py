from flask import Flask, jsonify, request  # <-- ADICIONADO 'request' AQUI
import face_recognition
import os
import cv2
import numpy as np
import time

app = Flask(__name__)

KNOWN_FACES_DIR = "known_faces"
TOLERANCE = 0.6
FRAME_THICKNESS = 3
FONT_THICKNESS = 2
MODEL = "hog" 

print("carregando rostos conhecidos...")

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
            print(f"rosto de {name} carregado.")

def executarReconhecimento():
    print("iniciando a webcam para o ESP32")
    video_capture = cv2.VideoCapture(0)
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

        cv2.imshow("Reconhecimento Facial", frame)
        if match != "Desconhecido" or cv2.waitKey(1) & 0xFF == ord('q'):
            break
        
    video_capture.release()
    cv2.destroyAllWindows()
    return match

# ==================== NOVA FUNÇÃO DE CADASTRO ====================
def executarCadastro(nome):
    print(f"Aguardando posicionamento do rosto para: {nome}")
    video_capture = cv2.VideoCapture(0)
    timeout = time.time() + 15  # Dá 15 segundos para o usuário se posicionar na câmera
    sucesso = False

    while time.time() < timeout:
        ret, frame = video_capture.read()
        if not ret:
            break

        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        locations = face_recognition.face_locations(rgb_frame, model=MODEL)
        
        # Cria uma cópia do frame para desenhar elementos visuais de instrução
        frame_instrucao = frame.copy()

        if len(locations) == 1:
            # Encontrou exatamente uma pessoa. Salva e processa!
            top, right, bottom, left = locations[0]
            cv2.rectangle(frame_instrucao, (left, top), (right, bottom), (0, 255, 0), FRAME_THICKNESS)
            cv2.putText(frame_instrucao, "Rosto Detectado! Capturando...", (10, 30), 
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), FONT_THICKNESS)
            cv2.imshow("Cadastro de Face", frame_instrucao)
            cv2.waitKey(1000) # Mantém a tela travada em verde por 1 segundo

            # Salva o frame original limpo (sem retângulos desenhados)
            filepath = os.path.join(KNOWN_FACES_DIR, f"{nome}.jpg")
            cv2.imwrite(filepath, frame)

            # Atualiza a memória RAM do script em tempo real (sem precisar reiniciar o Python)
            image = face_recognition.load_image_file(filepath)
            encodings = face_recognition.face_encodings(image)
            if len(encodings) > 0:
                known_faces.append(encodings[0])
                known_names.append(nome)
                print(f"Rosto de {nome} adicionado e indexado com sucesso.")
                sucesso = True
                break
        elif len(locations) > 1:
            cv2.putText(frame_instrucao, "Erro: Mais de uma pessoa na camera!", (10, 30), 
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), FONT_THICKNESS)
        else:
            cv2.putText(frame_instrucao, "Olhe fixamente para a camera...", (10, 30), 
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), FONT_THICKNESS)

        cv2.imshow("Cadastro de Face", frame_instrucao)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    video_capture.release()
    cv2.destroyAllWindows()
    return sucesso

@app.route("/get-usuario", methods=["POST"])
def disparar_camera():
    print("\n[HTTP] Requisição de RECONHECIMENTO recebida!")
    usuario = executarReconhecimento()
    return jsonify({"usuario": usuario})

# ==================== NOVA ROTA DE CADASTRO ====================
@app.route("/cadastrar-rosto", methods=["POST"])
def cadastrar_rosto():
    print("\n[HTTP] Requisição de CADASTRO recebida!")
    dados = request.get_json()
    nome = dados.get("nome")
    
    if not nome:
        return jsonify({"status": "erro", "mensagem": "Nome nao enviado"}), 400
        
    if executarCadastro(nome):
        return jsonify({"status": "sucesso", "mensagem": f"Rosto de {nome} cadastrado!"}), 200
    else:
        return jsonify({"status": "erro", "mensagem": "Nao foi possivel detectar o rosto ou deu Timeout"}), 500

if __name__ == "__main__":
    app.run(host="192.168.4.2", port=5000)