#ifndef LCRYP_QT_COINCONTROLTREEWIDGET_H
#define LCRYP_QT_COINCONTROLTREEWIDGET_H
#include <QKeyEvent>
#include <QTreeWidget>
class CoinControlTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    explicit CoinControlTreeWidget(QWidget *parent = nullptr);
protected:
    virtual void keyPressEvent(QKeyEvent *event) override;
};
#endif
