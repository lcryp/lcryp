#include <qt/lcrypaddressvalidator.h>
#include <key_io.h>
LcRypAddressEntryValidator::LcRypAddressEntryValidator(QObject *parent) :
    QValidator(parent)
{
}
QValidator::State LcRypAddressEntryValidator::validate(QString &input, int &pos) const
{
    Q_UNUSED(pos);
    if (input.isEmpty())
        return QValidator::Intermediate;
    for (int idx = 0; idx < input.size();)
    {
        bool removeChar = false;
        QChar ch = input.at(idx);
        switch(ch.unicode())
        {
        case 0x200B:
        case 0xFEFF:
            removeChar = true;
            break;
        default:
            break;
        }
        if (ch.isSpace())
            removeChar = true;
        if (removeChar)
            input.remove(idx, 1);
        else
            ++idx;
    }
    QValidator::State state = QValidator::Acceptable;
    for (int idx = 0; idx < input.size(); ++idx)
    {
        int ch = input.at(idx).unicode();
        if (((ch >= '0' && ch<='9') ||
            (ch >= 'a' && ch<='z') ||
            (ch >= 'A' && ch<='Z')) &&
            ch != 'I' && ch != 'O')
        {
        }
        else
        {
            state = QValidator::Invalid;
        }
    }
    return state;
}
LcRypAddressCheckValidator::LcRypAddressCheckValidator(QObject *parent) :
    QValidator(parent)
{
}
QValidator::State LcRypAddressCheckValidator::validate(QString &input, int &pos) const
{
    Q_UNUSED(pos);
    if (IsValidDestinationString(input.toStdString())) {
        return QValidator::Acceptable;
    }
    return QValidator::Invalid;
}
