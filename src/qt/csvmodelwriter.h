#ifndef LCRYP_QT_CSVMODELWRITER_H
#define LCRYP_QT_CSVMODELWRITER_H
#include <QList>
#include <QObject>
QT_BEGIN_NAMESPACE
class QAbstractItemModel;
QT_END_NAMESPACE
class CSVModelWriter : public QObject
{
    Q_OBJECT
public:
    explicit CSVModelWriter(const QString &filename, QObject *parent = nullptr);
    void setModel(const QAbstractItemModel *model);
    void addColumn(const QString &title, int column, int role=Qt::EditRole);
    bool write();
private:
    QString filename;
    const QAbstractItemModel *model;
    struct Column
    {
        QString title;
        int column;
        int role;
    };
    QList<Column> columns;
};
#endif
