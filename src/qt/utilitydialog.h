#ifndef LCRYP_QT_UTILITYDIALOG_H
#define LCRYP_QT_UTILITYDIALOG_H
#include <QDialog>
#include <QWidget>
QT_BEGIN_NAMESPACE
class QMainWindow;
QT_END_NAMESPACE
namespace Ui {
    class HelpMessageDialog;
}
class HelpMessageDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HelpMessageDialog(QWidget *parent, bool about);
    ~HelpMessageDialog();
    void printToConsole();
    void showOrPrint();
private:
    Ui::HelpMessageDialog *ui;
    QString text;
private Q_SLOTS:
    void on_okButton_accepted();
};
class ShutdownWindow : public QWidget
{
    Q_OBJECT
public:
    explicit ShutdownWindow(QWidget *parent=nullptr, Qt::WindowFlags f=Qt::Widget);
    static QWidget* showShutdownWindow(QMainWindow* window);
protected:
    void closeEvent(QCloseEvent *event) override;
};
#endif
