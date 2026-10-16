#include "video_options_text.h"
#include "game_addresses.h"
#include "support.h"

static const VideoOptionsText english = {
    .camera = "Drift Camera",
    .driver = "Driver",
    .parameters = {"Sensitivity", "Look Speed", "Range"},
    .descriptions = {"Enable, make it only active in the first-person views, or disable the "
                     "drift camera.",
                     "The ratio of car angle to camera angle. Increase this setting to "
                     "increase how sensitive the "
                     "camera is to car angle.",
                     "The speed at which the camera moves from its original position towards "
                     "the direction of "
                     "travel. Reducing this setting will smooth out the camera's movement due "
                     "to changes in "
                     "direction.",
                     "Reduce this setting to limit the camera's movement when the car is at "
                     "peak angle."},
    .choices = {L"On", L"Driver", L"Off"},
    .fov_views = {"Chase", "Far Chase", "Driver", "Hood", "Bumper"},
    .fov_description = "Set the vertical field of view for the given camera in degrees.",
    .settings_names = {"Real-time Shadows", "", "Photo Mode Samples", "", "Minimap Orientation",
                       "Autodrive Cinematic Camera", "Skip Championship Dialogue", "Spawn Location",
                       "Change Car Position"},
    .settings_descriptions =
        {"Update sun shadows every frame. Increases GPU load. Restart required.", "",
         "Set the photo mode samples for depth of field and motion blur. Default "
         "uses 129 samples. "
         "Higher values increase capture time. Restart required.",
         "", "Choose whether the minimap follows the car or the camera.",
         "Use cinematic cameras during Autodrive.",
         "Skip the forced dialogue when entering a championship race in an "
         "ineligible car. Restart "
         "required.",
         "Choose the spawn location used when booting the game. Default spawns you "
         "at the nearest car "
         "meet relative to when you closed the game as usual. A game restart is "
         "needed to apply "
         "changes.",
         "Where the new car should be when you change cars in the open world. The "
         "latter option will "
         "leave you exactly where the old car was."},
    .default_choice = L"Default",
    .minimap_choices = {L"Follow Car", L"Follow Camera"},
    .spawn_choices = {L"Car Meet Entrance", L"Festival Site"},
    .car_position_choices = {L"Nearest Road", L"Same Location"}};

static const VideoOptionsText brazilian = {
    .camera = "Câmera de Drift",
    .driver = "Piloto",
    .parameters = {"Sensibilidade", "Velocidade de movimento", "Alcance"},
    .descriptions = {"Ative a câmera de drift, deixe-a ativa apenas nas visões em primeira "
                     "pessoa ou desative-a.",
                     "A proporção entre o ângulo do carro e o ângulo da câmera. Aumente esta "
                     "configuração para "
                     "tornar a câmera mais sensível ao ângulo do carro.",
                     "A velocidade com que a câmera se move da posição original em direção ao "
                     "movimento do carro. "
                     "Reduzir esta configuração suaviza o movimento da câmera nas mudanças de "
                     "direção.",
                     "Reduza esta configuração para limitar o movimento da câmera quando o "
                     "carro estiver no ângulo "
                     "máximo."},
    .choices = {L"Ligado", L"Piloto", L"Desligado"},
    .fov_views = {"Perseguição", "Perseguição distante", "Piloto", "Capô", "Para-choque"},
    .fov_description = "Defina o campo de visão vertical da câmera selecionada em graus.",
    .settings_names = {"Sombras em tempo real", "", "Amostras do modo foto", "",
                       "Orientação do minimapa", "Câmera cinematográfica do Autodrive",
                       "Ignorar diálogo de campeonato", "Local de início",
                       "Posição ao trocar de carro"},
    .settings_descriptions =
        {"Atualiza as sombras do sol a cada quadro. Aumenta a carga da GPU. "
         "Reinicie o jogo para "
         "aplicar.",
         "",
         "Define as amostras do modo foto para profundidade de campo e desfoque de "
         "movimento. Padrão "
         "usa 129 amostras. Valores maiores aumentam o tempo de captura. Reinicie "
         "o jogo para aplicar.",
         "", "Escolhe se o minimapa acompanha o carro ou a câmera.",
         "Usa câmeras cinematográficas durante o Autodrive.",
         "Ignora o diálogo obrigatório ao entrar em uma corrida de campeonato com "
         "um carro inelegível. "
         "Reinicie o jogo para aplicar.",
         "Escolha o local onde você começa ao iniciar o jogo. Padrão coloca você "
         "no encontro de carros "
         "mais próximo de onde estava ao fechar o jogo, como de costume. Reinicie "
         "o jogo para aplicar "
         "as alterações.",
         "Onde o novo carro deve ficar quando você troca de carro no mundo aberto. "
         "A segunda opção "
         "deixa você exatamente onde o carro anterior estava."},
    .default_choice = L"Padrão",
    .minimap_choices = {L"Seguir carro", L"Seguir câmera"},
    .spawn_choices = {L"Entrada do encontro de carros", L"Local do festival"},
    .car_position_choices = {L"Estrada mais próxima", L"Mesmo local"}};

