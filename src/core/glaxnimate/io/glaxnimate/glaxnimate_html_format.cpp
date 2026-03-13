/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/io/glaxnimate/glaxnimate_html_format.hpp"
#include "glaxnimate/io/glaxnimate/glaxnimate_format.hpp"
#include "glaxnimate/io/lottie/lottie_html_format.hpp"
#include "glaxnimate/io/lottie/cbor_write_json.hpp"


bool glaxnimate::io::glaxnimate::GlaxnimateHtmlFormat::on_save(
    QIODevice& file, const QString&, model::Composition* comp, const QVariantMap&
)
{
    file.write(lottie::LottieHtmlFormat::html_head(this, comp,
        "<script src='https://cdn.jsdelivr.net/npm/glaxnimate/glaxnimate.min.js'></script>"
    ));
    file.write(R"(
<body>
<canvas id="animation"></canvas>

<script>
    var glaxnimate_json = )");

    file.write(GlaxnimateFormat::to_json(comp->document()).toJson());

    file.write(R"(

    let player = new Glaxnimate.Player({
        canvas: document.getElementById("animation"),
        data: JSON.stringify(glaxnimate_json),
        format: "glaxnimate",
        autoplay: true
    });
</script>
</body></html>
)");

    return true;
}
