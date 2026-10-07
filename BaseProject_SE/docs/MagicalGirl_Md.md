# Magical Girl Game - Existing Project Implementation Spec

------------------------------------------------------------------------

# 1. 게임 콘셉트

장르:

**3인칭 마법소녀 액션 RPG + 퀘스트 + 탐험**

플레이어는 평범한 학생으로 시작한다.

도시에서 학생들이 사라지는 사건이 발생하고, 친구 유나를 찾던 주인공은
처음으로 괴물을 발견한다.

위기에 빠진 순간 정체불명의 마법 생명체 `루미`를 만나 마법소녀로
변신한다.

이후 도시 곳곳에서 괴물을 처치하고 `검은 조각`을 수집하면서 괴물이
나타나는 원인을 조사한다.

스토리의 장기 목표는 단순하게 유지한다.

> 괴물을 물리치면서 검은 조각의 정체와 괴물을 만들어내는 존재를
> 추적한다.

엔딩은 현재 구현하지 않는다.

------------------------------------------------------------------------

# 2. 핵심 게임 루프

``` text
NPC 대화
↓
퀘스트 수락
↓
지역 이동
↓
조사 / 아이템 발견
↓
몬스터 전투
↓
보스 전투
↓
검은 조각 또는 단서 획득
↓
NPC에게 보고
↓
보상 / 스킬 / 지역 해금
↓
다음 퀘스트
```

새로운 지역을 추가할 때도 이 구조를 재사용한다.

------------------------------------------------------------------------

# 3. MVP 구현 범위

첫 구현에서는 아래만 완성한다.

## 지역

-   주인공 집
-   학교
-   학교 뒤 골목

## NPC

-   유나
-   지수
-   루미

## 몬스터

-   Shadow
-   Shadow Eater Boss

## 시스템

-   NPC 상호작용
-   대화
-   퀘스트
-   퀘스트 목표 표시
-   변신
-   기본 공격
-   회피
-   특수 마법
-   HP / MP
-   몬스터 AI
-   몬스터 사망
-   아이템 드롭
-   인벤토리
-   검은 조각
-   퀘스트 보상
-   다음 퀘스트 해금

------------------------------------------------------------------------

# 4. 프롤로그

## 4.1 시작

게임은 주인공의 방에서 시작한다.

TV 또는 휴대폰 뉴스에서 짧게 다음 내용을 보여준다.

> 최근 도시에서 학생 실종 신고가 증가하고 있습니다.

긴 설명은 하지 않는다.

플레이어는 학교로 이동한다.

이 구간에서 다음 조작을 자연스럽게 안내한다.

-   이동
-   카메라
-   달리기
-   점프
-   상호작용

------------------------------------------------------------------------

# 5. 학교 이벤트

학교에서 친구 `NPC_YUNA`와 대화한다.

유나는 최근 같은 학교 학생이 실종됐다는 소문을 알려준다.

마지막 목격 장소는 학교 뒤편의 오래된 골목이다.

예시 대사:

``` text
유나:
"요즘 학교 뒤쪽 골목으로 가지 말래."

"우리 학교 학생도 거기서 마지막으로 봤대."
```

이 시점에는 전투가 없다.

방과 후 이벤트로 전환한다.

------------------------------------------------------------------------

# 6. 메인 퀘스트 001

## Quest ID

`MQ_001_MISSING_FRIEND`

## 이름

`연락이 끊긴 친구`

## 시작 NPC

`NPC_JISOO`

## 시작 조건

`School_Day01_Event == Completed`

## 퀘스트 흐름

``` text
지수와 대화
↓
유나의 위치 조사
↓
학교 경비 NPC와 대화
↓
편의점 NPC와 대화
↓
학교 뒤 골목 이동
↓
검은 조각 발견
↓
유나의 가방 발견
↓
쓰러진 유나 발견
↓
괴물 등장
↓
도망
↓
루미 등장
↓
첫 변신
↓
Shadow 3마리 처치
↓
Shadow Eater 보스
↓
유나 확인
↓
루미와 대화
↓
퀘스트 완료
```

## Step IDs

