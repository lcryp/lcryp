#ifndef LCRYP_QT_INTRO_H
#define LCRYP_QT_INTRO_H
#include <QDialog>
#include <QMutex>
#include <QThread>
static const bool DEFAULT_CHOOSE_DATADIR = false;
class FreespaceChecker;
namespace interfaces {
    class Node;
}
namespace Ui {
    class Intro;
}
class Intro : public QDialog
{
    Q_OBJECT
public:
    explicit Intro(QWidget *parent = nullptr,
                   int64_t blockchain_size_gb = 0, int64_t chain_state_size_gb = 0);
    ~Intro();
    QString getDataDirectory();
    void setDataDirectory(const QString &dataDir);
    int64_t getPruneMiB() const;
    static bool showIfNeeded(bool& did_show_intro, int64_t& prune_MiB);
Q_SIGNALS:
    void requestCheck();
public Q_SLOTS:
    void setStatus(int status, const QString &message, quint64 bytesAvailable);
private Q_SLOTS:
    void on_dataDirectory_textChanged(const QString &arg1);
    void on_ellipsisButton_clicked();
    void on_dataDirDefault_clicked();
    void on_dataDirCustom_clicked();
private:
    Ui::Intro *ui;
    QThread *thread;
    QMutex mutex;
    bool signalled;
    QString pathToCheck;
    const int64_t m_blockchain_size_gb;
    const int64_t m_chain_state_size_gb;
    int64_t m_required_space_gb{0};
    uint64_t m_bytes_available{0};
    int64_t m_prune_target_gb;
    void startThread();
    void checkPath(const QString &dataDir);
    QString getPathToCheck();
    void UpdatePruneLabels(bool prune_checked);
    void UpdateFreeSpaceLabel();
    friend class FreespaceChecker;
};
#endif