static const VideoOptionsText german = {
    .camera = "Driftkamera",
    .driver = "Fahrer",
    .parameters = {"Empfindlichkeit", "Blickgeschwindigkeit", "Bereich"},
    .descriptions = {"Aktiviere die Driftkamera, beschränke sie auf die Ego-Perspektiven oder "
                     "deaktiviere sie.",
                     "Das Verhältnis zwischen Fahrzeugwinkel und Kamerawinkel. Erhöhe diese "
                     "Einstellung, damit die "
                     "Kamera empfindlicher auf den Fahrzeugwinkel reagiert.",
                     "Die Geschwindigkeit, mit der sich die Kamera aus ihrer Ausgangsposition "
                     "in die "
                     "Bewegungsrichtung dreht. Eine niedrigere Einstellung sorgt bei "
                     "Richtungswechseln für "
                     "sanftere Kamerabewegungen.",
                     "Verringere diese Einstellung, um die Kamerabewegung beim maximalen "
                     "Fahrzeugwinkel zu "
                     "begrenzen."},
    .choices = {L"Ein", L"Fahrer", L"Aus"},
    .fov_views = {"Verfolger", "Weiter Verfolger", "Fahrer", "Motorhaube", "Stoßstange"},
    .fov_description = "Stelle das vertikale Sichtfeld der jeweiligen Kamera in Grad ein.",
    .settings_names = {"Echtzeitschatten", "", "Fotomodus-Samples", "", "Minikartenausrichtung",
                       "Autodrive-Filmkamera", "Meisterschaftsdialog überspringen", "Startposition",
                       "Position beim Autowechsel"},
    .settings_descriptions =
        {"Aktualisiert Sonnenschatten in jedem Bild. Erhöht die GPU-Last. Neustart "
         "erforderlich.",
         "",
         "Legt die Fotomodus-Samples für Schärfentiefe und Bewegungsunschärfe "
         "fest. Standard verwendet "
         "129 Samples. Höhere Werte verlängern die Aufnahme. Neustart "
         "erforderlich.",
         "", "Wähle, ob sich die Minikarte nach dem Auto oder der Kamera richtet.",
         "Verwendet Filmkameras während Autodrive.",
         "Überspringt den erzwungenen Dialog bei Meisterschaftsrennen mit einem "
         "ungeeigneten Auto. "
         "Neustart erforderlich.",
         "Wähle die Startposition beim Laden des Spiels. Standard startet wie "
         "gewohnt beim "
         "Autotreffen, das deiner Position beim Beenden des Spiels am nächsten "
         "liegt. Änderungen "
         "erfordern einen Spielneustart.",
         "Wo das neue Auto nach einem Fahrzeugwechsel in der offenen Welt stehen "
         "soll. Die zweite "
         "Option lässt dich genau an der Position des vorherigen Autos."},
    .default_choice = L"Standard",
    .minimap_choices = {L"Auto folgen", L"Kamera folgen"},
    .spawn_choices = {L"Einfahrt zum Autotreffen", L"Festivalgelände"},
    .car_position_choices = {L"Nächste Straße", L"Gleiche Position"}};

static const VideoOptionsText french = {
    .camera = "Caméra de drift",
    .driver = "Pilote",
    .parameters = {"Sensibilité", "Vitesse de rotation", "Amplitude"},
    .descriptions = {"Activez la caméra de drift, limitez-la aux vues à la première personne "
                     "ou désactivez-la.",
                     "Le rapport entre l'angle de la voiture et celui de la caméra. Augmentez "
                     "ce réglage pour "
                     "rendre la caméra plus sensible à l'angle de la voiture.",
                     "La vitesse à laquelle la caméra quitte sa position initiale pour suivre "
                     "la direction du "
                     "déplacement. Réduire ce réglage adoucit les mouvements de caméra lors "
                     "des changements de "
                     "direction.",
                     "Réduisez ce réglage pour limiter le mouvement de la caméra lorsque la "
                     "voiture atteint son "
                     "angle maximal."},
    .choices = {L"Oui", L"Pilote", L"Non"},
    .fov_views = {"Poursuite", "Poursuite éloignée", "Pilote", "Capot", "Pare-chocs"},
    .fov_description = "Réglez le champ de vision vertical de la caméra concernée en degrés.",
    .settings_names = {"Ombres en temps réel", "", "Échantillons du mode photo", "",
                       "Orientation de la mini-carte", "Caméra cinématique Autodrive",
                       "Ignorer le dialogue de championnat", "Lieu d’apparition",
                       "Position au changement de voiture"},
    .settings_descriptions =
        {"Actualise les ombres du soleil à chaque image. Augmente la charge du "
         "GPU. Redémarrage "
         "requis.",
         "",
         "Définissez les échantillons du mode photo pour la profondeur de champ et "
         "le flou de "
         "mouvement. Par défaut : 129 échantillons. Une valeur élevée prolonge la "
         "capture. Redémarrage "
         "requis.",
         "", "Choisissez si la mini-carte suit la voiture ou la caméra.",
         "Utilise des caméras cinématiques pendant Autodrive.",
         "Ignore le dialogue obligatoire en entrant dans une course de championnat "
         "avec une voiture "
         "non admissible. Redémarrage requis.",
         "Choisissez le lieu d’apparition au démarrage du jeu. Par défaut, vous "
         "apparaissez comme "
         "d’habitude au rassemblement le plus proche de votre position à la "
         "fermeture du jeu. "
         "Redémarrez le jeu pour appliquer les modifications.",
         "L’endroit où placer la nouvelle voiture lors d’un changement de voiture "
         "dans le monde "
         "ouvert. La seconde option vous laisse exactement à l’emplacement de "
         "l’ancienne voiture."},
    .default_choice = L"Par défaut",
    .minimap_choices = {L"Suivre la voiture", L"Suivre la caméra"},
    .spawn_choices = {L"Entrée du rassemblement", L"Site du festival"},
    .car_position_choices = {L"Route la plus proche", L"Même emplacement"}};

