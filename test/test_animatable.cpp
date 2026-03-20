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
#include "glaxnimate/command/animation_commands.hpp"

using namespace glaxnimate::command;
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


    void test_point_basics()
    {
        MetaTestSubject ts(new Document(""));
        auto& property = ts.anim_point;
        using type = QPointF;

        QCOMPARE(property.keyframe_count(), 0);
        QCOMPARE(property.animated(), false);

        property.set_keyframe(10, QPointF(100, -100));
        PROPERTY_KEYFRAMES(type, property, newkf<type>(10, QPointF(100, -100)));
        QCOMPARE(property.animated(), true);

        property.set_keyframe(5, QPointF(200, -200));
        property.set_keyframe(20, QPointF(300, -300));
        PROPERTY_KEYFRAMES(type, property, newkf<type>(5, QPointF(200, -200)), newkf<type>(10, QPointF(100, -100)), newkf<type>(20, QPointF(300, -300)));

        property.set_keyframe(10, QPointF(400, -400));
        PROPERTY_KEYFRAMES(type, property, newkf<type>(5, QPointF(200, -200)), newkf<type>(10, QPointF(400, -400)), newkf<type>(20, QPointF(300, -300)));

        KEYFRAME_COMPARE(property.keyframe(property.keyframe_index(5)), 5, QPointF(200, -200), KeyframeTransition());
        KEYFRAME_COMPARE(property.keyframe(property.keyframe_index(10)), 10, QPointF(400, -400), KeyframeTransition());

        // keyframe_containing
        KEYFRAME_COMPARE(property.keyframe(property.keyframe_index(99)), 20, QPointF(300, -300), KeyframeTransition());
        KEYFRAME_COMPARE(property.keyframe(property.keyframe_index(-99)), 5, QPointF(200, -200), KeyframeTransition());
        KEYFRAME_COMPARE(property.keyframe(property.keyframe_index(10)), 10, QPointF(400, -400), KeyframeTransition());
        KEYFRAME_COMPARE(property.keyframe(property.keyframe_index(12)), 10, QPointF(400, -400), KeyframeTransition());

        property.move_keyframe(0, 25);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(10, QPointF(400, -400)), newkf<type>(20, QPointF(300, -300)), newkf<type>(25, QPointF(200, -200)));
        property.move_keyframe(2, 15);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(10, QPointF(400, -400)), newkf<type>(15, QPointF(200, -200)), newkf<type>(20, QPointF(300, -300)));
        property.move_keyframe(1, 5);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(5, QPointF(200, -200)), newkf<type>(10, QPointF(400, -400)), newkf<type>(20, QPointF(300, -300)));

        property.remove_keyframe_at_time(10);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(5, QPointF(200, -200)), newkf<type>(20, QPointF(300, -300)));

        property.remove_keyframe_at_time(10);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(5, QPointF(200, -200)), newkf<type>(20, QPointF(300, -300)));

        QCOMPARE(property.animated(), true);
        property.clear_keyframes();
        QCOMPARE(property.keyframe_count(), 0);
        QCOMPARE(property.animated(), false);
    }


    void test_point_transitions()
    {
        MetaTestSubject ts(new Document(""));
        auto& property = ts.anim_point;
        qreal time_offset = 0;

        property.set_time(5 + time_offset);
        QCOMPARE(property.get(), QPointF());

        property.set_keyframe(0, QPointF(100, -100));
        QCOMPARE(property.get(), QPointF(100, -100));

        property.set_keyframe(10, QPointF(200, -200));
        QCOMPARE(property.get(), QPointF(150, -150));

        property.set_keyframe(10, QPointF(300, -300));
        QCOMPARE(property.get(), QPointF(200, -200));

        property.set_time(2.5 + time_offset);
        QCOMPARE(property.get(), QPointF(150, -150));
        QCOMPARE(property.get_at(7.5 + time_offset), QPointF(250, -250));

        property.keyframe(0)->set_transition(KeyframeTransition(QPointF(.5, 0), QPointF(.5, 1)));
        KEYFRAME_COMPARE(property.keyframe(0), 0, QPointF(100, -100), KeyframeTransition(QPointF(.5, 0), QPointF(.5, 1)));
        QCOMPARE(property.get_at(5 + time_offset), QPointF(200, -200));
        qreal offset = 2118;
        QCOMPARE(qRound(property.get_at(7.5 + time_offset).x()*100), 30000-offset);
        property.set_time(0);property.set_time(2.5 + time_offset); // TODO FIXME This shouldn't be here
        QCOMPARE(qRound(property.get_at(2.5 + time_offset).x()*100), 10000+offset);
        QCOMPARE(qRound(property.get().x()*100), 10000+offset);
    }

    void test_point_bezier()
    {
        MetaTestSubject ts(new Document(""));
        auto& property = ts.anim_point;
        using type = QPointF;

        property.set_keyframe(0, QPointF(0, 0));
        property.set_keyframe(100, QPointF(100, 200));
        QCOMPARE(property.get_at(50), QPointF(50, 100));
        property.keyframe(0)->set_point(math::bezier::Point(
            QPointF(0, 0),
            QPointF(0, 0),
            QPointF(0, -50)
        ));
        property.keyframe(1)->set_point(math::bezier::Point(
            QPointF(100, 200),
            QPointF(50, 200),
            QPointF(100, 200)
        ));
        QCOMPARE(qRound(property.get_at(50).x() * 100), 3806);
        QCOMPARE(qRound(property.get_at(50).y() * 100), 10108);

        math::bezier::CubicBezierSolver<QPointF> b(
            QPointF(0, 0),
            QPointF(0, -50),
            QPointF(50, 200),
            QPointF(100, 200)
        );
        QPointF p = b.solve(0.25);
        property.split_segment(0, 0.25);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(0, QPointF(0, 0)), newkf<type>(10, p), newkf<type>(100, QPointF(100, 200)));

        math::bezier::Bezier bez;
        bez.add_point(QPointF(1, 23));
        bez.add_point(QPointF(4, 56));
        bez.add_point(QPointF(8, 67));
        property.set_bezier(bez);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(0, QPointF(1, 23)), newkf<type>(10, QPointF(4, 56)), newkf<type>(100, QPointF(8, 67)));
    }

    void test_command_set_keyframe()
    {
        Document doc("");
        MetaTestSubject ts(&doc);
        auto& property = ts.anim_int;
        using type = int;

        QCOMPARE(property.animated(), false);
        auto cmd1 = new SetKeyframe(&property, 10, QVariant(123), false);
        cmd1->redo();
        QCOMPARE(property.animated(), true);
        PROPERTY_KEYFRAMES(type, property, newkf<type>(10, 123));
        cmd1->undo();
        QCOMPARE(property.animated(), false);
        cmd1->redo();

        auto cmd2 = new SetKeyframe(&property, 20, QVariant(45), false);
        cmd2->redo();
        PROPERTY_KEYFRAMES(type, property, newkf<type>(10, 123), newkf<type>(20, 45));
        cmd2->undo();
        PROPERTY_KEYFRAMES(type, property, newkf<type>(10, 123));
        cmd2->redo();

        auto cmd3 = new SetKeyframe(&property, 10, QVariant(67), false);
        cmd3->redo();
        PROPERTY_KEYFRAMES(type, property, newkf<type>(10, 67), newkf<type>(20, 45));
        cmd3->undo();
        PROPERTY_KEYFRAMES(type, property, newkf<type>(10, 123), newkf<type>(20, 45));
        cmd2->undo();
        cmd1->undo();
        cmd1->redo();
        cmd2->redo();
        cmd3->redo();
        PROPERTY_KEYFRAMES(type, property, newkf<type>(10, 67), newkf<type>(20, 45));
    }

    void test_command_remove_keyframe()
    {
        Document doc("");
        MetaTestSubject ts(&doc);
        auto& property = ts.anim_int;
        using type = int;
        property.set_keyframe(10, 100);
        property.set_keyframe(20, 120);
        property.set_keyframe(30, 130);

        auto cmd1 = new RemoveKeyframeTime(&property, 20);
        cmd1->redo();
        PROPERTY_KEYFRAMES(type, property, newkf<type>(10, 100), newkf<type>(30, 130));
        cmd1->undo();
        PROPERTY_KEYFRAMES(type, property, newkf<type>(10, 100), newkf<type>(20, 120), newkf<type>(30, 130));
        cmd1->redo();

        auto cmd2 = new RemoveKeyframeTime(&property, 10);
        cmd2->redo();
        PROPERTY_KEYFRAMES(type, property, newkf<type>(30, 130));
        cmd2->undo();
        PROPERTY_KEYFRAMES(type, property, newkf<type>(10, 100), newkf<type>(30, 130));


        auto cmd3 = new RemoveAllKeyframes(&property, 67);
        cmd3->redo();
        QCOMPARE(property.keyframe_count(), 0);
        QCOMPARE(property.get(), 67);
        cmd3->undo();
        PROPERTY_KEYFRAMES(type, property, newkf<type>(10, 100), newkf<type>(30, 130));
        cmd3->redo();


    }

    void test_command_move_keyframe()
    {
        Document doc("");
        MetaTestSubject ts(&doc);
        auto& property = ts.anim_int;
        using type = int;
        property.set_keyframe(10, 100);
        property.set_keyframe(20, 120);
        property.set_keyframe(30, 130);

        {
            auto cmd = new MoveKeyframe(&property, 1, 40);
            cmd->redo();
            PROPERTY_KEYFRAMES(type, property, newkf<type>(10, 100), newkf<type>(30, 130), newkf<type>(40, 120));
            cmd->undo();
            PROPERTY_KEYFRAMES(type, property, newkf<type>(10, 100), newkf<type>(20, 120), newkf<type>(30, 130));
        }

        {
            auto cmd = new MoveKeyframe(&property, 1, 30);
            cmd->redo();
            PROPERTY_KEYFRAMES(type, property, newkf<type>(10, 100), newkf<type>(30, 130), newkf<type>(30, 120));
            cmd->undo();
            PROPERTY_KEYFRAMES(type, property, newkf<type>(10, 100), newkf<type>(20, 120), newkf<type>(30, 130));
        }
    }

    void test_command_set_multiple()
    {
        Document doc("");
        MetaTestSubject ts(&doc);
        std::vector<model::AnimatableBase*> props = {&ts.anim_int, &ts.anim_float, &ts.anim_point};

        ts.set_time(48);
        ts.anim_int.set(4);
        ts.anim_float.set(5);
        ts.anim_point.set(QPointF(6, 7));

        QCOMPARE(ts.anim_int.get(), 4);
        QCOMPARE(ts.anim_float.get(), 5);
        QCOMPARE(ts.anim_point.get(), QPointF(6, 7));

        {
            SetMultipleAnimated cmd(QStringLiteral("test"), false, props, 1, 2, QPointF(3, 4));
            cmd.redo();
            QCOMPARE(ts.anim_int.get(), 1);
            QCOMPARE(ts.anim_float.get(), 2);
            QCOMPARE(ts.anim_point.get(), QPointF(3, 4));
            QCOMPARE(ts.anim_int.animated(), false);
            QCOMPARE(ts.anim_float.animated(), false);
            QCOMPARE(ts.anim_point.animated(), false);
            cmd.undo();
            QCOMPARE(ts.anim_int.get(), 4);
            QCOMPARE(ts.anim_float.get(), 5);
            QCOMPARE(ts.anim_point.get(), QPointF(6, 7));
            QCOMPARE(ts.anim_int.animated(), false);
            QCOMPARE(ts.anim_float.animated(), false);
            QCOMPARE(ts.anim_point.animated(), false);
        }

        {
            doc.set_record_to_keyframe(true);
            SetMultipleAnimated cmd(QStringLiteral("test"), false, props, 1, 2, QPointF(3, 4));
            cmd.redo();
            QCOMPARE(ts.anim_int.get(), 1);
            QCOMPARE(ts.anim_float.get(), 2);
            QCOMPARE(ts.anim_point.get(), QPointF(3, 4));
            QCOMPARE(ts.anim_int.animated(), true);
            QCOMPARE(ts.anim_float.animated(), true);
            QCOMPARE(ts.anim_point.animated(), true);
            PROPERTY_KEYFRAMES(int, ts.anim_int, newkf<int>(0, 4), newkf<int>(48, 1));
            PROPERTY_KEYFRAMES(float, ts.anim_float, newkf<float>(0, 5), newkf<float>(48, 2));
            PROPERTY_KEYFRAMES(QPointF, ts.anim_point, newkf<QPointF>(0, QPointF(6, 7)), newkf<QPointF>(48, QPointF(3, 4)));
            cmd.undo();
            QCOMPARE(ts.anim_int.get(), 4);
            QCOMPARE(ts.anim_float.get(), 5);
            QCOMPARE(ts.anim_point.get(), QPointF(6, 7));
            QCOMPARE(ts.anim_int.animated(), false);
            QCOMPARE(ts.anim_float.animated(), false);
            QCOMPARE(ts.anim_point.animated(), false);
            doc.set_record_to_keyframe(false);
        }

        {
            ts.anim_int.set_keyframe(10, 10);
            PROPERTY_KEYFRAMES(int, ts.anim_int, newkf<int>(10, 10));
            SetMultipleAnimated cmd(QStringLiteral("test"), false, props, 1, 2, QPointF(3, 4));
            cmd.redo();
            QCOMPARE(ts.anim_int.get(), 1);
            QCOMPARE(ts.anim_float.get(), 2);
            QCOMPARE(ts.anim_point.get(), QPointF(3, 4));
            QCOMPARE(ts.anim_int.animated(), true);
            QCOMPARE(ts.anim_float.animated(), false);
            QCOMPARE(ts.anim_point.animated(), false);
            PROPERTY_KEYFRAMES(int, ts.anim_int, newkf<int>(10, 10), newkf<int>(48, 1));
            cmd.undo();
            QCOMPARE(ts.anim_int.get(), 10);
            QCOMPARE(ts.anim_float.get(), 5);
            QCOMPARE(ts.anim_point.get(), QPointF(6, 7));
            QCOMPARE(ts.anim_int.animated(), true);
            QCOMPARE(ts.anim_float.animated(), false);
            QCOMPARE(ts.anim_point.animated(), false);
            PROPERTY_KEYFRAMES(int, ts.anim_int, newkf<int>(10, 10));
        }
    }

    void test_command_set_multiple_push()
    {
        Document doc("");
        MetaTestSubject ts(&doc);

        ts.set_time(48);
        ts.anim_int.set(4);
        ts.anim_float.set(5);
        ts.anim_point.set(QPointF(6, 7));

        QCOMPARE(ts.anim_int.get(), 4);
        QCOMPARE(ts.anim_float.get(), 5);
        QCOMPARE(ts.anim_point.get(), QPointF(6, 7));

        {
            SetMultipleAnimated cmd(QStringLiteral("test"), std::vector<model::AnimatableBase*>{}, QVariantList(), QVariantList(), false);
            cmd.push_property(&ts.anim_int, 1);
            cmd.push_property(&ts.anim_float, 2);
            cmd.push_property(&ts.anim_point, QPointF(3, 4));
            cmd.redo();
            QCOMPARE(ts.anim_int.get(), 1);
            QCOMPARE(ts.anim_float.get(), 2);
            QCOMPARE(ts.anim_point.get(), QPointF(3, 4));
            QCOMPARE(ts.anim_int.animated(), false);
            QCOMPARE(ts.anim_float.animated(), false);
            QCOMPARE(ts.anim_point.animated(), false);
            cmd.undo();
            QCOMPARE(ts.anim_int.get(), 4);
            QCOMPARE(ts.anim_float.get(), 5);
            QCOMPARE(ts.anim_point.get(), QPointF(6, 7));
            QCOMPARE(ts.anim_int.animated(), false);
            QCOMPARE(ts.anim_float.animated(), false);
            QCOMPARE(ts.anim_point.animated(), false);
        }

        {
            doc.set_record_to_keyframe(true);
            SetMultipleAnimated cmd(QStringLiteral("test"), std::vector<model::AnimatableBase*>{}, QVariantList(), QVariantList(), false);
            cmd.push_property(&ts.anim_int, 1);
            cmd.push_property(&ts.anim_float, 2);
            cmd.push_property(&ts.anim_point, QPointF(3, 4));
            cmd.redo();
            QCOMPARE(ts.anim_int.get(), 1);
            QCOMPARE(ts.anim_float.get(), 2);
            QCOMPARE(ts.anim_point.get(), QPointF(3, 4));
            QCOMPARE(ts.anim_int.animated(), true);
            QCOMPARE(ts.anim_float.animated(), true);
            QCOMPARE(ts.anim_point.animated(), true);
            PROPERTY_KEYFRAMES(int, ts.anim_int, newkf<int>(0, 4), newkf<int>(48, 1));
            PROPERTY_KEYFRAMES(float, ts.anim_float, newkf<float>(0, 5), newkf<float>(48, 2));
            PROPERTY_KEYFRAMES(QPointF, ts.anim_point, newkf<QPointF>(0, QPointF(6, 7)), newkf<QPointF>(48, QPointF(3, 4)));
            cmd.undo();
            QCOMPARE(ts.anim_int.get(), 4);
            QCOMPARE(ts.anim_float.get(), 5);
            QCOMPARE(ts.anim_point.get(), QPointF(6, 7));
            QCOMPARE(ts.anim_int.animated(), false);
            QCOMPARE(ts.anim_float.animated(), false);
            QCOMPARE(ts.anim_point.animated(), false);
            doc.set_record_to_keyframe(false);
        }

        {
            ts.anim_int.set_keyframe(10, 10);
            PROPERTY_KEYFRAMES(int, ts.anim_int, newkf<int>(10, 10));
            SetMultipleAnimated cmd(QStringLiteral("test"), std::vector<model::AnimatableBase*>{}, QVariantList(), QVariantList(), false);
            cmd.push_property(&ts.anim_int, 1);
            cmd.push_property(&ts.anim_float, 2);
            cmd.push_property(&ts.anim_point, QPointF(3, 4));
            cmd.redo();
            QCOMPARE(ts.anim_int.get(), 1);
            QCOMPARE(ts.anim_float.get(), 2);
            QCOMPARE(ts.anim_point.get(), QPointF(3, 4));
            QCOMPARE(ts.anim_int.animated(), true);
            QCOMPARE(ts.anim_float.animated(), false);
            QCOMPARE(ts.anim_point.animated(), false);
            PROPERTY_KEYFRAMES(int, ts.anim_int, newkf<int>(10, 10), newkf<int>(48, 1));
            cmd.undo();
            QCOMPARE(ts.anim_int.get(), 10);
            QCOMPARE(ts.anim_float.get(), 5);
            QCOMPARE(ts.anim_point.get(), QPointF(6, 7));
            QCOMPARE(ts.anim_int.animated(), true);
            QCOMPARE(ts.anim_float.animated(), false);
            QCOMPARE(ts.anim_point.animated(), false);
            PROPERTY_KEYFRAMES(int, ts.anim_int, newkf<int>(10, 10));
        }
    }
};

QTEST_GUILESS_MAIN(TestAnimatable)
#include "test_animatable.moc"