``` text
MQ001_01_TALK_JISOO
MQ001_02_ASK_SCHOOL_GUARD
MQ001_03_ASK_STORE_WORKER
MQ001_04_ENTER_BACK_ALLEY
MQ001_05_FIND_BLACK_FRAGMENT
MQ001_06_FIND_YUNA_BAG
MQ001_07_FIND_YUNA
MQ001_08_ESCAPE_SHADOWS
MQ001_09_MEET_LUMI
MQ001_10_FIRST_TRANSFORMATION
MQ001_11_KILL_SHADOWS
MQ001_12_KILL_SHADOW_EATER
MQ001_13_CHECK_YUNA
MQ001_14_TALK_LUMI
```

## 완료 보상

-   Transformation 해금
-   Magic Shot 해금
-   Dodge 해금
-   Black Fragment x3
-   EXP
-   Gold

## 다음 퀘스트

`MQ_002_SOMETHING_IN_SCHOOL`

------------------------------------------------------------------------

# 7. 첫 번째 검은 조각 이벤트

골목에 들어간 플레이어가 바닥에서 처음으로 검은 조각을 발견한다.

## Item ID

`ITEM_BLACK_FRAGMENT`

## 표시 이름

`검은 조각`

## 설명

> 정체를 알 수 없는 검은 결정 조각. 약한 마력이 느껴진다.

이 아이템은 일반 판매 불가.

스토리 진행 및 퀘스트 조건에 사용한다.

------------------------------------------------------------------------

# 8. 첫 전투 전 도주 이벤트

유나를 발견했을 때 작은 Shadow들이 나타난다.

플레이어는 아직 변신하지 않았기 때문에 공격할 수 없다.

목표:

`EscapeToTargetPoint`

도주 구간 마지막에 출구가 막히고 루미 이벤트가 시작된다.

------------------------------------------------------------------------

# 9. 루미 등장 및 첫 변신

## NPC ID

`NPC_LUMI`

루미는 검은 조각에 반응해 등장한다.

예시:

``` text
루미:
"그걸 어디서 주웠어?"

주인공:
"뭐?"

루미:
"네가 가지고 있는 검은 조각!"
```

괴물에게 포위되면 루미가 변신을 제안한다.

``` text
루미:
"일단 살아남고 싶으면 내 손을 잡아."
```

첫 변신 Cinematic/Event 실행.

이 이벤트 이후:

``` text
CanTransform = true
```

------------------------------------------------------------------------

# 10. 플레이어 상태

최소 상태:

``` text
Normal
Transforming
MagicalGirl
Dead
```

첫 변신 이전:

``` text
CanTransform = false
```

`MQ001_10_FIRST_TRANSFORMATION` 완료 후:

``` text
CanTransform = true
```

전투 능력은 MagicalGirl 상태에서만 사용 가능하도록 구성한다.

------------------------------------------------------------------------

# 11. 플레이어 전투

초기 전투는 단순하게 유지한다.

## 11.1 기본 마법 공격

Skill ID:

`SKILL_MAGIC_SHOT`

3연속 공격.

``` text
Shot 1
-> Shot 2
-> Shot 3
```

3타의 데미지를 약간 높게 설정한다.

------------------------------------------------------------------------

## 11.2 근접 공격

Skill ID:

`SKILL_MAGIC_MELEE`

마법봉을 사용한 근접 3연타.

MVP에서 원거리 공격만 먼저 구현해도 된다.

------------------------------------------------------------------------

## 11.3 회피

Skill ID:

`SKILL_DODGE`

입력 방향으로 빠르게 이동.

짧은 무적 시간 적용 가능.

------------------------------------------------------------------------

## 11.4 특수 마법

Skill ID:

`SKILL_STAR_BURST`

MP 소비.

플레이어 전방 또는 주변에 마법 폭발을 생성한다.

다수 적 공격 가능.

------------------------------------------------------------------------

## 11.5 궁극기

Phase 2 이후 구현.

Skill ID:

`SKILL_LUMINA_BREAK`

Ultimate Gauge가 100%일 때 사용.

광역 공격.

------------------------------------------------------------------------

# 12. 플레이어 스탯

초기에는 아래만 사용한다.

``` text
Level
CurrentHP
MaxHP
CurrentMP
MaxMP
Attack
MagicAttack
EXP
Gold
```

