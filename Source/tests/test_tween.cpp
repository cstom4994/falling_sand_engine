
#include <cstdio>

#include "engine/game_utils/tweeny.h"

using MetaEngine::Tweeny::easing;
using std::getchar;
using std::printf;

/* Callback for the tweening */
bool print(MetaEngine::Tweeny::tween<int> &, int p);

/* Functors can also be used, allowing you to implement complex looping
 * This functor allows you to specify how many loops a tween should do */
template <typename... Ts>
struct loop {
    int count;
    bool operator()(MetaEngine::Tweeny::tween<Ts...> &t, Ts...);
};

/* If we reach the end, move backwards. Move forward if we reached the beginning */
bool yoyo(MetaEngine::Tweeny::tween<int> &t, int) {
    if (t.progress() <= 0.001f) {
        t.forward();
    }
    if (t.progress() >= 1.0f) {
        t.backward();
    }
    return false;
}

/* Looping function */
template <typename... Ts>
bool loop<Ts...>::operator()(MetaEngine::Tweeny::tween<Ts...> &t, Ts...) {
    if (t.progress() < 1.0f) return false;
    if (--count) {
        t.seek(0);
        return false;
    }
    return true;
}

/* Runs the tween with the designated easing */
template <typename EasingT>
void test(MetaEngine::Tweeny::tween<int> &tween, EasingT easing);

/* Nacro to help call test() with each type of easing */
#define EASING_TEST(tween, easing)           \
    printf("%s\n", #easing " In/Out/InOut"); \
    test(tween, easing##In);                 \
    test(tween, easing##Out);                \
    test(tween, easing##InOut)

int main() {
    /* Creates the tweening. By default tweenings use a linear easing */
    auto t = MetaEngine::Tweeny::from(0).to(100).during(100).onStep(print);

    /* Start with linear */
    printf("MetaEngine::Tweeny::easing::linear\n");
    test(t, easing::linear);

    /* Test all the other easings */
    EASING_TEST(t, MetaEngine::Tweeny::easing::quadratic);
    EASING_TEST(t, MetaEngine::Tweeny::easing::cubic);
    EASING_TEST(t, MetaEngine::Tweeny::easing::quartic);
    EASING_TEST(t, MetaEngine::Tweeny::easing::quintic);
    EASING_TEST(t, MetaEngine::Tweeny::easing::sinusoidal);
    EASING_TEST(t, MetaEngine::Tweeny::easing::exponential);
    EASING_TEST(t, MetaEngine::Tweeny::easing::circular);
    EASING_TEST(t, MetaEngine::Tweeny::easing::bounce);
    EASING_TEST(t, MetaEngine::Tweeny::easing::elastic);
    EASING_TEST(t, MetaEngine::Tweeny::easing::back);

    printf("MetaEngine::Tweeny::mutitype\n");
    auto tween = MetaEngine::Tweeny::from(0, 0.0f).to(2, 2.0f).during(100).onStep([](int i, float f) {
        printf("i=%d f=%f\n", i, f);
        return false;
    });
    while (tween.progress() < 1.0f) tween.step(1);

    /* infinite loop */
    printf("infinite loop example\n");
    auto infinitetween = MetaEngine::Tweeny::from(0).to(100).during(5).onStep(print);
    infinitetween.onStep([](MetaEngine::Tweeny::tween<int> &t, int) {
        if (t.progress() >= 1.0f) t.seek(0);
        return false;
    });
    for (int i = 0; i <= 20; i++) infinitetween.step(1);

    /* loop count */
    printf("counted loop example\n");
    auto count10tween = MetaEngine::Tweeny::from(0).to(100).during(5).onStep(print);
    count10tween.onStep(loop<int>{10});
    for (int i = 0; i <= 60; i++) count10tween.step(1);

    /* yoyo */
    printf("yoyo example\n");
    auto yoyotween = MetaEngine::Tweeny::from(0).to(100).during(5).onStep(print).onStep(yoyo);
    for (int i = 0; i <= 60; i++) yoyotween.step(1);

    return 0;
}

/* Set the easing, seek to beginning, loop until 100 */
template <typename EasingT>
void test(MetaEngine::Tweeny::tween<int> &tween, EasingT easing) {
    tween.via(0, easing);
    tween.seek(0);
    for (int i = 0; i <= 100; i++) tween.step(0.01f);
}

/* Prints a line, showing the tween value and a dot corresponding to that value */
bool print(MetaEngine::Tweeny::tween<int> &, int p) {
    printf("%+.3d |", p);                                             // 3 digits with sign
    for (int i = 0; i <= 100; i++) printf("%c", i == p ? '.' : ' ');  // prints the line
    printf("%c\n", p == 100 ? ';' : '|');
    return false;
}
