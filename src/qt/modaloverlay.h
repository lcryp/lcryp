#ifndef LCRYP_QT_MODALOVERLAY_H
#define LCRYP_QT_MODALOVERLAY_H
#include <QDateTime>
#include <QPropertyAnimation>
#include <QWidget>
static constexpr int HEADER_HEIGHT_DELTA_SYNC = 24;
namespace Ui {
    class ModalOverlay;
}
class ModalOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit ModalOverlay(bool enable_wallet, QWidget *parent);
    ~ModalOverlay();
    void tipUpdate(int count, const QDateTime& blockDate, double nVerificationProgress);
    void setKnownBestHeight(int count, const QDateTime& blockDate, bool presync);
    void showHide(bool hide = false, bool userRequested = false);
    bool isLayerVisible() const { return layerIsVisible; }
public Q_SLOTS:
    void toggleVisibility();
    void closeClicked();
Q_SIGNALS:
    void triggered(bool hidden);
protected:
    bool eventFilter(QObject * obj, QEvent * ev) override;
    bool event(QEvent* ev) override;
private:
    Ui::ModalOverlay *ui;
    int bestHeaderHeight;
    QDateTime bestHeaderDate;
    QVector<QPair<qint64, double> > blockProcessTime;
    bool layerIsVisible;
    bool userClosed;
    QPropertyAnimation m_animation;
    void UpdateHeaderSyncLabel();
    void UpdateHeaderPresyncLabel(int height, const QDateTime& blockDate);
};
#endif