static const VideoOptionsText spanish = {
    .camera = "Cámara de derrape",
    .driver = "Piloto",
    .parameters = {"Sensibilidad", "Velocidad de giro", "Rango"},
    .descriptions = {"Activa la cámara de derrape, haz que solo funcione en las vistas en "
                     "primera persona o "
                     "desactívala.",
                     "La proporción entre el ángulo del coche y el de la cámara. Aumenta este "
                     "ajuste para que la "
                     "cámara sea más sensible al ángulo del coche.",
                     "La velocidad a la que la cámara se mueve desde su posición original "
                     "hacia la dirección de "
                     "desplazamiento. Reducir este ajuste suaviza el movimiento de la cámara "
                     "durante los cambios "
                     "de dirección.",
                     "Reduce este ajuste para limitar el movimiento de la cámara cuando el "
                     "coche alcance su ángulo "
                     "máximo."},
    .choices = {L"Sí", L"Piloto", L"No"},
    .fov_views = {"Seguimiento", "Seguimiento lejano", "Piloto", "Capó", "Parachoques"},
    .fov_description = "Establece el campo de visión vertical de la cámara correspondiente en "
                       "grados.",
    .settings_names = {"Sombras en tiempo real", "", "Muestras del modo Foto", "",
                       "Orientación del minimapa", "Cámara cinemática de Autodrive",
                       "Omitir diálogo de campeonato", "Ubicación inicial",
                       "Posición al cambiar de coche"},
    .settings_descriptions =
        {"Actualiza las sombras del sol en cada fotograma. Aumenta la carga de la "
         "GPU. Requiere "
         "reiniciar.",
         "",
         "Ajusta las muestras del modo foto para profundidad de campo y desenfoque "
         "de movimiento. "
         "Predeterminado usa 129 muestras. Los valores altos aumentan el tiempo de "
         "captura. Requiere "
         "reiniciar.",
         "", "Elige si el minimapa sigue al coche o a la cámara.",
         "Usa cámaras cinemáticas durante Autodrive.",
         "Omite el diálogo obligatorio al entrar en una carrera de campeonato con "
         "un coche no apto. "
         "Requiere reiniciar.",
         "Elige dónde aparecer al iniciar el juego. Predeterminado te sitúa, como "
         "siempre, en la "
         "reunión de coches más cercana a tu posición al cerrar el juego. Reinicia "
         "el juego para "
         "aplicar los cambios.",
         "Dónde debe estar el nuevo coche al cambiar de coche en el mundo abierto. "
         "La segunda opción "
         "te deja exactamente donde estaba el coche anterior."},
    .default_choice = L"Predeterminado",
    .minimap_choices = {L"Seguir coche", L"Seguir cámara"},
    .spawn_choices = {L"Entrada de la reunión de coches", L"Recinto del festival"},
    .car_position_choices = {L"Carretera más cercana", L"Misma ubicación"}};

static const VideoOptionsText mexican = {
    .camera = "Cámara de derrape",
    .driver = "Piloto",
    .parameters = {"Sensibilidad", "Velocidad de giro", "Rango"},
    .descriptions = {"Activa la cámara de derrape, haz que solo funcione en las vistas en "
                     "primera persona o "
                     "desactívala.",
                     "La proporción entre el ángulo del auto y el de la cámara. Aumenta este "
                     "ajuste para que la "
                     "cámara sea más sensible al ángulo del auto.",
                     "La velocidad a la que la cámara se mueve desde su posición original "
                     "hacia la dirección de "
                     "desplazamiento. Reducir este ajuste suaviza el movimiento de la cámara "
                     "durante los cambios "
                     "de dirección.",
                     "Reduce este ajuste para limitar el movimiento de la cámara cuando el "
                     "auto alcance su ángulo "
                     "máximo."},
    .choices = {L"Sí", L"Piloto", L"No"},
    .fov_views = {"Seguimiento", "Seguimiento lejano", "Piloto", "Capó", "Parachoques"},
    .fov_description = "Establece el campo de visión vertical de la cámara correspondiente en "
                       "grados.",
    .settings_names = {"Sombras en tiempo real", "", "Muestras del modo Foto", "",
                       "Orientación del minimapa", "Cámara cinemática de Autodrive",
                       "Omitir diálogo de campeonato", "Ubicación inicial",
                       "Posición al cambiar de auto"},
    .settings_descriptions =
        {"Actualiza las sombras del sol en cada cuadro. Aumenta la carga de la "
         "GPU. Requiere "
         "reiniciar.",
         "",
         "Ajusta las muestras del modo foto para profundidad de campo y desenfoque "
         "de movimiento. "
         "Predeterminado usa 129 muestras. Los valores altos aumentan el tiempo de "
         "captura. Requiere "
         "reiniciar.",
         "", "Elige si el minimapa sigue al auto o a la cámara.",
         "Usa cámaras cinemáticas durante Autodrive.",
         "Omite el diálogo obligatorio al entrar en una carrera de campeonato con "
         "un auto no apto. "
         "Requiere reiniciar.",
         "Elige dónde aparecer al iniciar el juego. Predeterminado te sitúa, como "
         "siempre, en la "
         "reunión de autos más cercana a tu posición al cerrar el juego. Reinicia "
         "el juego para "
         "aplicar los cambios.",
         "Dónde debe estar el nuevo auto al cambiar de auto en el mundo abierto. "
         "La segunda opción te "
         "deja exactamente donde estaba el auto anterior."},
    .default_choice = L"Predeterminado",
    .minimap_choices = {L"Seguir auto", L"Seguir cámara"},
    .spawn_choices = {L"Entrada de la reunión de autos", L"Recinto del festival"},
    .car_position_choices = {L"Carretera más cercana", L"Misma ubicación"}};

