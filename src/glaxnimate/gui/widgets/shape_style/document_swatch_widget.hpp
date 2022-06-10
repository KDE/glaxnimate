#ifndef DOCUMENTSWATCHWIDGET_H
#define DOCUMENTSWATCHWIDGET_H

#include <QWidget>
#include <memory>


namespace color_widgets {
class ColorPaletteModel;
} // namespace color_widgets

namespace glaxnimate::model {
class Document;
class BrushStyle;
class NamedColor;
} // namespace glaxnimate::model

namespace glaxnimate::gui {

class DocumentSwatchWidget : public QWidget
{
    Q_OBJECT
public:
    DocumentSwatchWidget(QWidget* parent = nullptr);
    ~DocumentSwatchWidget();

    void set_document(model::Document* document);

    void add_new_color(const QColor& color);

    model::NamedColor* current_color() const;

    void set_palette_model(color_widgets::ColorPaletteModel* palette_model);

protected:
    void changeEvent ( QEvent* e ) override;

private slots:
    void swatch_link(int index, Qt::KeyboardModifiers mod);
    void swatch_add();
    void swatch_palette_color_added(int index);
    void swatch_palette_color_removed(int index);
    void swatch_palette_color_changed(int index);
    void swatch_doc_color_added(int position, model::NamedColor* color);
    void swatch_doc_color_removed(int pos);
    void swatch_doc_color_changed(int position, model::NamedColor* color);
    void swatch_menu(int index);

    void generate();
    void open();
    void save();

signals:
    void current_color_def(model::BrushStyle* def);
    void secondary_color_def(model::BrushStyle* def);
    void needs_new_color();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui

#endif // DOCUMENTSWATCHWIDGET_H