불필요하게 많은 능력치는 추가하지 않는다.

------------------------------------------------------------------------

# 13. NPC 시스템

NPC 공통 데이터:

``` text
NPCID
DisplayName
DialogueData
QuestIDs
bIsQuestProvider
bIsShop
InteractionRange
```

필요 기능:

``` text
Interact()
StartDialogue()
GiveQuest()
CompleteQuest()
OpenShop()
```

퀘스트 상태에 따라 대사를 변경할 수 있어야 한다.

예:

``` text
Quest Locked
Quest Available
Quest Active
Quest Completed
```

------------------------------------------------------------------------

# 14. 주요 NPC

## NPC_LUMI

역할:

-   메인 스토리 안내
-   변신 관련
-   마법 설명
-   괴물 감지
-   퀘스트 연결

## NPC_YUNA

역할:

-   주인공 친구
-   학교 스토리
-   일상 퀘스트
-   주인공이 마법소녀라는 사실을 모름

## NPC_JISOO

역할:

-   첫 메인 퀘스트 시작
-   유나 실종 정보 제공

## NPC_MIA

Phase 2 추가.

역할:

-   마법 상점
-   장비 강화
-   검은 조각 조사
-   다음 지역 정보 제공

## NPC_REINA

Phase 3 추가.

역할:

-   다른 마법소녀
-   라이벌이면서 필요할 때 협력
-   검은 조각을 조사 중
-   메인 스토리의 다음 단서 제공

------------------------------------------------------------------------

# 15. 몬스터 Base

기존 Enemy Base가 있으면 새로 만들지 않고 확장한다.

공통 데이터:

``` text
MonsterID
DisplayName
MaxHP
CurrentHP
AttackDamage
MoveSpeed
DetectionRange
AttackRange
EXPReward
GoldReward
DropTable
```

AI 상태:

``` text
Idle
Patrol
Chase
Attack
Hit
Dead
```

------------------------------------------------------------------------

# 16. 일반 몬스터

## MON_SHADOW

기본 근접 몬스터.

행동:

``` text
Idle
↓
Player Detect
↓
Chase
↓
Attack Range
↓
Claw Attack
```

플레이어가 멀어지면 다시 추적한다.

MVP에서는 이 몬스터를 먼저 완성한다.

------------------------------------------------------------------------

## MON_SHOOTER

Phase 2.

원거리 공격:

`DarkProjectile`

플레이어와 거리를 유지한다.

------------------------------------------------------------------------

## MON_RUSHER

Phase 2.

``` text
Charge
-> Rush
-> Recovery
```

돌진 전에 플레이어가 알아볼 수 있는 준비 동작이 필요하다.

------------------------------------------------------------------------

## MON_TANK

높은 HP / 낮은 이동 속도.

공격:

-   Punch
-   Ground Slam

------------------------------------------------------------------------

## MON_FLYER

공중형 몬스터.

원거리 마법 공격.

------------------------------------------------------------------------

## MON_HEALER

주변 몬스터 HP 회복.

------------------------------------------------------------------------

# 17. 프롤로그 보스

## Boss ID

`BOSS_SHADOW_EATER`

## 이름

`그림자 포식자`

## 공격 패턴

### Claw

전방 할퀴기.

### Rush

플레이어 방향으로 돌진.

공격 전에 준비 애니메이션 또는 이펙트를 보여준다.

### Jump Attack

플레이어 위치로 점프 후 범위 피해.

## AI

``` text
Detect Player
↓
Move Into Range
↓
Select Random Available Attack
↓
Attack
↓
Cooldown
↓
Repeat
```

MVP에서는 Phase 변경을 만들지 않는다.

## Drop

``` text
ITEM_BLACK_FRAGMENT x3
EXP
Gold
```

------------------------------------------------------------------------

# 18. 프롤로그 종료

보스 처치 후 유나가 정신을 차린다.

유나는 괴물이나 마법소녀 전투를 기억하지 못한다.

유나에게는 루미도 보이지 않는다.

유나가 돌아간 뒤 주인공과 루미가 대화한다.

루미도 검은 조각의 정확한 정체를 모른다.