static const VideoOptionsText italian = {
    .camera = "Telecamera di derapata",
    .driver = "Pilota",
    .parameters = {"Sensibilità", "Velocità di rotazione", "Ampiezza"},
    .descriptions = {"Attiva la telecamera di derapata, rendila attiva solo nelle visuali in "
                     "prima persona oppure "
                     "disattivala.",
                     "Il rapporto tra l'angolo dell'auto e quello della telecamera. Aumenta "
                     "questa impostazione "
                     "per rendere la telecamera più sensibile all'angolo dell'auto.",
                     "La velocità con cui la telecamera si sposta dalla posizione originale "
                     "verso la direzione di "
                     "movimento. Ridurre questa impostazione rende più fluidi i movimenti "
                     "della telecamera durante "
                     "i cambi di direzione.",
                     "Riduci questa impostazione per limitare il movimento della telecamera "
                     "quando l'auto "
                     "raggiunge l'angolo massimo."},
    .choices = {L"Sì", L"Pilota", L"No"},
    .fov_views = {"Inseguimento", "Inseguimento distante", "Pilota", "Cofano", "Paraurti"},
    .fov_description = "Imposta il campo visivo verticale della telecamera selezionata in gradi.",
    .settings_names = {"Ombre in tempo reale", "", "Campioni modalità Foto", "",
                       "Orientamento minimappa", "Telecamera cinematografica Autodrive",
                       "Salta dialogo campionato", "Posizione iniziale",
                       "Posizione al cambio auto"},
    .settings_descriptions =
        {"Aggiorna le ombre del sole a ogni fotogramma. Aumenta il carico della "
         "GPU. Riavvio "
         "richiesto.",
         "",
         "Imposta i campioni della modalità foto per profondità di campo e "
         "sfocatura di movimento. "
         "Predefinito usa 129 campioni. Valori maggiori aumentano il tempo di "
         "cattura. Riavvio "
         "richiesto.",
         "", "Scegli se la minimappa segue l’auto o la telecamera.",
         "Usa telecamere cinematografiche durante Autodrive.",
         "Salta il dialogo obbligatorio entrando in una gara di campionato con "
         "un’auto non idonea. "
         "Riavvio richiesto.",
         "Scegli la posizione iniziale all’avvio del gioco. Predefinito ti colloca "
         "come al solito al "
         "raduno più vicino alla tua posizione alla chiusura del gioco. Riavvia il "
         "gioco per applicare "
         "le modifiche.",
         "Dove posizionare la nuova auto quando cambi auto nel mondo aperto. La "
         "seconda opzione ti "
         "lascia esattamente nella posizione dell’auto precedente."},
    .default_choice = L"Predefinito",
    .minimap_choices = {L"Segui auto", L"Segui telecamera"},
    .spawn_choices = {L"Ingresso del raduno", L"Area del festival"},
    .car_position_choices = {L"Strada più vicina", L"Stessa posizione"}};

static const VideoOptionsText japanese = {
    .camera = "ドリフトカメラ",
    .driver = "ドライバー",
    .parameters = {"感度", "視点移動速度", "可動範囲"},
    .descriptions = {"ドリフトカメラを有効にする、一人称視点でのみ有効にする、または無効にしま"
                     "す。",
                     "車の角度とカメラの角度の比率です。この設定を上げると、車の角度に対するカ"
                     "メラの感度が高くなり"
                     "ます。",
                     "カメラが元の位置から車の移動方向へ向く速度です。この設定を下げると、方向"
                     "転換時のカメラの動き"
                     "が滑らかになります。",
                     "この設定を下げると、車が最大角度に達したときのカメラの動きを制限できます"
                     "。"},
    .choices = {L"ON", L"ドライバー", L"OFF"},
    .fov_views = {"チェイス", "遠距離チェイス", "ドライバー", "ボンネット", "バンパー"},
    .fov_description = "選択したカメラの垂直視野角を度単位で設定します。",
    .settings_names = {"リアルタイムの影", "", "フォトモードのサンプル数", "", "ミニマップの向き",
                       "Autodriveのシネマティックカメラ", "チャンピオンシップの会話をスキップ",
                       "開始地点", "車の変更時の位置"},
    .settings_descriptions =
        {"太陽の影を毎フレーム更新します。GPU負荷が増加します。再起動が必要です。", "",
         "フォトモードの被写界深度とモーションブラーのサンプル数を設定します。標準"
         "は129サンプルです。値"
         "を上げると撮影時間が長くなります。再起動が必要です。",
         "", "ミニマップの向きを車またはカメラに合わせます。",
         "Autodrive中にシネマティックカメラを使用します。",
         "参加条件を満たさない車でチャンピオンシップに入る際の強制会話をスキップし"
         "ます。再起動が必要で"
         "す。",
         "ゲーム起動時の開始地点を選択します。デフォルトでは、通常どおりゲーム終了"
         "時の位置に最も近いカ"
         "ーミーティングから開始します。変更を適用するにはゲームの再起動が必要です"
         "。",
         "オープンワールドで車を変更したときの、新しい車の位置を設定します。後者を"
         "選ぶと、前の車があっ"
         "た場所にそのまま留まります。"},
    .default_choice = L"デフォルト",
    .minimap_choices = {L"車に合わせる", L"カメラに合わせる"},
    .spawn_choices = {L"カーミーティング入口", L"フェスティバル会場"},
    .car_position_choices = {L"最寄りの道路", L"同じ位置"}};

