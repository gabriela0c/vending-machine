import cv2
import mediapipe as mp

mp_maos = mp.solutions.hands
maos = mp_maos.Hands(max_num_hands=1, min_detection_confidence=0.7)
mp_desenho = mp.solutions.drawing_utils
camera = cv2.VideoCapture(0)

# variável de estado para não "floodar" o terminal
produto_anterior = 0 

print("Sistema iniciado. Mostre de 1 a 4 dedos na câmera.")
print("Pressione 'q' na janela do vídeo para encerrar.")

while True:
    sucesso, frame = camera.read()
    if not sucesso:
        break

    # converte as cores para o mediapipe ler
    frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    resultados = maos.process(frame_rgb)

    if resultados.multi_hand_landmarks:
        for mao_pontos in resultados.multi_hand_landmarks:
            mp_desenho.draw_landmarks(frame, mao_pontos, mp_maos.HAND_CONNECTIONS)
            
            dedos_levantados = 0
            
            # ids das pontas e juntas
            pontas = [8, 12, 16, 20]
            juntas = [6, 10, 14, 18]

            # nas imagens, o eixo y = 0 fica no topo da tela, então se o y da ponta for 
            # menor que o y da junta a ponta está mais perto do teto (dedo esticado).
            for i in range(4):
                if mao_pontos.landmark[pontas[i]].y < mao_pontos.landmark[juntas[i]].y:
                    dedos_levantados += 1

            if 1 <= dedos_levantados <= 4:
                produto_escolhido = dedos_levantados
                
                # só imprime no terminal se for um gesto novo
                if produto_escolhido != produto_anterior:
                    print(f"-> Produto {produto_escolhido} escolhido!")
                    produto_anterior = produto_escolhido
                
                # feedback visual na tela (conectar com o esp depois)
                cv2.putText(frame, f"Produto: {produto_escolhido}", (50, 80), 
                            cv2.FONT_HERSHEY_SIMPLEX, 1.5, (0, 255, 0), 3)
            
            elif dedos_levantados == 0:
                # se fechar a mão inteira, reseta o estado
                produto_anterior = 0
                cv2.putText(frame, "Aguardando...", (50, 80), 
                            cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2)

    cv2.imshow("Vending Machine - Camera", frame)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

camera.release()
cv2.destroyAllWindows()