다음 날 학교에서 주인공의 검은 조각이 반응한다.

창밖으로 작은 Shadow가 지나간다.

루미:

> "또 나타났어."

다음 메인 퀘스트 시작.

------------------------------------------------------------------------

# 19. 메인 퀘스트 002

## Quest ID

`MQ_002_SOMETHING_IN_SCHOOL`

## 이름

`학교에 숨어든 것`

## 시작 조건

``` text
MQ_001_MISSING_FRIEND == Completed
```

## 진행

``` text
학교에서 검은 조각 반응
↓
이상 현상 조사
↓
학교 내부 조사 지점 3개 확인
↓
Shadow 처치
↓
체육관 이동
↓
강화 Shadow 전투
↓
검은 조각 획득
↓
루미와 대화
```

## 보상

-   EXP
-   Gold
-   Magic Stone
-   신규 능력 또는 Dodge 강화

## 다음 퀘스트

`MQ_003_FIND_MIA`

------------------------------------------------------------------------

# 20. 메인 퀘스트 003 - 미아

## Quest ID

`MQ_003_FIND_MIA`

루미는 검은 조각을 조사할 수 있는 사람을 찾자고 제안한다.

NPC `MIA` 등장.

미아는 평범한 상점 직원처럼 보이지만 마법과 괴물을 알고 있다.

미아:

> "이건 그냥 괴물의 일부가 아니야."

미아는 비슷한 마력이 공원에서 감지됐다고 알려준다.

Next:

`MQ_004_BLACK_TRACE_IN_PARK`

------------------------------------------------------------------------

# 21. 공원 지역

## Quest ID

`MQ_004_BLACK_TRACE_IN_PARK`

NPC `MINHO`가 강아지를 찾아달라고 부탁한다.

퀘스트 흐름:

``` text
민호와 대화
↓
강아지 흔적 조사
↓
몬스터 처치
↓
강아지 발견
↓
Black Flower 등장
↓
보스 처치
↓
강아지 구조
↓
검은 조각 획득
↓
미아에게 보고
```

## Boss

`BOSS_BLACK_FLOWER`

공격:

-   Thorn
-   Poison
-   Vine Attack
-   Small Monster Summon

## 보상

``` text
SKILL_STAR_RAIN
ITEM_MAGIC_STONE
ITEM_BLACK_FRAGMENT
EXP
Gold
```

------------------------------------------------------------------------

# 22. 지하철 지역

## Quest ID

`MQ_005_LAST_TRAIN`

미아가 지하철에서 동일한 마력이 감지됐다고 알려준다.

진행:

``` text
지하철 이동
↓
역무원과 대화
↓
정전 구역 조사
↓
몬스터 처치
↓
마지막 열차 진입
↓
보스 전투
↓
큰 검은 조각 획득
↓
레이나 등장
```

## Boss

`BOSS_BLACK_CONDUCTOR`

공격:

-   Electric Attack
-   Teleport
-   Monster Summon
-   Rush

Drop:

`ITEM_LARGE_BLACK_FRAGMENT`

------------------------------------------------------------------------

# 23. 레이나

## NPC ID

`NPC_REINA`

보스 처치 직후 등장한다.

레이나는 큰 검은 조각을 가져가려고 한다.

예시 대사:

``` text
주인공:
"그거 내려놔!"

레이나:
"너 이게 뭔지도 모르잖아."

주인공:
"그러는 너는 알아?"

레이나:
"조금은."
```

레이나는 조각을 가지고 떠난다.

Next Quest:

`MQ_006_FIND_REINA`

이 시점부터 플레이어는 괴물뿐 아니라 레이나와 검은 조각의 목적도
추적한다.

------------------------------------------------------------------------

# 24. 퀘스트 시스템

퀘스트는 Data Driven 방식으로 만든다.

퀘스트마다 전용 코드를 새로 작성하는 방식은 피한다.

## Quest State

``` text
Locked
Available
Active
Completed
```

## Quest Step State

``` text
Inactive
Active
Completed
```

## Quest Data

``` text
QuestID
QuestName
Description
PrerequisiteQuestID
StartNPC
Steps
Rewards
NextQuestID
```

## Step Type

