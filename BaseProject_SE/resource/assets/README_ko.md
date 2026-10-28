# 최종 리소스

Blender File > Append > 해당 .blend > Collection으로 가져오십시오. 각 파일은 모델/재질/리그/Action만 포함하는 라이브러리이며 카메라·조명은 포함하지 않습니다.

- characters: 주인공 변신/학생, 유나, 지수, 남학생 2종, 고양이 루미(7개).
- monsters: 슬라임, 유령 마법사, 독버섯, 그림자 토끼, 마도서, 보스(6개).
- buildings: 학교/주택/상점 조립 예제, 상세 부품, 조립 모듈(5개).
- nature: 벚나무/풀/바위/지형/바다 재질 모듈(1개).
- props: 책상/의자/침대/벤치/가로등 등(1개).
- weapons: 4계열×4단계 및 주인공과 같은 기본 별 지팡이(17개).
- previews: 손에 지팡이를 든 자세와 건물 미리보기.

팔의 앞쪽으로 비틀린 형태를 제거했습니다. 주인공은 곧은 위팔에서 아래팔을 살짝 굽혀 지팡이를 들어 올린 모습입니다. 별과 봉의 원래 결합 형태를 유지하고, 지팡이 전체를 손에서 바깥쪽 25도/앞쪽 12도 기울였습니다. 참고 이미지는 손과 무기의 자세 기준으로 사용했고 기존 캐릭터 색감과 점눈은 유지했습니다.
인간형과 몬스터는 부위별 스킨 메시, 각 10개 Action, 30fps입니다. NLA는 보관용 mute 상태이며 Action Editor에서 리그 이름에 맞는 Action을 선택합니다. Idle/Walk/Run/Jump_Start/Jump_Air/Jump_Land/Turn_Left_90/Turn_Right_90/Attack/Turn_Neutral이 있습니다. 아래팔은 현재 arm.R 뼈를 공유하는 고정 굽힘 모양이며 독립 팔꿈치 IK는 없습니다.
걷기↔뛰기는 주기 위상을 맞춰 약 0.2초 보간하는 것이 초기 제안값입니다. 점프 높이와 root 회전은 게임 변환과 이중 적용하지 않도록 처리해야 합니다. 정지 렌더, 스킨 가중치, 파일 재불러오기와 10개 Action 보존을 검사했습니다. 이번 수정 자세의 전체 동작 중 관통 검증과 게임 내 스키닝 확인은 별도입니다.
캐릭터/무기는 원점 기준입니다. 건물/환경은 조립 예제의 배치 좌표를 보존합니다. 메시 파츠 교환 시 대상 리그로 부모와 Armature Modifier를 맞춰야 합니다.
이전 버전 폴더와 임시 제작 스크립트는 정리했습니다. resource/runtime 및 기존 export_runtime_models.py는 다른 작업의 게임 연동 파일이므로 유지했습니다. 그 기존 내보내기 스크립트는 삭제된 v02 경로를 참조할 수 있으므로 이 assets 구조에 맞춘 수정이 필요합니다. 여기서는 게임 코드를 변경하지 않았습니다.

전체 확인 이미지: previews/All_Assets_Overview.png (2400×1800). 종류별 Characters/Monsters/Weapons/Buildings/Nature/Props_Overview.png도 함께 저장했습니다. 비교 편의를 위해 항목별 표시 크기를 조정했으므로 이 이미지의 상대 크기는 게임 내 실제 스케일과 다릅니다.

주인공의 굽힌 아래팔은 분홍색 소매 재질로 통일하여 손만 노출되도록 수정했습니다. Hero_Magical.blend와 Hero_Grip/Characters_Overview/All_Assets_Overview.png는 현재 모습으로 덮어썼습니다.

자연물 추가: nature/Tree_A~C.blend, Grass_A~C.blend, Rock_A~C.blend 총 9종. 바닥 기준 원점으로 배치 가능합니다. random_variants.json에는 균등 선택 가중치, 크기 0.85~1.15, Z축 회전 0~360도 제안값을 기록했습니다. 게임 소환 로직은 구현하지 않았습니다. 새 미리보기는 previews/Nature_Variants.png이며 기존 전체 미리보기에는 이번 추가분이 없습니다.

추가 제작: NPC 20명(characters/npcs), 가구 24종(interiors/furniture), 실내 배치 예제 7곳(interiors/layouts), 주인공 뒷머리 보완. 상세 내용은 Town_NPCs_Interiors_ko.md와 해당 previews 이미지를 참고하십시오. 기존 전체 개요 이미지는 이번 추가분을 포함하지 않습니다.
