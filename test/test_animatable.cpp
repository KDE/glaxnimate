/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QTest>
#include <QPoint>
#include <QMetaProperty>

#include "glaxnimate/model/animation/animatable.hpp"
#include "glaxnimate/model/object.hpp"
#include "glaxnimate/model/document.hpp"

using namespace glaxnimate::model;
using namespace glaxnimate;


#define ASSERT_WRAP(code, ret) \
do {\
        if (!code)\
        return ret;\
} while (false)
/*
#define ASSERT_FORWARD_NOEXP(actual, expected) \
    ASSERT_WRAP(QTest::qCompare(actual, expected, (std::string(actual_name) + #actual).c_str(), #expected, file, line), false)


#define KEYFRAME_COMPARE(a, ...) \
    ASSERT_WRAP(assert_keyframe(a, __VA_ARGS__, #a, __FILE__, __LINE__), void())
*/

#define KEYFRAME_COMPARE(kf, t, value, trans) \
    QCOMPARE((kf)->time(), t); \
    QCOMPARE((kf)->get(), value); \
    QCOMPARE((kf)->transition().before(), (trans).before()); \
    QCOMPARE((kf)->transition().after(), (trans).after());


class MetaTestSubject : public Object
{
    Q_OBJECT
    GLAXNIMATE_ANIMATABLE(int, anim_int, 1)
    GLAXNIMATE_ANIMATABLE(float, anim_float, 1.)
    GLAXNIMATE_ANIMATABLE(QPointF, anim_point, {})
public:
    using Object::Object;
};


#define PROPERTY_KEYFRAMES(T, prop, ...) \
    do { \
        std::unique_ptr<Keyframe<T>> kfs[] = {__VA_ARGS__}; \
        QCOMPARE(prop.keyframe_count(), sizeof(kfs) / sizeof(kfs[0])); \
        for ( int i = 0; i < prop.keyframe_count(); i++ ) { \
            QCOMPARE(prop.keyframe(i)->time(), kfs[i]->time()); \
            QCOMPARE(prop.keyframe(i)->get(), kfs[i]->get()); \
        } \
    } while(false)

template<class T>
std::unique_ptr<Keyframe<T>> newkf(FrameTime time, T val, KeyframeTransition trans = {})
{
    auto p = std::make_unique<Keyframe<T>>(time, val);
    p->set_transition(trans);
    return p;
}