``` text
Talk
GoToLocation
Interact
Collect
Kill
Escape
BossKill
ReturnToNPC
```

------------------------------------------------------------------------

# 25. Quest Manager 권장 기능

기존 시스템이 없다면 중앙 Quest Manager 또는 Quest Component를 추가한다.

필요 함수 예:

``` text
StartQuest(QuestID)
GetQuestState(QuestID)
AdvanceQuestStep(QuestID)
CompleteQuest(QuestID)

OnNPCDialogue(NPCID)
OnLocationEntered(LocationID)
OnItemCollected(ItemID, Amount)
OnMonsterKilled(MonsterID)
OnBossKilled(BossID)
```

Event Dispatcher / Delegate를 사용해 다른 시스템과 느슨하게 연결한다.

------------------------------------------------------------------------

# 26. 아이템 시스템

Item Data:

``` text
ItemID
DisplayName
Description
ItemType
MaxStack
Icon
SellPrice
```

Item Type:

``` text
Consumable
Material
Equipment
Quest
```

------------------------------------------------------------------------

# 27. 주요 아이템

## ITEM_BLACK_FRAGMENT

스토리 핵심.

일반 판매 불가.

## ITEM_LARGE_BLACK_FRAGMENT

보스/스토리 전용.

## ITEM_MAGIC_STONE

장비 및 마법 강화 재료.

## ITEM_HP_POTION

HP 회복.

## ITEM_MP_POTION

MP 회복.

## ITEM_MONSTER_CORE

몬스터 재료.

판매/제작 확장 가능.

------------------------------------------------------------------------

# 28. 인벤토리

필수 기능:

``` text
AddItem(ItemID, Amount)
RemoveItem(ItemID, Amount)
HasItem(ItemID, Amount)
GetItemCount(ItemID)
UseItem(ItemID)
```

아이템 획득 이벤트를 Quest System이 받을 수 있어야 한다.

------------------------------------------------------------------------

# 29. 장비

초기 슬롯:

``` text
Weapon
Charm
Costume
```

## Weapon

마법봉.

Attack 또는 MagicAttack 증가.

## Charm

특수 효과.

예:

``` text
CHARM_RECOVERY
적 처치 시 HP 소량 회복

CHARM_WIND
이동 속도 증가
```

## Costume

초기에는 외형 변경 전용으로 구현 가능.

------------------------------------------------------------------------

# 30. 상점

Phase 2에서 `NPC_MIA`에 추가.

기능:

``` text
Buy
Sell
Upgrade
```

판매 가능:

-   HP Potion
-   MP Potion
-   Magic Stone
-   Weapon
-   Charm

검은 조각 판매 금지.

------------------------------------------------------------------------

# 31. 성장

몬스터 처치 시 EXP 획득.

레벨업:

``` text
MaxHP 증가
MaxMP 증가
Attack 증가
MagicAttack 증가
```

초기에는 Skill Tree를 만들지 않는다.

새로운 주요 스킬은 보스/메인 퀘스트 보상으로 해금한다.

예:

``` text
시작
Magic Shot

학교
Magic Dash

공원
Star Rain

지하철
Lightning Spear

쇼핑몰
Magic Shield
```

------------------------------------------------------------------------

# 32. 서브 퀘스트

## SQ_001_LOST_BAG

NPC의 가방 찾기.

Reward:

Gold.

## SQ_002_NEED_MEDICINE

약 전달.

Reward:

HP Potion.

## SQ_003_NIGHT_ESCORT

NPC를 목적지까지 호위.

이동 중 몬스터 Spawn.

Reward:

Magic Stone.

## SQ_004_STRANGE_PHOTO

사진에 찍힌 장소 조사.

숨겨진 몬스터 처치.

Reward:

Charm.

------------------------------------------------------------------------

# 33. UI

## HUD

표시:

``` text
HP
MP
현재 Quest Objective
```

Phase 2:

``` text
Ultimate Gauge
```

## Quest Tracker

예:

``` text
[연락이 끊긴 친구]

유나를 찾으세요.

그림자 처치: 2 / 3
```

## Interaction

상호작용 가능한 대상 근처에서:

``` text
[E] 대화
[E] 조사
[E] 획득
```

