#ifndef LCRYP_QT_LCRYPAMOUNTFIELD_H
#define LCRYP_QT_LCRYPAMOUNTFIELD_H
#include <consensus/amount.h>
#include <qt/lcrypunits.h>
#include <QWidget>
class AmountSpinBox;
QT_BEGIN_NAMESPACE
class QValueComboBox;
QT_END_NAMESPACE
class LcRypAmountField: public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qint64 value READ value WRITE setValue NOTIFY valueChanged USER true)
public:
    explicit LcRypAmountField(QWidget *parent = nullptr);
    CAmount value(bool *value=nullptr) const;
    void setValue(const CAmount& value);
    void SetAllowEmpty(bool allow);
    void SetMinValue(const CAmount& value);
    void SetMaxValue(const CAmount& value);
    void setSingleStep(const CAmount& step);
    void setReadOnly(bool fReadOnly);
    void setValid(bool valid);
    bool validate();
    void setDisplayUnit(LcRypUnit new_unit);
    void clear();
    void setEnabled(bool fEnabled);
    QWidget *setupTabChain(QWidget *prev);
Q_SIGNALS:
    void valueChanged();
protected:
    bool eventFilter(QObject *object, QEvent *event) override;
private:
    AmountSpinBox *amount;
    QValueComboBox *unit;
private Q_SLOTS:
    void unitChanged(int idx);
};
#endif
