import face_recognition
import os
import cv2
import numpy as np

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

print("iniciando a webcam...")
# aqui precisamos conectar com o celular de algum jeito
# inicia a captura de vídeo (0 é a webcam padrão do notebook)
video_capture = cv2.VideoCapture(0)

while True:
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
        match = "Desconhecido" # Nome padrão se não encontrar ninguém
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
        # nessa parte iremos conectar com o esp pra mandar o sinal ou algo assim

    # mostra a imagem resultante na tela
    cv2.imshow('Reconhecimento Facial (Pressione Q para sair)', frame)

    # aperte a tecla 'q' no teclado para encerrar o programa
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# libera a câmera e fecha as janelas
video_capture.release()
cv2.destroyAllWindows()