static const VideoOptionsText korean = {
    .camera = "드리프트 카메라",
    .driver = "운전자",
    .parameters = {"민감도", "시점 이동 속도", "범위"},
    .descriptions = {"드리프트 카메라를 켜거나, 1인칭 시점에서만 작동하게 하거나, 끕니다.",
                     "차량 각도와 카메라 각도의 비율입니다. 이 설정을 높이면 차량 각도에 대한 "
                     "카메라의 민감도가 "
                     "높아집니다.",
                     "카메라가 원래 위치에서 차량의 이동 방향으로 움직이는 속도입니다. 이 "
                     "설정을 낮추면 방향이 "
                     "바뀔 때 카메라가 더 부드럽게 움직입니다.",
                     "이 설정을 낮추면 차량이 최대 각도에 도달했을 때 카메라의 움직임을 제한할 "
                     "수 있습니다."},
    .choices = {L"켜짐", L"운전자", L"꺼짐"},
    .fov_views = {"추적", "원거리 추적", "운전자", "후드", "범퍼"},
    .fov_description = "해당 카메라의 수직 시야각을 도 단위로 설정합니다.",
    .settings_names = {"실시간 그림자", "", "사진 모드 샘플 수", "", "미니맵 방향",
                       "Autodrive 시네마틱 카메라", "챔피언십 대화 건너뛰기", "시작 위치",
                       "차량 변경 시 위치"},
    .settings_descriptions =
        {"매 프레임 태양 그림자를 갱신합니다. GPU 부하가 증가합니다. 재시작이 "
         "필요합니다.",
         "",
         "사진 모드의 피사계 심도와 모션 블러 샘플 수를 설정합니다. 기본값은 "
         "129개입니다. 값이 "
         "높을수록 촬영 시간이 늘어납니다. 재시작이 필요합니다.",
         "", "미니맵이 차량 또는 카메라 방향을 따르도록 설정합니다.",
         "Autodrive 중 시네마틱 카메라를 사용합니다.",
         "참가 조건에 맞지 않는 차량으로 챔피언십에 진입할 때의 강제 대화를 "
         "건너뜁니다. 재시작이 "
         "필요합니다.",
         "게임을 시작할 때의 위치를 선택합니다. 기본값은 평소처럼 게임 종료 시 "
         "위치에서 가장 가까운 "
         "자동차 모임에서 시작합니다. 변경 사항을 적용하려면 게임을 다시 시작해야 "
         "합니다.",
         "오픈 월드에서 차량을 변경할 때 새 차량을 배치할 위치입니다. 두 번째 "
         "옵션은 이전 차량이 있던 "
         "위치에 그대로 둡니다."},
    .default_choice = L"기본값",
    .minimap_choices = {L"차량 따라가기", L"카메라 따라가기"},
    .spawn_choices = {L"자동차 모임 입구", L"페스티벌 현장"},
    .car_position_choices = {L"가장 가까운 도로", L"같은 위치"}};

static const VideoOptionsText chinese = {
    .camera = "甩尾攝影機",
    .driver = "駕駛",
    .parameters = {"靈敏度", "視角移動速度", "範圍"},
    .descriptions = {"啟用甩尾攝影機、讓它只在第一人稱視角中啟用，或停用甩尾攝影機。",
                     "車輛角度與攝影機角度的比例。提高此設定可增加攝影機對車輛角度的靈敏度。",
                     "攝影機從原始位置轉向車輛移動方向的速度。降低此設定可讓攝影機在方向改變時"
                     "移動得更平順。",
                     "降低此設定可限制車輛達到最大角度時攝影機的移動幅度。"},
    .choices = {L"開啟", L"駕駛", L"關閉"},
    .fov_views = {"追蹤", "遠距離追蹤", "駕駛", "引擎蓋", "保險桿"},
    .fov_description = "以度為單位設定所選攝影機的垂直視野。",
    .settings_names = {"即時陰影", "", "相片模式取樣數", "", "小地圖方向", "Autodrive電影攝影機",
                       "略過錦標賽對話", "開始位置", "換車時的位置"},
    .settings_descriptions =
        {"每一幀更新太陽陰影。會增加GPU負載。需要重新啟動遊戲。", "",
         "設定相片模式中景深與動態模糊的取樣數。預設使用129個樣本。較高數值會增加拍"
         "攝時間。需要重新啟動"
         "遊戲。",
         "", "選擇小地圖跟隨車輛或攝影機的方向。", "Autodrive時使用電影攝影機。",
         "駕駛不符合資格的車輛進入錦標賽時，略過強制對話。需要重新啟動遊戲。",
         "選擇遊戲啟動時的開始位置。預設會像平常一樣，從最接近您關閉遊戲時所在位置"
         "的車聚開始。需要重新"
         "啟動遊戲才能套用變更。",
         "在開放世界換車時，新車應該出現的位置。後者會讓您留在舊車原本所在的位置"
         "。"},
    .default_choice = L"預設",
    .minimap_choices = {L"跟隨車輛", L"跟隨攝影機"},
    .spawn_choices = {L"車聚入口", L"嘉年華會場"},
    .car_position_choices = {L"最近的道路", L"相同位置"}};

