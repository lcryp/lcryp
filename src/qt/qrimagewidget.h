#ifndef LCRYP_QT_QRIMAGEWIDGET_H
#define LCRYP_QT_QRIMAGEWIDGET_H
#include <QImage>
#include <QLabel>
static const int MAX_URI_LENGTH = 255;
static constexpr int QR_IMAGE_SIZE = 300;
static constexpr int QR_IMAGE_TEXT_MARGIN = 10;
static constexpr int QR_IMAGE_MARGIN = 2 * QR_IMAGE_TEXT_MARGIN;
QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE
class QRImageWidget : public QLabel
{
    Q_OBJECT
public:
    explicit QRImageWidget(QWidget *parent = nullptr);
    bool setQR(const QString& data, const QString& text = "");
    QImage exportImage();
public Q_SLOTS:
    void saveImage();
    void copyImage();
protected:
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void contextMenuEvent(QContextMenuEvent *event) override;
private:
    QMenu *contextMenu;
};
#endif
