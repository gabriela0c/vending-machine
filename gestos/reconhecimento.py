import cv2
import mediapipe as mp

# --- Inicialização da Visão Computacional ---
mp_maos = mp.solutions.hands
maos = mp_maos.Hands(max_num_hands=1, min_detection_confidence=0.7)
mp_desenho = mp.solutions.drawing_utils
camera = cv2.VideoCapture(0)

# Variável de estado para não "floodar" o terminal
produto_anterior = 0 

print("Sistema iniciado. Mostre de 1 a 4 dedos na câmera.")
print("Pressione 'q' na janela do vídeo para encerrar.")

while True:
    sucesso, frame = camera.read()
    if not sucesso:
        break

    # Converte as cores para o MediaPipe ler
    frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    resultados = maos.process(frame_rgb)

    # Se achar alguma mão...
    if resultados.multi_hand_landmarks:
        for mao_pontos in resultados.multi_hand_landmarks:
            mp_desenho.draw_landmarks(frame, mao_pontos, mp_maos.HAND_CONNECTIONS)
            
            dedos_levantados = 0
            
            # IDs das Pontas e Juntas (Indicador, Médio, Anelar e Mindinho)
            pontas = [8, 12, 16, 20]
            juntas = [6, 10, 14, 18]

            # O Segredo da Matemática:
            # Em imagens, o eixo Y = 0 fica no topo da tela. 
            # Então, se o Y da ponta for MENOR que o Y da junta, 
            # significa que a ponta está mais perto do teto (dedo esticado).
            for i in range(4):
                if mao_pontos.landmark[pontas[i]].y < mao_pontos.landmark[juntas[i]].y:
                    dedos_levantados += 1

            # --- A Lógica do Vending Machine ---
            if 1 <= dedos_levantados <= 4:
                produto_escolhido = dedos_levantados
                
                # Só imprime no terminal se for um gesto NOVO, 
                # igual faríamos com uma flag de estado em C++
                if produto_escolhido != produto_anterior:
                    print(f"-> Produto {produto_escolhido} escolhido!")
                    produto_anterior = produto_escolhido
                
                # Feedback visual na tela
                cv2.putText(frame, f"Produto: {produto_escolhido}", (50, 80), 
                            cv2.FONT_HERSHEY_SIMPLEX, 1.5, (0, 255, 0), 3)
            
            elif dedos_levantados == 0:
                # Se fechar a mão inteira, reseta o estado
                produto_anterior = 0
                cv2.putText(frame, "Aguardando...", (50, 80), 
                            cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2)

    cv2.imshow("Vending Machine - Camera", frame)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

camera.release()
cv2.destroyAllWindows()