static const VideoOptionsText czech = {
    .camera = "Driftovací kamera",
    .driver = "Řidič",
    .parameters = {"Citlivost", "Rychlost otáčení", "Rozsah"},
    .descriptions = {"Zapněte driftovací kameru, povolte ji pouze v pohledech z první osoby, "
                     "nebo ji vypněte.",
                     "Poměr mezi úhlem vozu a úhlem kamery. Zvýšením této hodnoty zvýšíte "
                     "citlivost kamery na úhel "
                     "vozu.",
                     "Rychlost, jakou se kamera otáčí z původní polohy do směru pohybu. "
                     "Snížením této hodnoty "
                     "vyhladíte pohyb kamery při změnách směru.",
                     "Snížením této hodnoty omezíte pohyb kamery, když vůz dosáhne maximálního "
                     "úhlu."},
    .choices = {L"Zapnuto", L"Řidič", L"Vypnuto"},
    .fov_views = {"Sledování", "Vzdálené sledování", "Řidič", "Kapota", "Nárazník"},
    .fov_description = "Nastavte svislé zorné pole vybrané kamery ve stupních.",
    .settings_names = {"Stíny v reálném čase", "", "Vzorky fotografického režimu", "",
                       "Orientace minimapy", "Filmová kamera Autodrive",
                       "Přeskočit dialog šampionátu", "Počáteční poloha", "Poloha při změně auta"},
    .settings_descriptions =
        {"Aktualizuje sluneční stíny v každém snímku. Zvyšuje zatížení GPU. "
         "Vyžaduje restart.",
         "",
         "Nastaví vzorky fotografického režimu pro hloubku ostrosti a rozmazání "
         "pohybu. Výchozí "
         "nastavení používá 129 vzorků. Vyšší hodnoty prodlouží snímání. Vyžaduje "
         "restart.",
         "", "Zvolte, zda minimapa sleduje auto, nebo kameru.",
         "Používá filmové kamery během Autodrive.",
         "Přeskočí vynucený dialog při vstupu do závodu šampionátu s nezpůsobilým "
         "autem. Vyžaduje "
         "restart.",
         "Vyberte polohu při spuštění hry. Výchozí vás jako obvykle umístí na sraz "
         "aut nejbližší vaší "
         "poloze při ukončení hry. Změny se projeví po restartu hry.",
         "Kde má stát nové auto při změně auta v otevřeném světě. Druhá možnost "
         "vás ponechá přesně na "
         "místě předchozího auta."},
    .default_choice = L"Výchozí",
    .minimap_choices = {L"Sledovat auto", L"Sledovat kameru"},
    .spawn_choices = {L"Vjezd na sraz aut", L"Areál festivalu"},
    .car_position_choices = {L"Nejbližší silnice", L"Stejná poloha"}};

static const VideoOptionsText hungarian = {
    .camera = "Driftkamera",
    .driver = "Vezető",
    .parameters = {"Érzékenység", "Nézetváltás sebessége", "Tartomány"},
    .descriptions = {"Kapcsold be a driftkamerát, engedélyezd csak a belső nézetekben, vagy "
                     "kapcsold ki.",
                     "Az autó szöge és a kamera szöge közötti arány. Növeld ezt az értéket, "
                     "hogy a kamera "
                     "érzékenyebben reagáljon az autó szögére.",
                     "A sebesség, amellyel a kamera az eredeti helyzetéből a haladás iránya "
                     "felé fordul. Az érték "
                     "csökkentésével simábbá teheted a kamera mozgását az irányváltások során.",
                     "Csökkentsd ezt az értéket, hogy korlátozd a kamera mozgását, amikor az "
                     "autó eléri a "
                     "legnagyobb szöget."},
    .choices = {L"Be", L"Vezető", L"Ki"},
    .fov_views = {"Követő", "Távoli követő", "Vezető", "Motorháztető", "Lökhárító"},
    .fov_description = "Állítsd be a kiválasztott kamera függőleges látószögét fokban.",
    .settings_names = {"Valós idejű árnyékok", "", "Fotómód mintái", "", "Minitérkép tájolása",
                       "Autodrive filmes kamerája", "Bajnokság párbeszédének kihagyása",
                       "Kezdőhely", "Autócsere helye"},
    .settings_descriptions =
        {"Minden képkockán frissíti a napárnyékokat. Növeli a GPU terhelését. "
         "Újraindítás szükséges.",
         "",
         "Beállítja a fotómód mélységélességének és mozgáselmosásának mintáit. Az "
         "alapérték 129 minta. "
         "A nagyobb érték hosszabb felvételi időt jelent. Újraindítás szükséges.",
         "", "Válaszd ki, hogy a minitérkép az autót vagy a kamerát kövesse.",
         "Filmes kamerákat használ az Autodrive során.",
         "Kihagyja a kötelező párbeszédet, ha nem megfelelő autóval lépsz be egy "
         "bajnoki versenybe. "
         "Újraindítás szükséges.",
         "Válaszd ki a játék indításakor használt kezdőhelyet. Az alapérték a "
         "megszokott módon a "
         "kilépéskori helyedhez legközelebbi autóstalálkozón indít. A "
         "módosításokhoz újra kell "
         "indítani a játékot.",
         "Az új autó helye, amikor a nyílt világban autót váltasz. A második "
         "lehetőség pontosan ott "
         "hagy, ahol az előző autó volt."},
    .default_choice = L"Alapérték",
    .minimap_choices = {L"Autó követése", L"Kamera követése"},
    .spawn_choices = {L"Autóstalálkozó bejárata", L"Fesztivál helyszíne"},
    .car_position_choices = {L"Legközelebbi út", L"Azonos hely"}};

