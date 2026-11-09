#include "LiveStemQueue.h"
#include <cstdlib>
#include <cstdint>
#include <thread>
#include <atomic>
#include <iostream>

#define CHECK(expr) do { if (!(expr)) { std::cerr << "CHECK failed: " << #expr << " line " << __LINE__ << "\\n"; std::abort(); } } while(false)

using masterlab::LiveStemQueue;
using masterlab::StereoLiveFrame;
using masterlab::SeparatedLiveFrame;

static void testFifoAndFull() {
    LiveStemQueue<StereoLiveFrame, 4> q;
    CHECK(q.empty());
    for (std::uint64_t i = 0; i < 4; ++i)
        CHECK(q.tryPush({float(i), -float(i), i, 3}));
    CHECK(!q.tryPush({42,42,42,3}));
    CHECK(q.size() == 4);
    StereoLiveFrame out{};
    for (std::uint64_t i = 0; i < 4; ++i) {
        CHECK(q.tryPop(out));
        CHECK(out.position == i && out.epoch == 3);
        CHECK(out.left == float(i) && out.right == -float(i));
    }
    CHECK(!q.tryPop(out));
    CHECK(q.empty());
}
static void testWrapAndEpochPreserved() {
    LiveStemQueue<StereoLiveFrame, 3> q;
    StereoLiveFrame out{};
    for (std::uint64_t i = 0; i < 20000; ++i) {
        CHECK(q.tryPush({.25f,-.5f,i, i<10000?4u:5u}));
        CHECK(q.tryPop(out));
        CHECK(out.position == i);
        CHECK(out.epoch == (i<10000?4u:5u));
    }
}
static void testSeparatedFrame() {
    LiveStemQueue<SeparatedLiveFrame,2> q;
    SeparatedLiveFrame x{};
    x.position=100; x.epoch=7;
    x.channels[0]=1.f; x.channels[7]=-.5f;
    CHECK(q.tryPush(x));
    SeparatedLiveFrame y{};
    CHECK(q.tryPop(y));
    CHECK(y.position==100 && y.epoch==7);
    CHECK(y.channels[0]==1.f && y.channels[7]==-.5f);
}
static void testSpscConcurrent() {
    constexpr std::uint64_t N=200000;
    LiveStemQueue<StereoLiveFrame, 128> q;
    std::atomic<bool> wrong{false};
    std::thread writer([&]{
        for(std::uint64_t i=0;i<N;) {
            if(q.tryPush({static_cast<float>(i), -static_cast<float>(i), i, 9})) ++i;
        }
    });
    std::thread reader([&]{
        StereoLiveFrame x{};
        for(std::uint64_t i=0;i<N;) {
            if(q.tryPop(x)) {
                if(x.position!=i || x.epoch!=9 || x.left!=float(i) || x.right!=-float(i)) wrong.store(true);
                ++i;
            }
        }
    });
    writer.join(); reader.join();
    CHECK(!wrong.load());
    CHECK(q.empty());
}
int main(){
    testFifoAndFull(); testWrapAndEpochPreserved(); testSeparatedFrame(); testSpscConcurrent();
    std::cout << "LiveStemQueueTests OK\n";
}
