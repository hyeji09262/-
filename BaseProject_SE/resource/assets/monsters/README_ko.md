# 몬스터 리소스

총 11종. Blender Collection 라이브러리이며 각 파일의 같은 이름 Collection을 Append합니다.

추가 5종: NightmareBat(악몽 박쥐), CoralCrab(산호 게), MimicChest(미믹 상자), LanternWisp(등불 도깨비), ThornSeed(가시 씨앗).
Grimoire는 상대가 읽는 책처럼 세워서 약 140도로 펼쳤습니다. 페이지는 독자(+Y), 바깥 표지는 상대편(-Y)을 향하며 공중에 떠 있습니다.

각 몬스터는 Armature와 부위별 가중치를 포함합니다. 새 5종은 Idle, Walk, Run, Jump_Start, Jump_Air, Jump_Land, Turn_Left_90, Turn_Right_90, Attack, Turn_Neutral 10개 동작입니다. Grimoire는 Book_Open/Book_Close를 포함하여 12개입니다.
애니메이션은 음소거된 NLA 트랙으로 보관됩니다. 한 트랙만 활성화하거나 Action Editor에서 선택하여 재생하세요. 이동은 제자리 동작이며 게임 이동/AI 연결은 별도입니다.
걷기↔달리기는 같은 주기 위상을 맞춰 0.15~0.25초 블렌딩, 회전 전후는 Turn_Neutral 또는 Idle로 0.1~0.2초 블렌딩을 권장합니다. 점프는 Start→Air→Land 순서로 연결합니다. 실제 재생 속도와 충돌 판정은 엔진에서 조정하세요.

최신 이미지: ../previews/New_Monsters_Overview.png (변경된 책+추가 5종), Grimoire_Open.png, Grimoire_Attack.png.
Monsters_Overview.png는 직전 6종 업그레이드 시점 이미지로 책의 변경 전 방향을 보여줍니다.
가중치와 각 동작의 대표 프레임 행렬을 검사했고 미리보기를 확인했습니다. 전체 연속 동작과 게임 내 연결은 별도 확인이 필요합니다.
