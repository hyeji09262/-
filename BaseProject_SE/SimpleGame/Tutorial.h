#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include "Renderer.h"
#include "Dependencies/freeglut.h"

namespace Tutorial {
struct Vec { float x,y; };
struct Color { float r,g,b,a; };
struct Prop { Vec p; int kind; float radius; };
struct NPC { Vec p; const char* name; const char* line; };
struct Monster { Vec p,home; int hp,kind; float cooldown,phase; };
struct Particle { Vec p,velocity; float z,vz,life,total; Color color; };
static const int MapWidth=60,MapHeight=48,LandWidth=44;
static NPC villagers[]={
    {{32,24},"촌장 모라","숲의 파수꾼을 물리치고, 해안 제단에서 새벽 불씨를 되찾아 주세요."},
    {{29,26},"제빵사 보리","제 빵은 무기로도 쓰입니다. 품질 문제라는 소문은 사실입니다."},
    {{34,27},"어부 해솔","바다가 배를 가져갔어요. 노만이라도 돌려달라고 협상 중입니다."},
    {{33,22},"견습생 루아","스페이스 키로 가까운 적을 공격해요. 붉은 원이 차오르면 피하세요!"},
    {{28,24},"묘지기 무영","요즘 손님들이 자꾸 퇴실합니다. 관에는 환불 규정이 없는데."},
    {{36,26},"경비병 단","동쪽 바닷가에 봉인된 제단이 있습니다. 말로는 안 열리더군요."},
    {{31,29},"약초상 연두","몸이 아프면 마을 광장에서 쉬세요. 퇴비가 되기에는 아직 젊어요."},
    {{26,28},"대장장이 철수","날이 안 선다고요? 그건 검이 아니라 뒤집개입니다."},
    {{37,28},"등대지기 나루","황혼이 길어도 등불은 꺼뜨릴 수 없지요."},
    {{27,21},"목수 다온","나무가 걸어 다니기 시작했어요. 목재 배달은 편해졌네요."},
    {{35,30},"행상인 모루","저주받은 물건은 반품 불가! 저주도 상품의 일부랍니다."},
    {{30,23},"아이 별","숲의 작은 괴물은 반짝이는 마법을 무서워해요!"},
    {{38,24},"양조사 담","이 술은 오래 묵었습니다. 만든 사람이 해골이 됐거든요."},
    {{29,31},"수녀 은","돌아올 곳이 있다는 걸 잊지 마세요. 광장은 안전합니다."}
};
static constexpr int NPCCount=sizeof(villagers)/sizeof(villagers[0]);
static Renderer* renderer=nullptr;
static int width=1280,height=800,stage=0,hp=5,bossHP=10,upgrade=0,kills=0;
static Vec player={31,26},boss={30,16},shrine={43,26},beamTarget={31,26};
static bool keys[256]={},paused=false,moving=false;
static float elapsed=0,clockTime=0,attackCD=0,hurtCD=0,pulse=0,beam=0,messageTime=0,finishTime=0;
static float walkCycle=0,facing=1,healTimer=0,castPose=0;
static std::string message;
static std::vector<Prop> props;
static std::vector<Monster> monsters;
static std::vector<Particle> particles;
static const Color ink={.055f,.058f,.10f,1},gold={1,.76f,.39f,1},teal={.35f,.95f,.84f,1};
inline float Dist(Vec a,Vec b) {float x=a.x-b.x,y=a.y-b.y;return std::sqrt(x*x+y*y);}
inline Vec Project(Vec p,float z=0) {return {640+(p.x-p.y-player.x+player.y)*32,430+(p.x+p.y-player.x-player.y)*16-z};}
inline bool Visible(Vec s,float margin=130) {return s.x>-margin&&s.x<1280+margin&&s.y>-margin&&s.y<800+margin;}
inline void Tri(Vec a,Vec b,Vec c,Color col) {
    renderer->Triangle(a.x*width/1280,a.y*height/800,b.x*width/1280,b.y*height/800,
        c.x*width/1280,c.y*height/800,col.r,col.g,col.b,col.a);
}
inline void Box(float x,float y,float w,float h,Color c) {Tri({x,y},{x+w,y},{x+w,y+h},c);Tri({x,y},{x+w,y+h},{x,y+h},c);}
inline void Quad(Vec a,Vec b,Vec c,Vec d,Color col) {Tri(a,b,c,col);Tri(a,c,d,col);}
inline void Diamond(Vec p,float w,float h,Color c) {Tri({p.x-w,p.y},{p.x,p.y-h},{p.x+w,p.y},c);Tri({p.x-w,p.y},{p.x+w,p.y},{p.x,p.y+h},c);}
inline void Ellipse(Vec p,float w,float h,Color c,int segments=18) {
    for(int i=0;i<segments;++i) {
        float a=i*6.283185f/segments,b=(i+1)*6.283185f/segments;
        Tri(p,{p.x+std::cos(a)*w,p.y+std::sin(a)*h},{p.x+std::cos(b)*w,p.y+std::sin(b)*h},c);
    }
}
inline void Line(Vec a,Vec b,float w,Color c) {
    float dx=b.x-a.x,dy=b.y-a.y,len=std::sqrt(dx*dx+dy*dy);
    if(len<.01f)return;
    Vec n={-dy/len*w*.5f,dx/len*w*.5f};
    Quad({a.x+n.x,a.y+n.y},{b.x+n.x,b.y+n.y},{b.x-n.x,b.y-n.y},{a.x-n.x,a.y-n.y},c);
}
inline void Glow(Vec p,float w,float h,Color c) {
    for(int i=5;i>=1;--i) {float t=i/5.f;Color layer=c;layer.a=c.a*.14f;Ellipse(p,w*t,h*t,layer,16);}
}
inline void Shadow(Vec p,float w,float h,float length) {
    // Cast shadows are all drawn in a ground pass, before any upright objects.
    Vec s=Project(p);
    for(int i=4;i>=0;--i) {
        float spread=1+i*.13f;
        Ellipse({s.x-length*.55f,s.y+length*.21f},w*spread+length*.45f,h*spread,{.015f,.024f,.047f,.035f},16);
    }
    Ellipse(s,w*.6f,h*.65f,{.012f,.02f,.03f,.18f},14);
}
inline void Text(float x,float y,const std::string& text,Color c=gold,float size=18) {
    renderer->Text(x*width/1280,y*height/800,text,size*height/800,c.r,c.g,c.b,c.a);
}
inline void Say(const char* line) {message=line;messageTime=8;}
inline bool Path(int x,int y) {
    return (std::abs(x-31)<=1&&y>=15&&y<=34)||(std::abs(y-26)<=1&&x>=24&&x<=43)||
        (std::abs(x-19)<=1&&y>=9&&y<=37)||(std::abs(y-36)<=1&&x>=18&&x<=35);
}
inline bool Safe(Vec p) {return Dist(p,{31,26})<6;}
inline bool Walkable(Vec p) {
    if(p.x<.7f||p.x>LandWidth-.5f||p.y<.7f||p.y>MapHeight-1.5f)return false;
    for(const auto& prop:props)if(Dist(p,prop.p)<prop.radius+.22f)return false;
    return true;
}
inline void Move(Vec& p,Vec delta) {
    Vec q={p.x+delta.x,p.y};if(Walkable(q))p=q;
    q={p.x,p.y+delta.y};if(Walkable(q))p=q;
}
inline void Burst(Vec p,int amount,Color color,float z=22) {
    for(int i=0;i<amount&&particles.size()<320;++i) {
        float angle=i*2.399963f+clockTime,life=.35f+(i%7)*.08f;
        particles.push_back({p,{std::cos(angle)*1.7f,std::sin(angle)*1.7f},z,16.f+(i%5)*7,life,life,color});
    }
}
inline void Reset() {
    player={31,26};stage=0;hp=5;bossHP=10;upgrade=kills=0;paused=moving=false;
    elapsed=clockTime=attackCD=hurtCD=pulse=beam=finishTime=walkCycle=healTimer=castPose=0;
    std::fill(keys,keys+256,false);props.clear();monsters.clear();particles.clear();
    Say("잔불 마을에 오신 걸 환영해요. WASD로 움직여 금빛 표시 아래 촌장을 찾아가세요.");
    props={{{27,27},1,1},{{35,23},2,1},{{25,22},3,1},{{38,31},1,1},
        {{24,30},2,1},{{38,22},3,1},{{36,33},2,1},{{25,18},1,1},
        {{33,28},8,.55f},{{26,26},6,.65f},{{37,27},4,.32f},{{39,28.5f},5,.34f},
        {{28,28},7,.18f},{{34,25},7,.18f},{{32,21},7,.18f},{{40,26},7,.18f}};
    // Keep residents and the tutorial corridor free of decorative collision.
    props.erase(std::remove_if(props.begin(),props.end(),[](const Prop& p){
        for(const auto& npc:villagers)if(Dist(npc.p,p.p)<p.radius+.5f)return true;
        return false;
    }),props.end());
    Vec homes[]={{28,19},{34,17},{22,20},{18,12},{12,16},{9,28},
        {18,31},{24,37},{35,39},{39,33},{8,39},{17,42}};
    for(int i=0;i<12;++i)monsters.push_back({homes[i],homes[i],3,i%2,0,i*.7f});
    for(int y=2;y<MapHeight-2;++y)for(int x=2;x<LandWidth-2;++x) {
        if((x*17+y*31)%9!=0||Path(x,y))continue;
        Vec p={float(x),float(y)};
        if(Dist(p,{31,26})<8||Dist(p,boss)<4||Dist(p,shrine)<3)continue;
        bool reserved=false;for(auto m:monsters)if(Dist(p,m.home)<1.7f)reserved=true;
        if(!reserved)props.push_back({p,(x+y)%7==0?9:0,.42f});
    }
}
inline int NearestNPC() {
    int result=-1;float distance=1.65f;
    for(int i=0;i<NPCCount;++i)if(Dist(player,villagers[i].p)<distance){result=i;distance=Dist(player,villagers[i].p);}
    return result;
}
inline void Interact() {
    if(stage==2||stage==5)return;
    if(Dist(player,shrine)<(upgrade==1?4.f:2.1f)) {
        if(stage==3){stage=4;Burst(shrine,45,gold,45);Say("그림자 봉인이 풀렸습니다! 새벽 불씨를 되찾았어요. 촌장에게 돌아가세요.");}
        else if(stage<3)Say("그림자 봉인입니다. 숲의 파수꾼을 물리치고 마법을 먼저 배워야 합니다.");
        else Say("새벽 불씨는 주머니에 안전하게 있습니다. 촌장에게 돌아가세요.");
        return;
    }
    int i=NearestNPC();if(i<0)return;
    if(i==0&&stage==0){stage=1;Say(villagers[0].line);}
    else if(i==0&&stage==4){stage=5;finishTime=elapsed;Say("모라: 빛이 돌아왔군요! 세금도 돌아왔지만요. 고마워요, 마법사님.");}
    else if(i==0&&stage==3)Say("동쪽 해안 제단에 그림자 마법을 써서 새벽 불씨를 찾아주세요.");
    else Say(villagers[i].line);
}
inline void Key(unsigned char key,bool down) {
    if(key>='A'&&key<='Z')key=key-'A'+'a';
    bool fresh=down&&!keys[key];keys[key]=down;if(!fresh)return;
    if(key==27){paused=!paused;return;}if(paused)return;
    if(key=='r'&&stage==5){Reset();return;}
    if(stage==2&&(key=='1'||key=='2')) {
        upgrade=key-'0';stage=3;Burst(player,30,teal,30);
        Say(upgrade==1?"먼 손길을 배웠습니다! 해안 제단을 4타일 거리에서 열 수 있어요.":
            "가벼운 발을 배웠습니다! 이동 속도가 올랐어요. 해안 제단으로 가세요.");
    }
    if(key=='e')Interact();
}
inline void Damage() {
    if(hurtCD>0)return;
    --hp;hurtCD=1.2f;Burst(player,14,{1,.3f,.35f,1});
    if(hp<=0) {
        player={31,26};hp=5;bossHP=10;pulse=0;healTimer=0;
        for(auto& m:monsters){m.p=m.home;m.cooldown=1;}
        Say("주민들이 구해 주었습니다. 퀘스트는 유지됩니다. 붉은 공격 범위를 피하세요.");
    }
}
inline void Update(float dt) {
    if(paused||stage==2||stage==5)return;
    elapsed+=dt;clockTime+=dt;attackCD-=dt;hurtCD-=dt;beam-=dt;castPose-=dt;messageTime-=dt;
    float sx=float(keys['d'])-float(keys['a']),sy=float(keys['s'])-float(keys['w']);
    float dx=sx+sy,dy=sy-sx,len=std::sqrt(dx*dx+dy*dy);
    Vec old=player;
    if(len>0) {float speed=upgrade==2?4.5f:3.5f;Move(player,{dx/len*speed*dt,dy/len*speed*dt});if(sx!=0)facing=sx;}
    moving=Dist(old,player)>.001f;if(moving)walkCycle+=dt*10;
    if(Safe(player)&&hp<5){healTimer+=dt;if(healTimer>2){++hp;healTimer=0;Burst(player,8,teal);}}else healTimer=0;
    for(auto& m:monsters) {
        if(m.hp<=0)continue;
        m.cooldown-=dt;m.phase+=dt;
        bool chase=!Safe(player)&&Dist(player,m.p)<5.5f&&Dist(player,m.home)<8;
        Vec goal=chase?player:Vec{m.home.x+std::sin(m.phase*.7f)*1.1f,m.home.y+std::cos(m.phase*.5f)*1.1f};
        float distance=Dist(m.p,goal);
        if(distance>.45f) {
            float speed=chase?(m.kind==0?1.65f:2.1f):.5f;
            Vec before=m.p;Move(m.p,{(goal.x-m.p.x)/distance*speed*dt,(goal.y-m.p.y)/distance*speed*dt});
            if(Safe(m.p))m.p=before;
        }
        if(!Safe(player)&&Dist(player,m.p)<.9f&&m.cooldown<=0){m.cooldown=1.8f;Damage();}
    }
    if(stage==1&&Dist(player,boss)<6.5f) {
        pulse+=dt;
        if(pulse>=3.2f){pulse=0;Burst(boss,35,{1,.2f,.25f,1});if(Dist(player,boss)<3)Damage();}
    }else pulse=0;
    if(keys[' ']&&attackCD<=0) {
        attackCD=.5f;castPose=.3f;beam=.16f;
        int target=-1;float distance=6.5f;bool hitBoss=false;
        if(stage==1&&Dist(player,boss)<distance){distance=Dist(player,boss);hitBoss=true;}
        for(int i=0;i<int(monsters.size());++i)if(monsters[i].hp>0&&Dist(player,monsters[i].p)<distance) {
            target=i;distance=Dist(player,monsters[i].p);hitBoss=false;
        }
        beamTarget=hitBoss?boss:target>=0?monsters[target].p:Vec{player.x+facing*2,player.y-facing*2};
        Burst(player,10,teal,40);Burst(beamTarget,16,teal,25);
        if(hitBoss) {
            if(--bossHP<=0){stage=2;Burst(boss,40,gold,40);Say("파수꾼을 물리쳤습니다! 마법 핵심과 스킬 포인트를 얻었습니다.");}
        } else if(target>=0&&--monsters[target].hp<=0) {
            ++kills;hp=(std::min)(5,hp+1);Burst(monsters[target].p,24,gold);
        }
    }
    for(auto& p:particles){p.life-=dt;p.p.x+=p.velocity.x*dt;p.p.y+=p.velocity.y*dt;p.z+=p.vz*dt;p.vz-=65*dt;}
    particles.erase(std::remove_if(particles.begin(),particles.end(),[](const Particle& p){return p.life<=0;}),particles.end());
}
inline Vec Target() {if(stage==0||stage>=4)return villagers[0].p;if(stage<3)return boss;return shrine;}
}