static const VideoOptionsText polish = {
    .camera = "Kamera driftu",
    .driver = "Kierowca",
    .parameters = {"Czułość", "Szybkość obrotu", "Zakres"},
    .descriptions = {"Włącz kamerę driftu, aktywuj ją tylko w widokach pierwszoosobowych lub "
                     "ją wyłącz.",
                     "Stosunek kąta samochodu do kąta kamery. Zwiększ tę wartość, aby kamera "
                     "była bardziej czuła "
                     "na kąt samochodu.",
                     "Szybkość, z jaką kamera obraca się z pierwotnego położenia w kierunku "
                     "ruchu. Zmniejszenie "
                     "tej wartości wygładzi ruch kamery przy zmianach kierunku.",
                     "Zmniejsz tę wartość, aby ograniczyć ruch kamery, gdy samochód osiągnie "
                     "maksymalny kąt."},
    .choices = {L"Wł.", L"Kierowca", L"Wył."},
    .fov_views = {"Pościg", "Daleki pościg", "Kierowca", "Maska", "Zderzak"},
    .fov_description = "Ustaw pionowe pole widzenia wybranej kamery w stopniach.",
    .settings_names = {"Cienie w czasie rzeczywistym", "", "Próbki trybu fotograficznego", "",
                       "Orientacja minimapy", "Filmowa kamera Autodrive", "Pomiń dialog mistrzostw",
                       "Miejsce startu", "Pozycja po zmianie samochodu"},
    .settings_descriptions =
        {"Aktualizuje cienie słońca w każdej klatce. Zwiększa obciążenie GPU. "
         "Wymaga restartu.",
         "",
         "Ustawia próbki trybu fotograficznego dla głębi ostrości i rozmycia "
         "ruchu. Domyślnie używa "
         "129 próbek. Wyższe wartości wydłużają czas rejestracji. Wymaga ponownego "
         "uruchomienia.",
         "", "Wybierz, czy minimapa ma śledzić samochód, czy kamerę.",
         "Używa filmowych kamer podczas Autodrive.",
         "Pomija obowiązkowy dialog przy wjeździe do wyścigu mistrzostw "
         "nieodpowiednim samochodem. "
         "Wymaga restartu.",
         "Wybierz miejsce startu po uruchomieniu gry. Domyślne umieszcza cię jak "
         "zwykle na zlocie "
         "najbliższym pozycji przy zamknięciu gry. Uruchom grę ponownie, aby "
         "zastosować zmiany.",
         "Miejsce nowego samochodu po zmianie auta w otwartym świecie. Druga opcja "
         "pozostawia cię "
         "dokładnie tam, gdzie był poprzedni samochód."},
    .default_choice = L"Domyślne",
    .minimap_choices = {L"Śledź samochód", L"Śledź kamerę"},
    .spawn_choices = {L"Wjazd na zlot", L"Teren festiwalu"},
    .car_position_choices = {L"Najbliższa droga", L"To samo miejsce"}};

static const VideoOptionsText russian = {
    .camera = "Дрифт-камера",
    .driver = "Водитель",
    .parameters = {"Чувствительность", "Скорость поворота", "Диапазон"},
    .descriptions = {"Включите дрифт-камеру, включите её только для видов от первого лица или "
                     "отключите её.",
                     "Соотношение угла автомобиля и угла камеры. Увеличьте это значение, чтобы "
                     "камера стала более "
                     "чувствительной к углу автомобиля.",
                     "Скорость, с которой камера поворачивается из исходного положения в "
                     "направлении движения. "
                     "Уменьшение этого значения делает движение камеры при смене направления "
                     "более плавным.",
                     "Уменьшите это значение, чтобы ограничить движение камеры, когда "
                     "автомобиль достигает "
                     "максимального угла."},
    .choices = {L"Вкл.", L"Водитель", L"Выкл."},
    .fov_views = {"Вид сзади", "Дальний вид сзади", "Водитель", "Капот", "Бампер"},
    .fov_description = "Установите вертикальный угол обзора выбранной камеры в градусах.",
    .settings_names = {"Тени в реальном времени", "", "Выборки фоторежима", "",
                       "Ориентация мини-карты", "Кинематографическая камера Autodrive",
                       "Пропуск диалога чемпионата", "Место появления",
                       "Положение при смене машины"},
    .settings_descriptions =
        {"Обновляет солнечные тени каждый кадр. Повышает нагрузку на GPU. "
         "Требуется перезапуск.",
         "",
         "Число образцов глубины резкости и размытия в движении в фоторежиме. По "
         "умолчанию — 129 "
         "образцов. Высокие значения увеличивают время съёмки. Требуется "
         "перезапуск.",
         "", "Выберите, следует ли мини-карта за автомобилем или камерой.",
         "Использует кинематографические камеры во время Autodrive.",
         "Пропускает обязательный диалог при въезде в гонку чемпионата на "
         "неподходящем автомобиле. "
         "Требуется перезапуск.",
         "Выберите место появления при запуске игры. По умолчанию вы, как обычно, "
         "появляетесь на "
         "автосходке, ближайшей к месту выхода из игры. Для применения изменений "
         "перезапустите игру.",
         "Где должна находиться новая машина при смене машины в открытом мире. "
         "Второй вариант "
         "оставляет вас точно там, где была предыдущая машина."},
    .default_choice = L"По умолчанию",
    .minimap_choices = {L"Следовать за автомобилем", L"Следовать за камерой"},
    .spawn_choices = {L"Въезд на автосходку", L"Площадка фестиваля"},
    .car_position_choices = {L"Ближайшая дорога", L"То же место"}};

