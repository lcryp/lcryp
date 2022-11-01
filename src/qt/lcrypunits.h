#ifndef LCRYP_QT_LCRYPUNITS_H
#define LCRYP_QT_LCRYPUNITS_H
#include <consensus/amount.h>
#include <QAbstractListModel>
#include <QDataStream>
#include <QString>
#define REAL_THIN_SP_CP 0x2009
#define REAL_THIN_SP_UTF8 "\xE2\x80\x89"
#define HTML_HACK_SP "<span style='white-space: nowrap; font-size: 6pt'> </span>"
#define THIN_SP_CP   REAL_THIN_SP_CP
#define THIN_SP_UTF8 REAL_THIN_SP_UTF8
#define THIN_SP_HTML HTML_HACK_SP
class LcRypUnits: public QAbstractListModel
{
    Q_OBJECT
public:
    explicit LcRypUnits(QObject *parent);
    enum class Unit {
        LCR,
        mLCR,
        uLCR,
        ryp
    };
    Q_ENUM(Unit)
    enum class SeparatorStyle
    {
        NEVER,
        STANDARD,
        ALWAYS
    };
    static QList<Unit> availableUnits();
    static QString longName(Unit unit);
    static QString shortName(Unit unit);
    static QString description(Unit unit);
    static qint64 factor(Unit unit);
    static int decimals(Unit unit);
    static QString format(Unit unit, const CAmount& amount, bool plussign = false, SeparatorStyle separators = SeparatorStyle::STANDARD, bool justify = false);
    static QString formatWithUnit(Unit unit, const CAmount& amount, bool plussign = false, SeparatorStyle separators = SeparatorStyle::STANDARD);
    static QString formatHtmlWithUnit(Unit unit, const CAmount& amount, bool plussign = false, SeparatorStyle separators = SeparatorStyle::STANDARD);
    static QString formatWithPrivacy(Unit unit, const CAmount& amount, SeparatorStyle separators, bool privacy);
    static bool parse(Unit unit, const QString& value, CAmount* val_out);
    static QString getAmountColumnTitle(Unit unit);
    enum RoleIndex {
        UnitRole = Qt::UserRole
    };
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    static QString removeSpaces(QString text)
    {
        text.remove(' ');
        text.remove(QChar(THIN_SP_CP));
        return text;
    }
    static CAmount maxMoney();
private:
    QList<Unit> unitlist;
};
typedef LcRypUnits::Unit LcRypUnit;
QDataStream& operator<<(QDataStream& out, const LcRypUnit& unit);
QDataStream& operator>>(QDataStream& in, LcRypUnit& unit);
#endif
