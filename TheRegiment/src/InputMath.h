#pragma once
namespace regiment {
struct RepeatKey {
    bool wasDown=false;
    unsigned long next=0;
    bool Poll(bool down,unsigned long now){
        bool edge=down&&!wasDown;
        bool fire=down&&(edge||static_cast<long>(now-next)>=0);
        wasDown=down;
        if(fire)next=now+(edge?350:90);
        return fire;
    }
};
}