실제 키는 기존 프로젝트 Input Mapping을 따른다.

------------------------------------------------------------------------

# 34. Save Data

최소 저장 항목:

``` text
PlayerLevel
EXP
Gold
Inventory
Equipment
UnlockedSkills
CompletedQuestIDs
ActiveQuestID
CurrentQuestStep
UnlockedAreas
CanTransform
```

기존 SaveGame 시스템이 있다면 확장한다.

------------------------------------------------------------------------

# 35. 지역 확장 규칙

새로운 지역을 추가할 때 최소 구성:

``` text
NPC 1~2명
새 몬스터 1종
기존 몬스터 변형
메인 퀘스트 1~2개
서브 퀘스트 1~3개
보스 1종
신규 아이템 또는 스킬 1개
다음 지역으로 이어지는 단서 1개
```

추천 확장 순서:

``` text
집
↓
학교
↓
골목
↓
공원
↓
지하철
↓
쇼핑몰
↓
놀이공원
↓
병원
↓
폐공장
↓
항구
↓
숲
↓
다른 도시
```

------------------------------------------------------------------------

# 36. 스토리 연결 규칙

새 지역을 갑자기 해금하지 않는다.

항상 이전 퀘스트의 결과가 다음 지역으로 이어져야 한다.

예:

``` text
유나 실종
↓
괴물 발견
↓
검은 조각 발견
↓
학교에서도 같은 조각 발견
↓
미아에게 조사 의뢰
↓
공원에서 같은 마력 발견
↓
공원 조사
↓
지하철에서 더 큰 반응 발견
↓
지하철 조사
↓
레이나 등장
↓
레이나 추적
↓
다음 지역
```

------------------------------------------------------------------------

# 37. Unreal 구현 권장 구조

> 기존 프로젝트 구조를 먼저 확인하고, 이름은 프로젝트에 맞게 조정한다.

필요할 경우 다음 구조를 참고한다.

``` text
Player
 ├─ Player Character
 ├─ Combat Component
 ├─ Transformation Component
 ├─ Inventory Component
 └─ Quest Component

NPC
 ├─ NPC Base
 ├─ Lumi
 ├─ Yuna
 ├─ Jisoo
 ├─ Mia
 └─ Reina

Enemy
 ├─ Monster Base
 ├─ Shadow
 ├─ Shooter
 ├─ Rusher
 └─ Boss Base

Quest
 ├─ Quest Manager
 ├─ Quest Data
 └─ Quest Objective Data

Item
 ├─ Item Data
 ├─ Pickup
 └─ Inventory

UI
 ├─ HUD
 ├─ Dialogue Widget
 ├─ Quest Tracker
 ├─ Inventory Widget
 └─ Shop Widget
```

가능하면 데이터는 DataAsset / DataTable 기반으로 관리한다.

------------------------------------------------------------------------

# 38. C++ / Blueprint 역할

## C++ 권장

공통 로직:

-   Quest System
-   Inventory
-   Item Data
-   Monster Base
-   Combat Component
-   Player Stats
-   Save Data
-   Interaction Interface

## Blueprint 권장

콘텐츠 제작:

-   개별 NPC 배치
-   NPC 대사
-   퀘스트 Trigger
-   몬스터 Spawn 위치
-   Boss 패턴 튜닝
-   VFX
-   Animation
-   Sound
-   Cutscene
-   지역 이벤트

즉:

**반복해서 사용하는 시스템은 C++**

**맵마다 달라지는 콘텐츠는 Blueprint/Data로 구성**

한다.

------------------------------------------------------------------------

# 39. Codex 구현 순서

## Step 1

현재 프로젝트 분석.

확인:

-   Unreal Engine 버전
-   Character 클래스
-   GameMode
-   PlayerController
-   Enhanced Input 사용 여부
-   기존 Enemy
-   기존 Interaction
-   기존 Inventory
-   기존 UI
-   기존 SaveGame

중복 기능을 만들지 않는다.

## Step 2

Interaction Interface 구현 또는 기존 기능 확장.

## Step 3

NPC Base + Dialogue 연결.

## Step 4

Quest Data + Quest Manager 구현.

## Step 5

