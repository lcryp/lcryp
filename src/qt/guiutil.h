#ifndef LCRYP_QT_GUIUTIL_H
#define LCRYP_QT_GUIUTIL_H
#include <consensus/amount.h>
#include <fs.h>
#include <net.h>
#include <netaddress.h>
#include <util/check.h>
#include <QApplication>
#include <QEvent>
#include <QHeaderView>
#include <QItemDelegate>
#include <QLabel>
#include <QMessageBox>
#include <QMetaObject>
#include <QObject>
#include <QProgressBar>
#include <QString>
#include <QTableView>
#include <cassert>
#include <chrono>
#include <utility>
class PlatformStyle;
class QValidatedLineEdit;
class SendCoinsRecipient;
namespace interfaces
{
    class Node;
}
QT_BEGIN_NAMESPACE
class QAbstractButton;
class QAbstractItemView;
class QAction;
class QDateTime;
class QDialog;
class QFont;
class QKeySequence;
class QLineEdit;
class QMenu;
class QPoint;
class QProgressDialog;
class QUrl;
class QWidget;
QT_END_NAMESPACE
namespace GUIUtil
{
    constexpr auto dialog_flags = Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint;
    QString dateTimeStr(const QDateTime &datetime);
    QString dateTimeStr(qint64 nTime);
    QFont fixedPitchFont(bool use_embedded_font = false);
    void setupAddressWidget(QValidatedLineEdit *widget, QWidget *parent);
    void AddButtonShortcut(QAbstractButton* button, const QKeySequence& shortcut);
    bool parseLcRypURI(const QUrl &uri, SendCoinsRecipient *out);
    bool parseLcRypURI(QString uri, SendCoinsRecipient *out);
    QString formatLcRypURI(const SendCoinsRecipient &info);
    bool isDust(interfaces::Node& node, const QString& address, const CAmount& amount);
    QString HtmlEscape(const QString& str, bool fMultiLine=false);
    QString HtmlEscape(const std::string& str, bool fMultiLine=false);
    void copyEntryData(const QAbstractItemView *view, int column, int role=Qt::EditRole);
    QList<QModelIndex> getEntryData(const QAbstractItemView *view, int column);
    bool hasEntryData(const QAbstractItemView *view, int column, int role);
    void setClipboard(const QString& str);
    void LoadFont(const QString& file_name);
    QString getDefaultDataDirectory();
    QString ExtractFirstSuffixFromFilter(const QString& filter);
    QString getSaveFileName(QWidget *parent, const QString &caption, const QString &dir,
        const QString &filter,
        QString *selectedSuffixOut);
    QString getOpenFileName(QWidget *parent, const QString &caption, const QString &dir,
        const QString &filter,
        QString *selectedSuffixOut);
    Qt::ConnectionType blockingGUIThreadConnection();
    bool isObscured(QWidget *w);
    void bringToFront(QWidget* w);
    void handleCloseWindowShortcut(QWidget* w);
    void openDebugLogfile();
    bool openLcRypConf();
    class ToolTipToRichTextFilter : public QObject
    {
        Q_OBJECT
    public:
        explicit ToolTipToRichTextFilter(int size_threshold, QObject *parent = nullptr);
    protected:
        bool eventFilter(QObject *obj, QEvent *evt) override;
    private:
        int size_threshold;
    };
    class LabelOutOfFocusEventFilter : public QObject
    {
        Q_OBJECT
    public:
        explicit LabelOutOfFocusEventFilter(QObject* parent);
        bool eventFilter(QObject* watched, QEvent* event) override;
    };
    bool GetStartOnSystemStartup();
    bool SetStartOnSystemStartup(bool fAutoStart);
    fs::path QStringToPath(const QString &path);
    QString PathToQString(const fs::path &path);
    QString NetworkToQString(Network net);
    QString ConnectionTypeToQString(ConnectionType conn_type, bool prepend_direction);
    QString formatDurationStr(std::chrono::seconds dur);
    QString FormatPeerAge(std::chrono::seconds time_connected);
    QString formatServicesStr(quint64 mask);
    QString formatPingTime(std::chrono::microseconds ping_time);
    QString formatTimeOffset(int64_t nTimeOffset);
    QString formatNiceTimeOffset(qint64 secs);
    QString formatBytes(uint64_t bytes);
    qreal calculateIdealFontSize(int width, const QString& text, QFont font, qreal minPointSize = 4, qreal startPointSize = 14);
    class ThemedLabel : public QLabel
    {
        Q_OBJECT
    public:
        explicit ThemedLabel(const PlatformStyle* platform_style, QWidget* parent = nullptr);
        void setThemedPixmap(const QString& image_filename, int width, int height);
    protected:
        void changeEvent(QEvent* e) override;
    private:
        const PlatformStyle* m_platform_style;
        QString m_image_filename;
        int m_pixmap_width;
        int m_pixmap_height;
        void updateThemedPixmap();
    };
    class ClickableLabel : public ThemedLabel
    {
        Q_OBJECT
    public:
        explicit ClickableLabel(const PlatformStyle* platform_style, QWidget* parent = nullptr);
    Q_SIGNALS:
        void clicked(const QPoint& point);
    protected:
        void mouseReleaseEvent(QMouseEvent *event) override;
    };
    class ClickableProgressBar : public QProgressBar
    {
        Q_OBJECT
    Q_SIGNALS:
        void clicked(const QPoint& point);
    protected:
        void mouseReleaseEvent(QMouseEvent *event) override;
    };
    typedef ClickableProgressBar ProgressBar;
    class ItemDelegate : public QItemDelegate
    {
        Q_OBJECT
    public:
        ItemDelegate(QObject* parent) : QItemDelegate(parent) {}
    Q_SIGNALS:
        void keyEscapePressed();
    private:
        bool eventFilter(QObject *object, QEvent *event) override;
    };
    void PolishProgressDialog(QProgressDialog* dialog);
    int TextWidth(const QFontMetrics& fm, const QString& text);
    void LogQtInfo();
    void PopupMenu(QMenu* menu, const QPoint& point, QAction* at_action = nullptr);
    QDateTime StartOfDay(const QDate& date);
    bool HasPixmap(const QLabel* label);
    QImage GetImage(const QLabel* label);
    template <typename SeparatorType>
    QStringList SplitSkipEmptyParts(const QString& string, const SeparatorType& separator)
    {
    #if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        return string.split(separator, Qt::SkipEmptyParts);
    #else
        return string.split(separator, QString::SkipEmptyParts);
    #endif
    }
    QString MakeHtmlLink(const QString& source, const QString& link);
    void PrintSlotException(
        const std::exception* exception,
        const QObject* sender,
        const QObject* receiver);
    template <typename Sender, typename Signal, typename Receiver, typename Slot>
    auto ExceptionSafeConnect(
        Sender sender, Signal signal, Receiver receiver, Slot method,
        Qt::ConnectionType type = Qt::AutoConnection)
    {
        return QObject::connect(
            sender, signal, receiver,
            [sender, receiver, method](auto&&... args) {
                bool ok{true};
                try {
                    (receiver->*method)(std::forward<decltype(args)>(args)...);
                } catch (const NonFatalCheckError& e) {
                    PrintSlotException(&e, sender, receiver);
                    ok = QMetaObject::invokeMethod(
                        qApp, "handleNonFatalException",
                        blockingGUIThreadConnection(),
                        Q_ARG(QString, QString::fromStdString(e.what())));
                } catch (const std::exception& e) {
                    PrintSlotException(&e, sender, receiver);
                    ok = QMetaObject::invokeMethod(
                        qApp, "handleRunawayException",
                        blockingGUIThreadConnection(),
                        Q_ARG(QString, QString::fromStdString(e.what())));
                } catch (...) {
                    PrintSlotException(nullptr, sender, receiver);
                    ok = QMetaObject::invokeMethod(
                        qApp, "handleRunawayException",
                        blockingGUIThreadConnection(),
                        Q_ARG(QString, "Unknown failure occurred."));
                }
                assert(ok);
            },
            type);
    }
    void ShowModalDialogAsynchronously(QDialog* dialog);
    inline bool IsEscapeOrBack(int key)
    {
        if (key == Qt::Key_Escape) return true;
#ifdef Q_OS_ANDROID
        if (key == Qt::Key_Back) return true;
#endif
        return false;
    }
}
#endif