static const VideoOptionsText dutch = {
    .camera = "Driftcamera",
    .driver = "Bestuurder",
    .parameters = {"Gevoeligheid", "Kijksnelheid", "Bereik"},
    .descriptions = {"Schakel de driftcamera in, activeer deze alleen in de "
                     "eerstepersoonsweergaven of schakel "
                     "deze uit.",
                     "De verhouding tussen de hoek van de auto en die van de camera. Verhoog "
                     "deze instelling om de "
                     "camera gevoeliger te maken voor de hoek van de auto.",
                     "De snelheid waarmee de camera vanuit de oorspronkelijke positie naar de "
                     "bewegingsrichting "
                     "draait. Een lagere instelling maakt de camerabeweging bij "
                     "richtingsveranderingen vloeiender.",
                     "Verlaag deze instelling om de camerabeweging te beperken wanneer de auto "
                     "zijn maximale hoek "
                     "bereikt."},
    .choices = {L"Aan", L"Bestuurder", L"Uit"},
    .fov_views = {"Volgen", "Ver volgen", "Bestuurder", "Motorkap", "Bumper"},
    .fov_description = "Stel het verticale gezichtsveld van de betreffende camera in graden in.",
    .settings_names = {"Realtime schaduwen", "", "Samples fotomodus", "", "Oriëntatie minikaart",
                       "Filmische camera Autodrive", "Kampioenschapsdialoog overslaan",
                       "Startlocatie", "Positie bij autowissel"},
    .settings_descriptions =
        {"Werkt zonneschaduwen elk beeld bij. Verhoogt de GPU-belasting. Opnieuw "
         "starten vereist.",
         "",
         "Stel de fotomodus-samples voor scherptediepte en bewegingsonscherpte in. "
         "Standaard gebruikt "
         "129 samples. Hogere waarden verlengen de opnametijd. Opnieuw starten "
         "vereist.",
         "", "Kies of de minikaart de auto of de camera volgt.",
         "Gebruikt filmische camera’s tijdens Autodrive.",
         "Slaat de verplichte dialoog over bij een kampioenschapsrace met een "
         "ongeschikte auto. "
         "Opnieuw starten vereist.",
         "Kies de startlocatie bij het opstarten van het spel. Standaard start je "
         "zoals gewoonlijk bij "
         "de autobijeenkomst het dichtst bij je positie toen je het spel afsloot. "
         "Start het spel "
         "opnieuw om wijzigingen toe te passen.",
         "Waar de nieuwe auto moet staan wanneer je in de open wereld van auto "
         "wisselt. De tweede "
         "optie laat je precies staan waar de vorige auto was."},
    .default_choice = L"Standaard",
    .minimap_choices = {L"Auto volgen", L"Camera volgen"},
    .spawn_choices = {L"Ingang autobijeenkomst", L"Festivalterrein"},
    .car_position_choices = {L"Dichtstbijzijnde weg", L"Dezelfde locatie"}};

static const struct {
    const char *code;
    const VideoOptionsText *text;
} translations[] = {{"EN", &english},   {"GB", &english}, {"BR", &brazilian}, {"DE", &german},
                    {"FR", &french},    {"ES", &spanish}, {"MX", &mexican},   {"IT", &italian},
                    {"JP", &japanese},  {"KO", &korean},  {"CHT", &chinese},  {"CZ", &czech},
                    {"HU", &hungarian}, {"PL", &polish},  {"RU", &russian},   {"NL", &dutch}};

const VideoOptionsText *video_options_text(unsigned char *game_image) {
    void *manager;
    unsigned char code[32];
    size_t length, capacity;
    if (!game_image || !memory_read(game_image + GAME_RVA_TEXT_TABLES, &manager, sizeof(manager)) ||
        !manager || !memory_read((unsigned char *)manager + 0x30, code, sizeof(code))) {
        return &english;
    }
    memory_copy(&length, code + 16, sizeof(length));
    memory_copy(&capacity, code + 24, sizeof(capacity));
    if (capacity != 15 || length < 2 || length > 3 || code[length] != 0) {
        return &english;
    }
    for (unsigned index = 0; index < sizeof(translations) / sizeof(translations[0]); ++index) {
        if (text_equal_ignore_case((const char *)code, translations[index].code)) {
            return translations[index].text;
        }
    }
    return &english;
}