`MQ_001_MISSING_FRIEND`를 실제 플레이 가능하게 연결.

## Step 6

검은 조각 Pickup + Inventory 연결.

## Step 7

Transformation 기능 연결.

## Step 8

Magic Shot + Dodge 구현.

## Step 9

Shadow AI 구현.

## Step 10

Shadow Eater Boss 구현.

## Step 11

Quest Reward 및 `MQ_002` 자동 해금.

## Step 12

Save/Load 연결.

## Step 13

MVP 전체 플레이 테스트.

Phase 1이 정상 동작하기 전에는 공원/지하철 콘텐츠를 먼저 만들지 않는다.

------------------------------------------------------------------------

# 40. MVP 완료 조건

아래 흐름이 처음부터 끝까지 중단 없이 실행되어야 한다.

``` text
게임 시작
-> 집
-> 학교 이동
-> 유나 대화
-> 방과 후
-> 지수 대화
-> 유나 조사
-> 골목 진입
-> 검은 조각 획득
-> 유나 가방 발견
-> 유나 발견
-> 괴물에게서 도망
-> 루미 등장
-> 첫 변신
-> Shadow 3마리 처치
-> Shadow Eater 보스
-> 유나 구조
-> 루미 대화
-> MQ_001 완료
-> 다음 날 학교
-> MQ_002 자동 시작
```

------------------------------------------------------------------------

# 41. 테스트 체크리스트

## Quest

-   [ ] 퀘스트를 중복 수락하지 않는다.
-   [ ] 이전 Step을 건너뛰어도 진행되지 않는다.
-   [ ] 몬스터 처치 수가 정확히 증가한다.
-   [ ] 필요한 아이템 획득 시 Step이 갱신된다.
-   [ ] 완료한 퀘스트가 다시 시작되지 않는다.
-   [ ] MQ_001 완료 후 MQ_002가 정상 해금된다.

## Player

-   [ ] 첫 변신 전에는 전투 능력을 사용할 수 없다.
-   [ ] 첫 변신 후 변신 기능이 해금된다.
-   [ ] HP 0에서 사망 처리가 정상 동작한다.
-   [ ] MP가 부족하면 특수 마법을 사용할 수 없다.
-   [ ] 회피가 정상 동작한다.

## Monster

-   [ ] 감지 범위 안에서 플레이어를 추적한다.
-   [ ] 공격 범위에서 공격한다.
-   [ ] 사망 후 공격하지 않는다.
-   [ ] 아이템을 중복 Drop하지 않는다.
-   [ ] Boss 사망 이벤트가 Quest System으로 전달된다.

## Inventory

-   [ ] 검은 조각 획득 가능.
-   [ ] Stack 수량 정상.
-   [ ] Save/Load 후 수량 유지.
-   [ ] 검은 조각 판매 불가.

## Save

-   [ ] Quest 상태 저장.
-   [ ] Inventory 저장.
-   [ ] 변신 해금 상태 저장.
-   [ ] Skill 해금 상태 저장.
-   [ ] 재실행 후 진행 상황 복원.

------------------------------------------------------------------------

# 42. 현재 구현하지 않을 것

초기 작업 범위를 과도하게 늘리지 않기 위해 아래는 나중에 구현한다.

-   최종 엔딩
-   대규모 오픈월드
-   복잡한 제작 시스템
-   복잡한 Skill Tree
-   PvP
-   멀티플레이
-   지나치게 많은 능력치
-   모든 지역 동시 구현
-   모든 몬스터 동시 구현

------------------------------------------------------------------------

# 43. 최종 방향

현재 스토리의 핵심은 다음 한 줄로 유지한다.

> 친구의 실종을 계기로 마법소녀가 된 주인공이 도시의 괴물들을 물리치며
> 검은 조각의 정체와 괴물을 만들어내는 존재를 추적한다.

새로운 업데이트는 항상 기존 이야기에 이어지도록 한다.

``` text
새 단서
-> 새 지역
-> 새 NPC
-> 새 사건
-> 새 몬스터
-> 보스
-> 검은 조각
-> 다음 단서
```

이 구조를 유지하면 엔딩을 미리 정하지 않아도 게임을 계속 확장할 수 있다.