#include "TutorialArt.h"

namespace Tutorial {
inline void Draw() {
    renderer->BeginWorld();
    DrawWorld();
    renderer->EndWorld(clockTime);
    // The HUD is deliberately outside the color grading / bloom pass.
    Vec target=Project(Target(),85);
    float tx=(std::max)(35.f,(std::min)(1245.f,target.x));
    float ty=(std::max)(167.f,(std::min)(643.f,target.y));
    Diamond({tx,ty+std::sin(clockTime*3)*4},7,12,gold);
    Box(20,20,730,118,{ink.r,ink.g,ink.b,.94f});Box(20,20,4,118,gold);
    Text(38,49,"잔불 마을  ·  마지막 온기",gold,23);
    const char* objectives[]={"01  촌장 모라에게 말 걸기 [E]","02  숲의 파수꾼 처치하기 [스페이스]",
        "03  스킬 강화 선택하기 [1 / 2]","04  해안 제단의 봉인 풀기 [E]",
        "05  촌장에게 새벽 불씨 전달하기 [E]","완료  ·  마을에 빛이 돌아왔습니다"};
    Text(38,81,objectives[stage],teal);
    char status[256];sprintf_s(status,"체력 %d/5    시간 %02d:%02d    목표 %.0f타일    야생 몬스터 처치 %d",
        hp,int(elapsed)/60,int(elapsed)%60,Dist(player,Target()),kills);
    Text(38,114,status,{.8f,.83f,.85f,1},16);
    Text(970,38,"싱글플레이 · 튜토리얼",gold,16);
    Text(970,62,Safe(player)?"마을 광장 · 안전 지역":player.x>41?"황혼 해안":"안개 숲 · 야생 지역",teal,16);
    DrawMinimap();
    Box(20,748,1240,34,{ink.r,ink.g,ink.b,.94f});
    Text(36,772,"WASD 이동    E 대화·상호작용    스페이스 마법    ESC 일시 정지    금빛 표시: 목표",gold,17);
    int nearestNpcIndex=NearestNPC();
    if(nearestNpcIndex>=0)Text(550,500,std::string("[E] ")+villagers[nearestNpcIndex].name,gold,18);
    else if(Dist(player,shrine)<(upgrade==1?4.f:2.1f))Text(544,500,"[E] 해안 제단");
    if(messageTime>0) {
        Box(20,677,1240,56,{ink.r,ink.g,ink.b,.95f});Text(38,711,message,teal,18);
    }
    if(stage==1&&Dist(player,boss)<6.5f) {
        Box(415,150,450,62,ink);Box(425,161,430*bossHP/10.f,7,{.85f,.25f,.33f,1});
        Text(430,196,pulse>2.1f?"파수꾼: 공격 준비! 붉은 범위 밖으로 피하세요!":"숲의 파수꾼 · 스페이스를 눌러 마법 공격",gold,16);
    }
    if(stage==2||stage==5||paused) {
        Box(0,0,1280,800,{.025f,.02f,.06f,.72f});Box(250,240,780,302,ink);
        Box(250,240,780,3,gold);
        if(paused){Text(295,303,"일시 정지",gold,27);Text(295,365,"ESC를 눌러 계속합니다. 시간도 잠시 쉬어 갑니다.");}
        else if(stage==2) {
            Text(295,295,"파수꾼 처치 · 스킬 포인트 획득",gold,26);
            Text(295,340,"기본 그림자 봉인 해제를 배웠습니다.",teal,20);
            Text(295,390,"[1] 먼 손길: 제단 상호작용 거리를 4타일로 확장");
            Text(295,435,"[2] 가벼운 발: 탐험 이동 속도 증가");
            Text(295,494,"어느 쪽을 골라도 해안 제단을 열 수 있습니다.",teal,17);
        }else {
            Text(295,295,"튜토리얼 완료 · 새벽 불씨 회수",gold,25);
            Text(295,345,"보스 처치 → 스킬 성장 → 탐험 → 보상",teal,20);
            char result[180];sprintf_s(result,"완료 시간 %02d:%02d  ·  목표 5분 이내",int(finishTime)/60,int(finishTime)%60);
            Text(295,389,result);
            Text(295,435,"모라: 빛도, 세금도 돌아왔군요. 고마워요!");
            Text(295,490,"[R] 다시 시작    창을 닫으면 종료합니다.",gold,17);
        }
    }
    renderer->Flush();
}
}
