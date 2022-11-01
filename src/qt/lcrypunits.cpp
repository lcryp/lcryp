#include <qt/lcrypunits.h>
#include <consensus/amount.h>
#include <QStringList>
#include <cassert>
static constexpr auto MAX_DIGITS_LCR = 16;
LcRypUnits::LcRypUnits(QObject *parent):
        QAbstractListModel(parent),
        unitlist(availableUnits())
{
}
QList<LcRypUnit> LcRypUnits::availableUnits()
{
    QList<LcRypUnit> unitlist;
    unitlist.append(Unit::LCR);
    unitlist.append(Unit::mLCR);
    unitlist.append(Unit::uLCR);
    unitlist.append(Unit::ryp);
    return unitlist;
}
QString LcRypUnits::longName(Unit unit)
{
    switch (unit) {
    case Unit::LCR: return QString("LCR");
    case Unit::mLCR: return QString("mLCR");
    case Unit::uLCR: return QString::fromUtf8("µLCR (bits)");
    case Unit::ryp: return QString("ryp (ryp)");
    }
    assert(false);
}
QString LcRypUnits::shortName(Unit unit)
{
    switch (unit) {
    case Unit::LCR: return longName(unit);
    case Unit::mLCR: return longName(unit);
    case Unit::uLCR: return QString("bits");
    case Unit::ryp: return QString("ryp");
    }
    assert(false);
}
QString LcRypUnits::description(Unit unit)
{
    switch (unit) {
    case Unit::LCR: return QString("LcRyps");
    case Unit::mLCR: return QString("Milli-LcRyps (1 / 1" THIN_SP_UTF8 "000)");
    case Unit::uLCR: return QString("Micro-LcRyps (bits) (1 / 1" THIN_SP_UTF8 "000" THIN_SP_UTF8 "000)");
    case Unit::ryp: return QString("ryp (ryp) (1 / 100" THIN_SP_UTF8 "000" THIN_SP_UTF8 "000)");
    }
    assert(false);
}
qint64 LcRypUnits::factor(Unit unit)
{
    switch (unit) {
    case Unit::LCR: return 100'000'000;
    case Unit::mLCR: return 100'000;
    case Unit::uLCR: return 100;
    case Unit::ryp: return 1;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}
int LcRypUnits::decimals(Unit unit)
{
    switch (unit) {
    case Unit::LCR: return 8;
    case Unit::mLCR: return 5;
    case Unit::uLCR: return 2;
    case Unit::ryp: return 0;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}
QString LcRypUnits::format(Unit unit, const CAmount& nIn, bool fPlus, SeparatorStyle separators, bool justify)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    qint64 n = (qint64)nIn;
    qint64 coin = factor(unit);
    int num_decimals = decimals(unit);
    qint64 n_abs = (n > 0 ? n : -n);
    qint64 quotient = n_abs / coin;
    QString quotient_str = QString::number(quotient);
    if (justify) {
        quotient_str = quotient_str.rightJustified(MAX_DIGITS_LCR - num_decimals, ' ');
    }
    // Use SI-style thin space separators as these are locale independent and can't be
    QChar thin_sp(THIN_SP_CP);
    int q_size = quotient_str.size();
    if (separators == SeparatorStyle::ALWAYS || (separators == SeparatorStyle::STANDARD && q_size > 4))
        for (int i = 3; i < q_size; i += 3)
            quotient_str.insert(q_size - i, thin_sp);
    if (n < 0)
        quotient_str.insert(0, '-');
    else if (fPlus && n > 0)
        quotient_str.insert(0, '+');
    if (num_decimals > 0) {
        qint64 remainder = n_abs % coin;
        QString remainder_str = QString::number(remainder).rightJustified(num_decimals, '0');
        return quotient_str + QString(".") + remainder_str;
    } else {
        return quotient_str;
    }
}
QString LcRypUnits::formatWithUnit(Unit unit, const CAmount& amount, bool plussign, SeparatorStyle separators)
{
    return format(unit, amount, plussign, separators) + QString(" ") + shortName(unit);
}
QString LcRypUnits::formatHtmlWithUnit(Unit unit, const CAmount& amount, bool plussign, SeparatorStyle separators)
{
    QString str(formatWithUnit(unit, amount, plussign, separators));
    str.replace(QChar(THIN_SP_CP), QString(THIN_SP_HTML));
    return QString("<span style='white-space: nowrap;'>%1</span>").arg(str);
}
QString LcRypUnits::formatWithPrivacy(Unit unit, const CAmount& amount, SeparatorStyle separators, bool privacy)
{
    assert(amount >= 0);
    QString value;
    if (privacy) {
        value = format(unit, 0, false, separators, true).replace('0', '#');
    } else {
        value = format(unit, amount, false, separators, true);
    }
    return value + QString(" ") + shortName(unit);
}
bool LcRypUnits::parse(Unit unit, const QString& value, CAmount* val_out)
{
    if (value.isEmpty()) {
        return false;
    }
    int num_decimals = decimals(unit);
    QStringList parts = removeSpaces(value).split(".");
    if(parts.size() > 2)
    {
        return false;
    }
    QString whole = parts[0];
    QString decimals;
    if(parts.size() > 1)
    {
        decimals = parts[1];
    }
    if(decimals.size() > num_decimals)
    {
        return false;
    }
    bool ok = false;
    QString str = whole + decimals.leftJustified(num_decimals, '0');
    if(str.size() > 18)
    {
        return false;
    }
    CAmount retvalue(str.toLongLong(&ok));
    if(val_out)
    {
        *val_out = retvalue;
    }
    return ok;
}
QString LcRypUnits::getAmountColumnTitle(Unit unit)
{
    return QObject::tr("Amount") + " (" + shortName(unit) + ")";
}
int LcRypUnits::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return unitlist.size();
}
QVariant LcRypUnits::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if(row >= 0 && row < unitlist.size())
    {
        Unit unit = unitlist.at(row);
        switch(role)
        {
        case Qt::EditRole:
        case Qt::DisplayRole:
            return QVariant(longName(unit));
        case Qt::ToolTipRole:
            return QVariant(description(unit));
        case UnitRole:
            return QVariant::fromValue(unit);
        }
    }
    return QVariant();
}
CAmount LcRypUnits::maxMoney()
{
    return MAX_MONEY;
}
namespace {
qint8 ToQint8(LcRypUnit unit)
{
    switch (unit) {
    case LcRypUnit::LCR: return 0;
    case LcRypUnit::mLCR: return 1;
    case LcRypUnit::uLCR: return 2;
    case LcRypUnit::ryp: return 3;
    }
    assert(false);
}
LcRypUnit FromQint8(qint8 num)
{
    switch (num) {
    case 0: return LcRypUnit::LCR;
    case 1: return LcRypUnit::mLCR;
    case 2: return LcRypUnit::uLCR;
    case 3: return LcRypUnit::ryp;
    }
    assert(false);
}
}
QDataStream& operator<<(QDataStream& out, const LcRypUnit& unit)
{
    return out << ToQint8(unit);
}
QDataStream& operator>>(QDataStream& in, LcRypUnit& unit)
{
    qint8 input;
    in >> input;
    unit = FromQint8(input);
    return in;
}
