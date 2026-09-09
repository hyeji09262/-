#pragma once
// Included after Tutorial's world state. Art remains procedural and asset-independent.
namespace Tutorial {
inline Color Tint(Color c,float factor) {return {c.r*factor,c.g*factor,c.b*factor,c.a};}
inline void Person(Vec p,Color cloak,bool wizard=false,int variant=0) {
    Vec s=Project(p);if(!Visible(s))return;
    float walk=wizard&&moving?std::sin(walkCycle)*4:0;
    float breath=std::sin(clockTime*2+variant)*.8f;
    float aim=wizard?facing:variant%2?1.f:-1.f;
    float cast=wizard&&castPose>0?1.f:0;
    Color dark=Tint(cloak,.52f),skin={.79f,.60f,.43f,1};
    // Articulated boots, legs, sleeves, hands, belt, face and layered cloak.
    Line({s.x-5,s.y-17},{s.x-6+walk,s.y-3},7,dark);
    Line({s.x+5,s.y-17},{s.x+6-walk,s.y-3},7,dark);
    Box(s.x-10+walk,s.y-5,9,5,{.14f,.12f,.16f,1});
    Box(s.x+2-walk,s.y-5,9,5,{.14f,.12f,.16f,1});
    Quad({s.x-9,s.y-36+breath},{s.x+8,s.y-36+breath},{s.x+13,s.y-12},{s.x-12,s.y-12},cloak);
    Tri({s.x-9,s.y-36+breath},{s.x-13,s.y-12},{s.x-1,s.y-12},dark);
    Tri({s.x+1,s.y-33},{s.x+10,s.y-13},{s.x+4,s.y-14},Tint(cloak,1.23f));
    Box(s.x-10,s.y-19,20,3,{.24f,.16f,.12f,1});Box(s.x-2,s.y-20,4,5,gold);
    Vec hand={s.x+aim*(15+cast*10),s.y-23-cast*13};
    Line({s.x+aim*7,s.y-33+breath},hand,7,dark);
    Ellipse(hand,3,4,skin,10);
    Line({s.x-aim*8,s.y-32},{s.x-aim*13,s.y-22+walk*.6f},6,cloak);
    Ellipse({s.x,s.y-43+breath},8,10,skin,12);
    Box(s.x-8,s.y-51+breath,16,5,{.21f,.15f,.14f,1});
    Box(s.x+aim*3,s.y-44+breath,2,2,ink);
    if(wizard) {
        Ellipse({s.x,s.y-50+breath},15,4,dark,14);
        Tri({s.x-10,s.y-51+breath},{s.x-3,s.y-77+breath},{s.x+9,s.y-51+breath},cloak);
        Tri({s.x-3,s.y-77+breath},{s.x+9,s.y-51+breath},{s.x+4,s.y-54+breath},Tint(cloak,1.3f));
        Box(s.x-9,s.y-55+breath,18,3,gold);
        Line({hand.x,hand.y+23},{hand.x+3*aim,hand.y-28},3,{.57f,.37f,.22f,1});
        Vec orb={hand.x+aim*3,hand.y-30};
        Glow(orb,cast?30.f:16.f,cast?27.f:14.f,{.22f,.9f,.8f,.45f});
        Diamond(orb,5,8,teal);Diamond(orb,2,4,{.92f,1,1,1});
    } else {
        if(variant%3==0) {Ellipse({s.x,s.y-52},11,4,dark);Box(s.x-7,s.y-59,14,7,cloak);}
        if(variant%3==1)Quad({s.x-7,s.y-47},{s.x+6,s.y-47},{s.x+5,s.y-39},{s.x-8,s.y-40},{.8f,.74f,.64f,1});
        if(variant%3==2)Box(s.x-6,s.y-32,12,16,{.69f,.62f,.5f,1});
    }
}
inline void Building(const Prop& p) {
    Vec s=Project(p.p);int variant=p.kind-1;
    Color wall=variant==1?Color{.44f,.45f,.44f,1}:Color{.63f,.51f,.38f,1};
    Color roof=variant==2?Color{.51f,.40f,.23f,1}:variant==1?Color{.26f,.32f,.38f,1}:Color{.43f,.24f,.23f,1};
    // Isometric front and side walls rather than a front-facing rectangle.
    Quad({s.x-46,s.y-22},{s.x,s.y},{s.x,s.y-61},{s.x-46,s.y-83},Tint(wall,.73f));
    Quad({s.x,s.y},{s.x+46,s.y-22},{s.x+46,s.y-83},{s.x,s.y-61},wall);
    for(int row=0;row<4;++row) {
        float y=s.y-11-row*14;
        Line({s.x,y},{s.x+46,y-22},1.5f,Tint(wall,.65f));
        Line({s.x-46,y-22},{s.x,y},1.5f,Tint(wall,.50f));
        if(variant==1)for(int k=0;k<3;++k) {
            float x=s.x+6+k*15+(row%2)*4;
            Line({x,y-(x-s.x)*.48f},{x,y-13-(x-s.x)*.48f},1,Tint(wall,.63f));
        }
    }
    if(variant!=1) {
        Line({s.x,s.y},{s.x,s.y-61},5,{.24f,.17f,.14f,1});
        Line({s.x-44,s.y-23},{s.x-44,s.y-79},4,{.24f,.17f,.14f,1});
        Line({s.x+44,s.y-23},{s.x+44,s.y-79},4,{.24f,.17f,.14f,1});
        Line({s.x+2,s.y-2},{s.x+43,s.y-78},3,{.29f,.2f,.15f,1});
    }
    Quad({s.x-5,s.y-61},{s.x-56,s.y-84},{s.x-7,s.y-122},{s.x+45,s.y-99},Tint(roof,.7f));
    Quad({s.x-5,s.y-61},{s.x+57,s.y-86},{s.x+45,s.y-99},{s.x-7,s.y-122},roof);
    for(int row=1;row<=5;++row) {
        float t=row/6.f;
        Vec a={s.x-5-2*t,s.y-61-61*t},b={s.x+57-12*t,s.y-86-13*t};
        Line(a,b,variant==2?2.3f:1.4f,Tint(roof,1.24f));
        if(variant==2)for(int k=0;k<8;++k) {
            float u=k/8.f;Vec at={a.x+(b.x-a.x)*u,a.y+(b.y-a.y)*u};
            Line(at,{at.x-2,at.y+5},.8f,Tint(roof,.8f));
        }
    }
    // Door, inset windows, warm spill and chimney.
    Quad({s.x+9,s.y-5},{s.x+25,s.y-12},{s.x+25,s.y-43},{s.x+9,s.y-36},ink);
    Line({s.x+17,s.y-9},{s.x+17,s.y-39},1,{.35f,.23f,.15f,1});
    Ellipse({s.x+22,s.y-27},1.5f,1.5f,gold,8);
    Quad({s.x-33,s.y-42},{s.x-14,s.y-33},{s.x-14,s.y-53},{s.x-33,s.y-62},gold);
    Line({s.x-24,s.y-58},{s.x-24,s.y-38},2,ink);Line({s.x-33,s.y-52},{s.x-14,s.y-43},2,ink);
    Glow({s.x-23,s.y-46},28,28,{1,.57f,.22f,.25f});
    Box(s.x+19,s.y-119,12,24,{.37f,.35f,.36f,1});Box(s.x+17,s.y-121,16,5,{.24f,.24f,.27f,1});
    for(int i=0;i<3;++i) {
        float rise=std::fmod(clockTime*13+i*18,54.f);
        Ellipse({s.x+25+std::sin(rise*.06f)*7,s.y-124-rise},5+rise*.13f,4+rise*.08f,{.53f,.50f,.56f,(1-rise/54)*.13f},12);
    }
}
inline void DrawProp(const Prop& p) {
    Vec s=Project(p.p);if(!Visible(s))return;
    if(p.kind>=1&&p.kind<=3){Building(p);return;}
    if(p.kind==0) {
        float sway=std::sin(clockTime*1.4f+p.p.x)*2;
        Box(s.x-4,s.y-46,8,46,{.25f,.20f,.17f,1});Box(s.x-2,s.y-44,2,43,{.40f,.30f,.20f,1});
        for(int layer=0;layer<3;++layer) {
            float y=s.y-21-layer*24,w=34-layer*6;
            Color c={.12f+layer*.035f,.28f+layer*.027f,.25f+layer*.025f,1};
            Tri({s.x-w+sway,s.y-18-layer*24},{s.x+sway,y-58},{s.x+w+sway,y},c);
            Tri({s.x+sway,y-58},{s.x+w+sway,y},{s.x+3+sway,y-5},Tint(c,.65f));
            Line({s.x-w*.6f+sway,y-11},{s.x-2+sway,y-43},1,{.30f,.40f,.30f,.6f});
        }
    }else if(p.kind==4) {
        Box(s.x-10,s.y-25,20,25,{.43f,.29f,.19f,1});Ellipse({s.x,s.y-25},10,4,{.58f,.4f,.25f,1});
        Box(s.x-11,s.y-21,22,3,{.2f,.23f,.26f,1});Box(s.x-11,s.y-7,22,3,{.2f,.23f,.26f,1});
        for(int i=-6;i<9;i+=5)Line({s.x+i,s.y-22},{s.x+i,s.y-2},1,{.25f,.17f,.12f,1});
    }else if(p.kind==5) {
        Box(s.x-13,s.y-24,26,24,{.50f,.35f,.23f,1});
        Line({s.x-12,s.y-23},{s.x+12,s.y-1},3,{.72f,.53f,.34f,1});
        Line({s.x+12,s.y-23},{s.x-12,s.y-1},3,{.72f,.53f,.34f,1});
    }else if(p.kind==6) {
        Box(s.x-24,s.y-22,48,17,{.36f,.25f,.18f,1});
        for(int i=-1;i<=1;i+=2) {
            Ellipse({s.x+i*18,s.y-3},8,9,{.18f,.15f,.17f,1});
            Line({s.x+i*18-6,s.y-3},{s.x+i*18+6,s.y-3},2,{.58f,.40f,.25f,1});
        }
        Line({s.x+22,s.y-12},{s.x+48,s.y-4},3,{.54f,.34f,.2f,1});
        for(int i=0;i<5;++i)Ellipse({s.x-16+i*7,s.y-26},5,6,{.68f,.43f,.22f,1});
    }else if(p.kind==7) {
        Box(s.x-2,s.y-68,4,68,{.23f,.20f,.21f,1});Line({s.x,s.y-66},{s.x+14,s.y-66},3,{.25f,.24f,.25f,1});
        Glow({s.x+12,s.y-55},37,42,{1,.59f,.18f,.37f});
        Box(s.x+7,s.y-63,11,16,{.95f,.66f,.24f,1});Box(s.x+6,s.y-64,13,3,ink);
        Line({s.x+12,s.y-63},{s.x+12,s.y-47},1,ink);
    }else if(p.kind==8) {
        Ellipse({s.x,s.y-4},22,12,{.40f,.42f,.42f,1});
        Box(s.x-22,s.y-19,44,15,{.44f,.44f,.40f,1});
        Ellipse({s.x,s.y-20},22,11,{.63f,.60f,.51f,1});Ellipse({s.x,s.y-20},15,7,{.12f,.24f,.28f,1});
        Box(s.x-22,s.y-55,4,40,{.39f,.25f,.15f,1});Box(s.x+18,s.y-55,4,40,{.39f,.25f,.15f,1});
        Box(s.x-23,s.y-56,46,5,{.47f,.31f,.18f,1});Line({s.x,s.y-52},{s.x,s.y-23},1,gold);
    }else {
        Quad({s.x-16,s.y-5},{s.x-9,s.y-23},{s.x+9,s.y-20},{s.x+18,s.y-3},{.38f,.42f,.44f,1});
        Tri({s.x-9,s.y-23},{s.x+9,s.y-20},{s.x-3,s.y-2},{.51f,.53f,.50f,1});
    }
}
inline void MonsterArt(const Monster& m) {
    Vec s=Project(m.p);if(!Visible(s))return;
    float bob=std::sin(m.phase*6)*2;
    Color c=m.kind==0?Color{.40f,.48f,.27f,1}:Color{.37f,.31f,.46f,1};
    if(m.kind==0) {
        Ellipse({s.x,s.y-12+bob},17,14-bob*.6f,c);
        Ellipse({s.x-6,s.y-17+bob},5,3,{.67f,.74f,.41f,.5f});
    }else {
        Line({s.x-9,s.y-7},{s.x-15,s.y+1+bob},4,Tint(c,.7f));Line({s.x+9,s.y-7},{s.x+15,s.y+1-bob},4,Tint(c,.7f));
        Tri({s.x-17,s.y-4},{s.x,s.y-40+bob},{s.x+17,s.y-4},c);
        Tri({s.x-10,s.y-28},{s.x-15,s.y-43+bob},{s.x-1,s.y-33},Tint(c,1.3f));
    }
    Box(s.x-8,s.y-17+bob,4,3,gold);Box(s.x+4,s.y-17+bob,4,3,gold);
    if(m.hp<3){Box(s.x-17,s.y-47,34,4,ink);Box(s.x-17,s.y-47,34*m.hp/3.f,4,{.9f,.34f,.35f,1});}
}
inline void BossArt() {
    Vec s=Project(boss);if(!Visible(s))return;
    float bob=std::sin(clockTime*2)*2;
    Tri({s.x-28,s.y},{s.x,s.y-86+bob},{s.x+28,s.y},{.21f,.18f,.25f,1});
    Tri({s.x-28,s.y},{s.x,s.y-86+bob},{s.x-4,s.y-6},{.32f,.29f,.34f,1});
    for(int side:{-1,1}) {
        Line({s.x+side*16,s.y-68},{s.x+side*26,s.y-100},4,{.54f,.43f,.30f,1});
        Line({s.x+side*24,s.y-91},{s.x+side*39,s.y-95},3,{.54f,.43f,.30f,1});
        Line({s.x+side*17,s.y-53},{s.x+side*34,s.y-28},7,{.29f,.25f,.27f,1});
        Glow({s.x+side*9,s.y-56+bob},10,7,{1,.15f,.23f,.5f});
        Box(s.x+side*9-3,s.y-58+bob,6,3,{1,.53f,.38f,1});
    }
    Diamond({s.x,s.y-29},5,9,{.78f,.40f,.70f,1});
}
inline void DrawMinimap() {
    const float x=1090,y=88,scale=2.5f;
    Box(x-10,y-8,170,146,{ink.r,ink.g,ink.b,.90f});
    Box(x,y,MapWidth*scale,MapHeight*scale,{.13f,.25f,.27f,1});
    Box(x+LandWidth*scale,y,(MapWidth-LandWidth)*scale,MapHeight*scale,{.22f,.35f,.49f,1});
    for(int yy=0;yy<MapHeight;++yy)for(int xx=0;xx<LandWidth;++xx)
        if(Path(xx,yy))Box(x+xx*scale,y+yy*scale,scale,scale,{.49f,.41f,.30f,1});
    for(const auto& n:villagers)Box(x+n.p.x*scale,y+n.p.y*scale,2,2,{.73f,.74f,.70f,1});
    for(const auto& m:monsters)if(m.hp>0)Box(x+m.p.x*scale,y+m.p.y*scale,2,2,{.91f,.36f,.35f,1});
    Vec target=Target();Diamond({x+target.x*scale,y+target.y*scale},4,4,gold);
    Ellipse({x+player.x*scale,y+player.y*scale},3,3,teal,10);
}
inline void DrawWorld() {
    Box(0,0,1280,800,{.10f,.14f,.18f,1});
    for(int sum=0;sum<MapWidth+MapHeight;++sum)for(int x=0;x<MapWidth;++x) {
        int y=sum-x;if(y<0||y>=MapHeight)continue;
        Vec s=Project({float(x),float(y)});if(!Visible(s,40))continue;
        float n=((x*7+y*3)%5)*.009f;
        Color c={.20f+n,.30f+n,.26f+n,1};
        if(x>=LandWidth)c={.14f+n*.5f,.27f+n,.35f+n,1};
        else if(x>=LandWidth-2)c={.53f+n,.47f+n,.36f+n,1};
        else if(Path(x,y))c={.39f+n,.34f+n,.29f+n,1};
        Diamond(s,32.4f,16.4f,c);
        if(x>=LandWidth) {
            float wave=std::sin(clockTime*1.6f+x*.6f+y*.8f);
            Line({s.x-18,s.y+wave*3},{s.x+8,s.y+wave*3-2},1.4f,{.53f,.74f,.77f,.15f+wave*.06f});
            if(x==LandWidth) {
                float tide=std::sin(clockTime*1.2f+y*.5f)*4;
                Line({s.x-30+tide,s.y},{s.x+tide,s.y-15},2.4f,{.80f,.86f,.79f,.50f});
                Line({s.x-27+tide,s.y+3},{s.x+3+tide,s.y-12},1,{.80f,.86f,.79f,.18f});
            }
        }else if(Path(x,y)) {
            for(int k=0;k<3;++k) {
                float ox=((x*13+y*7+k*19)%35)-17.f,oy=((x*3+y*11+k*7)%13)-6.f;
                Diamond({s.x+ox,s.y+oy},5,2,{.57f+n,.49f+n,.37f+n,.65f});
            }
        }else if(x<LandWidth-2&&(x+y)%3==0) {
            float breeze=std::sin(clockTime*1.6f+x+y)*2;
            for(int k=0;k<3;++k)Line({s.x+k*5-5,s.y+2},{s.x+k*5-6+breeze,s.y-5-k%2*3},1,{.36f,.44f,.27f,.75f});
            if((x*3+y)%17==0)Ellipse({s.x+7,s.y-3},2,2,{.78f,.55f,.37f,1},8);
        }
    }
    for(const auto& prop:props)if(Visible(Project(prop.p))) {
        float size=prop.kind>=1&&prop.kind<=3?42.f:prop.kind==0?19.f:12.f;
        Shadow(prop.p,size,size*.30f,prop.kind<=3?43.f:12.f);
        if(prop.kind==7)Glow(Project(prop.p),54,22,{1,.58f,.22f,.19f});
    }
    for(const auto& n:villagers)if(Visible(Project(n.p)))Shadow(n.p,12,5,15);
    Shadow(player,13,5,18);
    for(const auto& m:monsters)if(m.hp>0&&Visible(Project(m.p)))Shadow(m.p,15,6,13);
    if(stage<2)Shadow(boss,25,8,30);
    if(stage==1) {
        // Ground circle is sampled in world space, matching the 3-tile damage radius.
        Color warning={.95f,.20f,.30f,pulse>2.1f?.27f:.06f};
        Vec center=Project(boss);
        for(int i=0;i<40;++i) {
            float a=i*6.283185f/40,b=(i+1)*6.283185f/40;
            Vec p=Project({boss.x+std::cos(a)*3,boss.y+std::sin(a)*3});
            Vec q=Project({boss.x+std::cos(b)*3,boss.y+std::sin(b)*3});
            Tri(center,p,q,warning);
            if(i/40.f<pulse/3.2f)Line(p,q,2,{1,.40f,.33f,.75f});
        }
    }
    struct Item {float depth;int type,index;};
    std::vector<Item> items;
    for(int i=0;i<int(props.size());++i)if(Visible(Project(props[i].p)))items.push_back({props[i].p.x+props[i].p.y,0,i});
    for(int i=0;i<NPCCount;++i)items.push_back({villagers[i].p.x+villagers[i].p.y,1,i});
    items.push_back({player.x+player.y,2,0});
    if(stage<2)items.push_back({boss.x+boss.y,3,0});
    for(int i=0;i<int(monsters.size());++i)if(monsters[i].hp>0)items.push_back({monsters[i].p.x+monsters[i].p.y,4,i});
    items.push_back({shrine.x+shrine.y,5,0});
    std::stable_sort(items.begin(),items.end(),[](const Item&a,const Item&b){return a.depth<b.depth;});
    for(auto item:items) {
        if(item.type==0)DrawProp(props[item.index]);
        if(item.type==1)Person(villagers[item.index].p,{.38f+(item.index%4)*.09f,.34f+(item.index%3)*.07f,.40f,1},false,item.index);
        if(item.type==2)Person(player,hurtCD>0?gold:Color{.43f,.34f,.68f,1},true);
        if(item.type==3)BossArt();
        if(item.type==4)MonsterArt(monsters[item.index]);
        if(item.type==5) {
            Vec s=Project(shrine);
            for(int i=0;i<3;++i)Diamond({s.x,s.y-i*5},30-i*4,15-i*2,{.34f+i*.07f,.35f+i*.07f,.40f+i*.06f,1});
            Box(s.x-14,s.y-32,28,21,{.40f,.43f,.48f,1});
            Glow({s.x,s.y-49},40,52,stage>=4?Color{1,.6f,.2f,.32f}:Color{.5f,.4f,1,.3f});
            Diamond({s.x,s.y-49+std::sin(clockTime*2)*3},9,16,stage>=4?gold:teal);
            if(stage<4)for(int i=0;i<5;++i) {
                float a=clockTime+i*1.256f;
                Diamond({s.x+std::cos(a)*27,s.y-39+std::sin(a)*15},3,5,{.78f,.55f,1,.8f});
            }
        }
    }
    if(beam>0) {
        Vec a=Project(player,42),b=Project(beamTarget,24);
        Line(a,b,12,{.25f,.70f,1,.12f});Line(a,b,5,{.35f,.93f,1,.55f});Line(a,b,1.7f,{.90f,1,1,1});
        Glow(b,26,20,{.25f,.95f,1,.45f});
    }
    for(const auto& p:particles) {
        Color c=p.color;c.a=p.life/p.total;
        Vec s=Project(p.p,p.z);Diamond(s,2+c.a*2,2+c.a*2,c);
    }
    if(moving) {
        Vec s=Project(player);
        for(int i=0;i<3;++i)Ellipse({s.x-facing*(8+i*5),s.y+3+i},3+i*2.f,2,{.65f,.55f,.38f,.06f},10);
    }
    // World-anchored motes avoid the appearance of particles following the camera.
    for(int i=0;i<55;++i) {
        Vec p={float((i*13)%LandWidth),float((i*19)%MapHeight)};
        Vec s=Project(p,18+std::sin(clockTime+i)*8);
        if(Visible(s,10))Glow(s,4,4,{.92f,.70f,.33f,.28f});
    }
}
}
