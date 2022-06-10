#ifndef TRACEDIALOG_H
#define TRACEDIALOG_H

#include <memory>
#include <QDialog>

namespace glaxnimate {

namespace model {
class Image;
class DocumentNode;
} // namespace model

namespace gui {

class TraceDialog : public QDialog
{
    Q_OBJECT

public:
    TraceDialog(model::Image* image, QWidget* parent = nullptr);
    ~TraceDialog();

    model::DocumentNode* created() const;
protected:
    void changeEvent ( QEvent* e ) override;
    void resizeEvent(QResizeEvent * event) override;
    void showEvent(QShowEvent * event) override;

private slots:
    void update_preview();
    void apply();
    void change_mode(int mode);
    void add_color();
    void remove_color();
    void auto_colors();
    void zoom_preview(qreal percent);
    void show_help();
    void preview_slide(int percent);
    void reset_settings();
    void color_options();
    void toggle_advanced(bool advanced);

private:
    class Private;
    std::unique_ptr<Private> d;
};

}} // namespace glaxnimate::gui

#endif // TRACEDIALOG_H
