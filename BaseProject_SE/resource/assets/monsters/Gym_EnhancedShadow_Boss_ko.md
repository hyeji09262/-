# 체육관 강화 Shadow

기획 근거: MQ_002_SOMETHING_IN_SCHOOL의 체육관 강화 Shadow 전투.
기획에는 고유 보스 이름, 외형, 전용 패턴이 정해져 있지 않습니다. 이 리소스는 해당 역할을 위한 시안이며, 검은 조각 핵과 어깨 보호대·손목 밴드를 가진 육중한 근접형으로 구성했습니다.

파일: Gym_EnhancedShadow_Boss.blend. 같은 이름의 Collection을 Append하여 사용합니다.
정면 -Y, 높이 Z, 뿌리 원점은 지면 중앙입니다. 뼈 11개로 머리·몸통·상완·전완·허벅지·정강이를 구분했습니다. 메시 부품은 해당 뼈에 가중치 1로 연결됩니다.

30fps / 17개 클립. 기본 Idle, Walk, Run, Jump_Start/Air/Land, Turn_Left_90/Right_90, Turn_Neutral, Attack 외에 Punch, Ground_Slam, Rush_Charge/Rush/Recovery, Hit, Death가 있습니다. Attack은 Punch와 같은 기본 공격입니다. NLA 트랙은 보관을 위해 음소거되어 있으므로 하나만 활성화하거나 Action Editor에서 해당 동작을 선택하세요.

걷기→달리기는 보행 위상을 맞추어 0.2초, 돌진 준비→돌진은 0.1초, 돌진 회복→대기는 0.2초 블렌딩을 시작값으로 권장합니다. 회전 전후는 중립 자세로 연결합니다. 이동/돌진은 제자리이며 게임에서 이동량을 적용합니다. 점프 공중 동작은 root Z, 회전은 root yaw를 사용합니다.

공격 준비 시간과 타격 구간은 시안입니다. Punch는 약 1~13프레임 준비, 14~19 타격, 이후 복귀. Ground_Slam은 1~23 준비, 24~30 내려치기, 이후 복귀입니다. 충돌/피해/무적 시간과 AI는 포함하지 않습니다.

검증: 모든 메시의 가중치 존재, 각 클립 시작·중간·끝의 유한한 뼈 행렬, 기본 자세·뒷면·내려찍기 준비 자세 렌더를 확인했습니다. 게임 안에서 연속 동작, 발 미끄러짐, 충돌 판정은 추가 확인해야 합니다.

미리보기: ../previews/Gym_EnhancedShadow_Boss.png, Gym_EnhancedShadow_Boss_Back.png, Gym_EnhancedShadow_Boss_Slam.png.
