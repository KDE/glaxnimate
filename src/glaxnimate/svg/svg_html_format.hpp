#pragma once

#include "glaxnimate/lottie/lottie_html_format.hpp"
#include "glaxnimate/svg/svg_renderer.hpp"

namespace glaxnimate::io::svg {

class SvgHtmlFormat : public ImportExport
{
public:
    QString slug() const override { return "svg_html"; }
    QString name() const override { return QObject::tr("SVG Preview"); }
    QStringList extensions() const override { return {"html", "htm"}; }
    bool can_save() const override { return true; }
    bool can_open() const override { return false; }

private:
    bool on_save(QIODevice& file, const QString&,
                 model::Document* document, const QVariantMap&) override
    {
        file.write(lottie::LottieHtmlFormat::html_head(this, document, {}));
        file.write("<body><div id='animation'>");
        SvgRenderer rend(SMIL, CssFontType::FontFace);
        rend.write_document(document);
        rend.write(&file, true);
        file.write("</div></body></html>");
        return true;

    }
};

} // namespace glaxnimate::io::svg
