#ifndef LCRYP_QT_QVALUECOMBOBOX_H
#define LCRYP_QT_QVALUECOMBOBOX_H
#include <QComboBox>
#include <QVariant>
class QValueComboBox : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged USER true)
public:
    explicit QValueComboBox(QWidget *parent = nullptr);
    QVariant value() const;
    void setValue(const QVariant &value);
    void setRole(int role);
Q_SIGNALS:
    void valueChanged();
private:
    int role;
private Q_SLOTS:
    void handleSelectionChanged(int idx);
};
#endif
