/*
 * SPDX-FileCopyrightText: 2026 Builder106
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QTest>
#include <QDomDocument>

#include "glaxnimate/io/lottie/lottie_format.hpp"
#include "glaxnimate/io/svg/svg_format.hpp"
#include "glaxnimate/model/assets/assets.hpp"

using namespace glaxnimate;

namespace {

// A 10x10 rectangle whose center moves from x=10 at frame 0 to x=90 at frame 30
QByteArray lottie(int in_point, int out_point, const QString& easing = "\"o\": {\"x\": [0], \"y\": [0]}, \"i\": {\"x\": [1], \"y\": [1]}")
{
    return QStringLiteral(R"({
        "v": "5.7.0", "fr": 30, "ip": %1, "op": %2, "w": 100, "h": 100,
        "layers": [{
            "ty": 4, "ind": 1, "ip": 0, "op": 30, "st": 0,
            "ks": {},
            "shapes": [
                {"ty": "rc", "s": {"a": 0, "k": [10, 10]}, "r": {"a": 0, "k": 0},
                 "p": {"a": 1, "k": [
                    {"t": 0, "s": [10, 50], %3},
                    {"t": 30, "s": [90, 50]}
                 ]}},
                {"ty": "fl", "c": {"a": 0, "k": [1, 0, 0]}, "o": {"a": 0, "k": 100}}
            ]
        }]
    })").arg(in_point).arg(out_point).arg(easing).toUtf8();
}

} // namespace

class TestSvgRenderer : public QObject
{
    Q_OBJECT

    QDomDocument export_svg(const QByteArray& json)
    {
        model::Document document;
        io::lottie::LottieFormat lottie_format;
        QDomDocument dom;
        if ( !lottie_format.load(&document, json) )
            return dom;

        io::svg::SvgFormat svg_format;
        dom.setContent(svg_format.save(document.assets()->compositions->values[0], {}));
        return dom;
    }

private Q_SLOTS:
    void test_repeater_opacity_animation()
    {
        QByteArray json = lottie(0, 30);
        json.replace("{\"ty\": \"fl\"", R"({"ty": "rp", "c": {"a": 0, "k": 3}, "o": {"a": 0, "k": 0}, "m": 1,
            "tr": {"ty": "tr", "p": {"a": 0, "k": [12, 0]}, "a": {"a": 0, "k": [0, 0]}, "s": {"a": 0, "k": [100, 100]},
                   "r": {"a": 0, "k": 0}, "eo": {"a": 0, "k": 100},
                   "so": {"a": 1, "k": [{"t": 0, "s": [100], "o": {"x": [0], "y": [0]}, "i": {"x": [1], "y": [1]}}, {"t": 30, "s": [0]}]}}},
            {"ty": "fl")");
        QVERIFY(json.contains("\"rp\""));
        auto animations = export_svg(json).elementsByTagName("animate");
        int opacity = 0;
        for ( int i = 0; i < animations.size(); i++ )
            if ( animations.at(i).toElement().attribute("attributeName") == "opacity" )
                opacity++;
        // Each copy fades with its own share of the start opacity
        QCOMPARE(opacity, 3);
    }
};

QTEST_GUILESS_MAIN(TestSvgRenderer)
#include "test_svg_renderer.moc"
