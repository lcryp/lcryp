#include <qt/qvalidatedlineedit.h>
#include <qt/lcrypaddressvalidator.h>
#include <qt/guiconstants.h>
QValidatedLineEdit::QValidatedLineEdit(QWidget *parent) :
    QLineEdit(parent),
    valid(true),
    checkValidator(nullptr)
{
    connect(this, &QValidatedLineEdit::textChanged, this, &QValidatedLineEdit::markValid);
}
void QValidatedLineEdit::setText(const QString& text)
{
    QLineEdit::setText(text);
    checkValidity();
}
void QValidatedLineEdit::setValid(bool _valid)
{
    if(_valid == this->valid)
    {
        return;
    }
    if(_valid)
    {
        setStyleSheet("");
    }
    else
    {
        setStyleSheet("QValidatedLineEdit { " STYLE_INVALID "}");
    }
    this->valid = _valid;
}
void QValidatedLineEdit::focusInEvent(QFocusEvent *evt)
{
    setValid(true);
    QLineEdit::focusInEvent(evt);
}
void QValidatedLineEdit::focusOutEvent(QFocusEvent *evt)
{
    checkValidity();
    QLineEdit::focusOutEvent(evt);
}
void QValidatedLineEdit::markValid()
{
    setValid(true);
}
void QValidatedLineEdit::clear()
{
    setValid(true);
    QLineEdit::clear();
}
void QValidatedLineEdit::setEnabled(bool enabled)
{
    if (!enabled)
    {
        setValid(true);
    }
    else
    {
        checkValidity();
    }
    QLineEdit::setEnabled(enabled);
}
void QValidatedLineEdit::checkValidity()
{
    if (text().isEmpty())
    {
        setValid(true);
    }
    else if (hasAcceptableInput())
    {
        setValid(true);
        if (checkValidator)
        {
            QString address = text();
            int pos = 0;
            if (checkValidator->validate(address, pos) == QValidator::Acceptable)
                setValid(true);
            else
                setValid(false);
        }
    }
    else
        setValid(false);
    Q_EMIT validationDidChange(this);
}
void QValidatedLineEdit::setCheckValidator(const QValidator *v)
{
    checkValidator = v;
    checkValidity();
}
bool QValidatedLineEdit::isValid()
{
    if (checkValidator)
    {
        QString address = text();
        int pos = 0;
        if (checkValidator->validate(address, pos) == QValidator::Acceptable)
            return true;
    }
    return valid;
}
