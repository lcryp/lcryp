#include <QtCore/QObject>
class Mock : public QObject
{
#if defined(TEST_MOCK)
    Q_OBJECT
#endif
    public:
    Mock();
};
