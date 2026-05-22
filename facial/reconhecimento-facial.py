from flask import Flask, jsonify
import face_recognition
import os
import cv2
import numpy as np
import time

#TO DO: VER MAIN.CPP 

KNOWN_FACES_DIR = "known_faces"
TOLERANCE = 0.6
FRAME_THICKNESS = 3
FONT_THICKNESS = 2
MODEL = "hog" 

print("carregando rostos conhecidos...")

known_faces = []
known_names = []

# carrega as imagens da pasta
for filename in os.listdir(KNOWN_FACES_DIR):
    # ignora arquivos ocultos ou de sistema
    if filename.startswith('.'):
        continue
        
    filepath = f"{KNOWN_FACES_DIR}/{filename}"
    
    # verifica se é realmente um arquivo antes de tentar abrir
    if os.path.isfile(filepath):
        image = face_recognition.load_image_file(filepath)
        
        # garante que um rosto foi encontrado
        encodings = face_recognition.face_encodings(image)
        if len(encodings) > 0:
            encoding = encodings[0]
            known_faces.append(encoding)
            
            # pega o nome do arquivo sem a extensão
            name = os.path.splitext(filename)[0]
            
            known_names.append(name)
            print(f"rosto de {name} carregado.")

def executarReconhecimento():
    print("iniciando a webcam para o ESP32")
    video_capture = cv2.VideoCapture(0)

    match = "Desconhecido" # Nome padrão se não encontrar ninguém

    timeout = time.time() + 45 #Defini um tempo limite para evitar de crashar o programa

    while time.time()<timeout:
        ret, frame = video_capture.read()
        
        if not ret:
            print("não foi possível acessar a câmera.")
            break

        # converte do opencv pro face-recognition
        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

        # encontra todos os rostos e suas codificações no frame atual
        locations = face_recognition.face_locations(rgb_frame, model=MODEL)
        encodings = face_recognition.face_encodings(rgb_frame, locations)

        for face_encoding, face_location in zip(encodings, locations):
            # compara o rosto encontrado com os rostos conhecidos
            results = face_recognition.compare_faces(known_faces, face_encoding, TOLERANCE)
            color = [0, 0, 255]    # Vermelho para desconhecidos

            # usa a distância para encontrar a pessoa mais parecida, 
            # em vez de pegar apenas a primeira que der true
            face_distances = face_recognition.face_distance(known_faces, face_encoding)
            if len(face_distances) > 0:
                best_match_index = np.argmin(face_distances)
                if results[best_match_index]:
                    match = known_names[best_match_index]
                    color = [0, 255, 0] # Verde para conhecidos
                    print(f"Rosto reconhecido: {match}")

            # extrai as coordenadas do rosto
            top, right, bottom, left = face_location

            # desenha o retângulo ao redor do rosto
            cv2.rectangle(frame, (left, top), (right, bottom), color, FRAME_THICKNESS)

            # desenha o fundo do texto
            cv2.rectangle(frame, (left, bottom - 25), (right, bottom), color, cv2.FILLED)
            
            # escreve o nome da pessoa
            cv2.putText(frame, match, (left + 6, bottom - 6), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), FONT_THICKNESS)

        #encerra o loop de processamento se encontrar o rosto (tirar caso de problema) 
        if match != "Desconhecido":
            cv2.waitKey(500) 
            break
        
        #apertar q para sair do reconhecimento (caso de muito errado)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
        


    # libera a câmera e fecha as janelas
    video_capture.release()
    cv2.destroyAllWindows()

    return match

#SALVAR FOTO NO NOTEBOOK
#def salvarFoto()
    #TO DO

#ROTA QUE O ESP VAI CHAMAR
@app.route("/get-usuario", methods=["POST"])
def disparar_camera():
    print("\n[HTTP] Requisição recebida do ESP32!")
    
    # Chama a função que abre a webcam e processa
    usuario = executarReconhecimento()
    
    # Retorna o JSON exatamente como o ESP32 espera receber no main.cpp
    return jsonify({"usuario": usuario})


if __name__ == "__main__":
    # Roda o servidor Flask. O host="0.0.0.0" permite que o ESP32 encontre o PC na rede local
    app.run(host="0.0.0.0", port=5000)