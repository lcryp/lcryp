#ifndef LCRYP_QT_LCRYPADDRESSVALIDATOR_H
#define LCRYP_QT_LCRYPADDRESSVALIDATOR_H
#include <QValidator>
class LcRypAddressEntryValidator : public QValidator
{
    Q_OBJECT
public:
    explicit LcRypAddressEntryValidator(QObject *parent);
    State validate(QString &input, int &pos) const override;
};
class LcRypAddressCheckValidator : public QValidator
{
    Q_OBJECT
public:
    explicit LcRypAddressCheckValidator(QObject *parent);
    State validate(QString &input, int &pos) const override;
};
#endif
