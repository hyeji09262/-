# 마을 NPC와 실내 가구

## NPC 20명
characters/npcs/에 주민 10명, 선생님 4명, 경비원 1명, 간호사 1명, 백화점 직원 3명, 편의점 알바 1명을 개별 Collection 라이브러리로 저장했습니다. 성별·머리·의상 색을 바꿔 구분했습니다. 교직원은 업무복/명찰/책, 경비원은 제복/모자/무전기, 간호사는 흰 유니폼/간호모, 백화점 직원은 남색·크림색 서비스복/스카프/명찰, 편의점 직원은 녹색 앞치마를 사용합니다. 주민의 복장은 일상복입니다.
각 NPC는 독립된 리그와 10개 Action을 보유합니다. 30fps: Idle 1–61, Walk 1–31, Run 1–21, Jump_Start 1–13, Jump_Air 1–25, Jump_Land 1–13, Turn_Left_90/Turn_Right_90 1–19, Attack 1–25, Turn_Neutral 1–3. NLA 트랙은 보관용 mute 상태이며 Action Editor에서 해당 NPC Action을 선택합니다.
부위별 공통 인간형 뼈와 점눈 규격을 유지했습니다. 모든 NPC의 양수 정점 가중치, 10개 트랙과 각 Action 8프레임에서 유한한 뼈 변환을 검사했습니다. 전체 동작 중 관통/발 접지는 별도 확인이 필요합니다.
npc_catalog.json은 역할·장소 태그·권장 걷기 속도/대기 시간 참고값입니다. 실제 배회 AI·길찾기·지도 배치·상호작용은 구현하지 않았습니다. 이동 애니메이션은 제자리 동작이므로 게임에서 위치를 이동해야 합니다.

## 주인공 뒷머리
Hero_Student.blend와 Hero_Magical.blend에 뒤통수를 덮는 연결된 둥근 머리 메시와 목덜미 머리카락을 추가했습니다. 기존 파일을 갱신했으며 별도 이전 버전은 만들지 않았습니다. 앞머리·양갈래·리그/동작은 보존했습니다. Hero_Back_Hair.png에서 뒤쪽을 확인할 수 있습니다.

## 가구 24종
interiors/furniture/에 책상 3종, 의자, 소파, 침대/의료침대, 책장/판매 진열대/약품장/옷장/사물함, 카운터, 컴퓨터, 계산대, 일반/유리문 냉장고, 싱크대와 쿡탑, 칠판, 전시 테이블, 의류 행거, 의료 카트, 가림막, 거울을 저장했습니다.
각 가구의 기준점은 바닥 중심, 미터 단위입니다. 컴퓨터와 계산대는 책상·카운터 위에 놓는 탁상 소품입니다. 서랍·문·거울/유리의 기능, 충돌, 상호작용이나 가구 애니메이션은 포함하지 않습니다. 유리는 색상 재질 기반의 단순 표현입니다.

## 실내 배치 예제 7곳
interiors/layouts/의 Home, Classroom, Staffroom, Infirmary, Guardroom, DepartmentStore, ConvenienceStore입니다. 각각 집/교실/교무실/보건실/경비실/백화점/편의점에 맞는 가구를 Collection 인스턴스로 배치했습니다. 바닥과 뒤·옆 벽만 둔 내부 확인용 절개 예제입니다.
방 크기는 보통 8×7m, 경비실은 5×4m입니다. 기존 외관 건물의 내부 치수에 자동으로 맞춘 것은 아니므로 실제 게임 실내 공간에 맞게 조정해야 합니다. 인스턴스 가구는 원본을 공유합니다. 각 가구를 독립 편집하려면 인스턴스를 실체화하거나 원본 Collection을 복제하십시오. layout_catalog.json에 가구별 좌표·방향을 기록했습니다.

Blender File > Append > 파일 > Collection으로 불러오면 됩니다. 카메라·조명·미리보기용 소품은 개별 라이브러리에서 제외합니다. NPCs_20_Overview.png, Hero_Back_Hair.png, Interiors_Overview.png 및 Interior_방이름.png를 previews/에 저장했습니다. 기존 전체 개요 이미지는 이번 추가분을 포함하지 않습니다.
게임 코드와 resource/runtime은 변경하지 않았습니다.

다시 불러오기 검사 결과는 town_interiors_validation.json에 저장했습니다. NPC 20명·가구 24종·실내 7곳의 참조를 확인했습니다.


2026-10-25 수정: 집·교실·교무실·보건실·경비실의 의자 13개를 각각 가까운 책상/식탁을 향하도록 돌렸습니다. 의자의 로컬 정면은 -Y, 등받이는 +Y입니다. layout_catalog.json과 배치 원본, 실내 미리보기를 갱신했으며 방향 검사 결과는 interiors/chair_facing_validation.json에 기록했습니다.
