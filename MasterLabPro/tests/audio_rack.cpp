#include "AudioRack.h"
#include <array>
#include <iostream>
int main(){
 ml::AudioRack rack;rack.prepare(48000);
 std::array<float,960> l,r;l.fill(.1f);r.fill(-.2f);
 rack.process(l.data(),r.data(),l.size());
 if(l[0]!=.1f||r.back()!=-.2f)return 1;
 ml::RackConfig c;c.stages[0]={1,6.0205999f};c.deltaId=1;
 if(!rack.submit(c))return 2;
 rack.process(l.data(),r.data(),l.size());
 if(std::abs(l.back()-.2f)>1e-6f)return 3;
 ml::RackMeters m;while(rack.poll(m)){}
 if(std::abs(m.delta.peak[0]+20.f)>.001f)return 4;
 c.stages[1]={2,-6.0205999f};if(!rack.submit(c))return 5;
 l.fill(.1f);r.fill(-.2f);rack.process(l.data(),r.data(),l.size());
 if(std::abs(l.back()-.1f)>1e-6f)return 6;
 std::swap(c.stages[0],c.stages[1]);if(!rack.submit(c))return 7;
 l.fill(.1f);rack.process(l.data(),nullptr,l.size());
 if(std::abs(l.front()-.1f)>1e-6f)return 8;
 c={};if(!rack.submit(c))return 9;
 l.fill(.1f);rack.process(l.data(),nullptr,l.size());
 l.fill(.1f);rack.process(l.data(),nullptr,l.size());
 if(l.front()!=.1f||l.back()!=.1f)return 10;
 c.stages[0]={3,0};c.stages[1]={3,0};if(rack.submit(c))return 11;
 ml::Spsc<int,3> q;int v;if(!q.push(1)||!q.push(2)||q.push(3)||!q.pop(v)||v!=1||!q.pop(v)||v!=2||q.pop(v))return 12;
 // Reinitialising the audio device preserves the last applied stage configuration.
 c={};c.stages[0]={5,6.0205999f};if(!rack.submit(c))return 13;
 l.fill(.1f);rack.process(l.data(),nullptr,l.size());
 rack.prepare(48000);l.fill(.1f);rack.process(l.data(),nullptr,l.size());
 if(std::abs(l.back()-.2f)>1e-6f)return 14;
 rack.resetWhenStopped();rack.prepare(48000);l.fill(.1f);rack.process(l.data(),nullptr,l.size());
 if(l.front()!=.1f||l.back()!=.1f)return 15;
 std::cout<<"PASS empty rack, gain chain, delta, reorder, retire, validation, queue, restart and reset\n";
}