class TestAnimatable: public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void test_int_basics()
    {
        MetaTestSubject ts(new Document(""));
        auto& property = ts.anim_int;
        using type = int;

        QCOMPARE(property.keyframe_count(), 0);
        QCOMPARE(property.animated(), false);

        property.set_keyframe(10, 100);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(10, 100));
        QCOMPARE(property.animated(), true);

        property.set_keyframe(5, 200);
        property.set_keyframe(20, 300);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(5, 200), newkf<type>(10, 100), newkf<type>(20, 300));

        property.set_keyframe(10, 400);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(5, 200), newkf<type>(10, 400), newkf<type>(20, 300));

        KEYFRAME_COMPARE(property.keyframe(property.keyframe_index(5)), 5, 200, KeyframeTransition());
        KEYFRAME_COMPARE(property.keyframe(property.keyframe_index(10)), 10, 400, KeyframeTransition());

        // keyframe_containing
        KEYFRAME_COMPARE(property.keyframe(property.keyframe_index(99)), 20, 300, KeyframeTransition());
        KEYFRAME_COMPARE(property.keyframe(property.keyframe_index(-99)), 5, 200, KeyframeTransition());
        KEYFRAME_COMPARE(property.keyframe(property.keyframe_index(10)), 10, 400, KeyframeTransition());
        KEYFRAME_COMPARE(property.keyframe(property.keyframe_index(12)), 10, 400, KeyframeTransition());

        property.move_keyframe(0, 25);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(10, 400), newkf<type>(20, 300), newkf<type>(25, 200));
        property.move_keyframe(2, 15);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(10, 400), newkf<type>(15, 200), newkf<type>(20, 300));
        property.move_keyframe(1, 5);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(5, 200), newkf<type>(10, 400), newkf<type>(20, 300));

        property.remove_keyframe_at_time(10);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(5, 200), newkf<type>(20, 300));

        property.remove_keyframe_at_time(10);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(5, 200), newkf<type>(20, 300));

        QCOMPARE(property.animated(), true);
        property.clear_keyframes();
        QCOMPARE(property.keyframe_count(), 0);
        QCOMPARE(property.animated(), false);
    }

    void test_float_basics()
    {
        MetaTestSubject ts(new Document(""));
        auto& property = ts.anim_float;
        using type = float;

        QCOMPARE(property.keyframe_count(), 0);
        QCOMPARE(property.animated(), false);

        property.set_keyframe(10, 100);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(10, 100));
        QCOMPARE(property.animated(), true);

        property.set_keyframe(5, 200);
        property.set_keyframe(20, 300);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(5, 200), newkf<type>(10, 100), newkf<type>(20, 300));

        property.set_keyframe(10, 400);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(5, 200), newkf<type>(10, 400), newkf<type>(20, 300));

        KEYFRAME_COMPARE(property.keyframe(property.keyframe_index(5)), 5, 200, KeyframeTransition());
        KEYFRAME_COMPARE(property.keyframe(property.keyframe_index(10)), 10, 400, KeyframeTransition());

        // keyframe_containing
        KEYFRAME_COMPARE(property.keyframe(property.keyframe_index(99)), 20, 300, KeyframeTransition());
        KEYFRAME_COMPARE(property.keyframe(property.keyframe_index(-99)), 5, 200, KeyframeTransition());
        KEYFRAME_COMPARE(property.keyframe(property.keyframe_index(10)), 10, 400, KeyframeTransition());
        KEYFRAME_COMPARE(property.keyframe(property.keyframe_index(12)), 10, 400, KeyframeTransition());

        property.move_keyframe(0, 25);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(10, 400), newkf<type>(20, 300), newkf<type>(25, 200));
        property.move_keyframe(2, 15);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(10, 400), newkf<type>(15, 200), newkf<type>(20, 300));
        property.move_keyframe(1, 5);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(5, 200), newkf<type>(10, 400), newkf<type>(20, 300));

        property.remove_keyframe_at_time(10);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(5, 200), newkf<type>(20, 300));

        property.remove_keyframe_at_time(10);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(5, 200), newkf<type>(20, 300));

        QCOMPARE(property.animated(), true);
        property.clear_keyframes();
        QCOMPARE(property.keyframe_count(), 0);
        QCOMPARE(property.animated(), false);
    }

    void test_float_transitions()
    {
        MetaTestSubject ts(new Document(""));
        auto& property = ts.anim_float;
        qreal time_offset = 0;

        property.set_time(5 + time_offset);
        QCOMPARE(property.get(), 1);

        property.set_keyframe(0, 100);
        QCOMPARE(property.get(), 100);

        property.set_keyframe(10, 200);
        QCOMPARE(property.get(), 150);

        property.set_keyframe(10, 300);
        QCOMPARE(property.get(), 200);

        property.set_time(2.5 + time_offset);
        QCOMPARE(property.get(), 150);
        QCOMPARE(property.get_at(7.5 + time_offset), 250);

        property.keyframe(0)->set_transition(KeyframeTransition(QPointF(.5, 0), QPointF(.5, 1)));
        KEYFRAME_COMPARE(property.keyframe(0), 0, 100, KeyframeTransition(QPointF(.5, 0), QPointF(.5, 1)));
        QCOMPARE(property.get_at(5 + time_offset), 200);
        qreal offset = 2118;
        QCOMPARE(qRound(property.get_at(7.5 + time_offset)*100), 30000-offset);
        property.set_time(0);property.set_time(2.5 + time_offset); // TODO FIXME This shouldn't be here
        QCOMPARE(qRound(property.get_at(2.5 + time_offset)*100), 10000+offset);
        QCOMPARE(qRound(property.get()*100), 10000+offset);


    }

    void test_int_transitions()
    {
        MetaTestSubject ts(new Document(""));
        auto& property = ts.anim_int;
        using type = int;
        qreal time_offset = 0.02; // prevents rounding issues

        property.set_time(5 + time_offset);
        QCOMPARE(property.get(), 1);

        property.set_keyframe(0, 100);
        QCOMPARE(property.get(), 100);

        property.set_keyframe(10, 200);
        QCOMPARE(property.get(), 150);

        property.set_keyframe(10, 300);
        QCOMPARE(property.get(), 200);

        property.set_time(2.5 + time_offset);
        QCOMPARE(property.get(), 150);
        QCOMPARE(property.get_at(7.5 + time_offset), 250);

        property.keyframe(0)->set_transition(KeyframeTransition(QPointF(.5, 0), QPointF(.5, 1)));
        KEYFRAME_COMPARE(property.keyframe(0), 0, 100, KeyframeTransition(QPointF(.5, 0), QPointF(.5, 1)));
        QCOMPARE(property.get_at(5 + time_offset), 200);
        QCOMPARE(property.get_at(7.5 + time_offset), 279);
        property.set_time(0);property.set_time(2.5 + time_offset); // TODO FIXME This shouldn't be here
        QCOMPARE(property.get_at(2.5 + time_offset), 121);
        QCOMPARE(property.get(), 121);


    }

};

QTEST_GUILESS_MAIN(TestAnimatable)
#include "test_animatable.moc"
