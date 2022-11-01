#ifndef LCRYP_QT_QVALIDATEDLINEEDIT_H
#define LCRYP_QT_QVALIDATEDLINEEDIT_H
#include <QLineEdit>
class QValidatedLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit QValidatedLineEdit(QWidget *parent);
    void clear();
    void setCheckValidator(const QValidator *v);
    bool isValid();
protected:
    void focusInEvent(QFocusEvent *evt) override;
    void focusOutEvent(QFocusEvent *evt) override;
private:
    bool valid;
    const QValidator *checkValidator;
public Q_SLOTS:
    void setText(const QString&);
    void setValid(bool valid);
    void setEnabled(bool enabled);
Q_SIGNALS:
    void validationDidChange(QValidatedLineEdit *validatedLineEdit);
private Q_SLOTS:
    void markValid();
    void checkValidity();
};
#endif
