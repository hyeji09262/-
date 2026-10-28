# 공원·지하철 보스

첨부 기획의 BOSS_BLACK_FLOWER / BOSS_BLACK_CONDUCTOR를 위한 Blender 리소스입니다. 외형은 해당 기획의 지역과 공격 패턴을 바탕으로 디자인했습니다.

- BlackFlower_Boss.blend: MQ_004_BLACK_TRACE_IN_PARK. 겹꽃, 독 주머니, 덩굴 팔, 뿌리 다리. 전용 동작 Thorn / Poison / Vine_Attack / Small_Monster_Summon.
- BlackConductor_Boss.blend: MQ_005_LAST_TRAIN. 차장 제복, 전기 지휘봉, 등불. 전용 동작 Electric_Attack / Teleport_Out / Teleport_In / Monster_Summon. 돌진은 Rush_Charge → Rush → Rush_Recovery로 연결.

각 파일에서 같은 이름 Collection을 Append합니다. 11개 뼈, 부위별 스키닝, 30fps. 기본 17개 + 전용 4개로 각각 21개 클립입니다. Attack은 공통 주먹 공격이며 전용 공격 클립은 별도 선택합니다. NLA 트랙은 음소거되어 있으므로 한 트랙만 활성화하거나 Action Editor에서 동작을 지정하세요.

걷기/달리기/돌진은 제자리입니다. 점프는 root 높이, 회전은 root yaw를 포함합니다. 걷기↔달리기는 위상을 맞춰 약 0.2초 블렌딩, 공격 후 Idle 복귀는 약 0.15~0.2초를 시작값으로 사용합니다. 순간이동 Out은 축소, In은 복귀 동작이며 위치 변경은 게임 측에서 처리합니다.

독 장판, 가시 투사체, 번개, 소환 몬스터 생성, 피해 판정과 AI는 포함하지 않습니다. 현재 결과는 메시 및 동작 리소스입니다.

검증: 모든 정점의 가중치, 각 클립의 시작/중간/끝 뼈 행렬, 기본 및 전용 공격 자세 렌더. 실제 엔진 연속 재생과 충돌 확인은 별도입니다.
미리보기는 ../previews/BlackFlower_Boss.png, BlackFlower_Boss_Attack.png, BlackConductor_Boss.png, BlackConductor_Boss_Attack.png입니다.
