#ifndef LCRYP_QT_SENDCOINSRECIPIENT_H
#define LCRYP_QT_SENDCOINSRECIPIENT_H
#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <consensus/amount.h>
#include <serialize.h>
#include <string>
#include <QString>
class SendCoinsRecipient
{
public:
    explicit SendCoinsRecipient() : amount(0), fSubtractFeeFromAmount(false), nVersion(SendCoinsRecipient::CURRENT_VERSION) { }
    explicit SendCoinsRecipient(const QString &addr, const QString &_label, const CAmount& _amount, const QString &_message):
        address(addr), label(_label), amount(_amount), message(_message), fSubtractFeeFromAmount(false), nVersion(SendCoinsRecipient::CURRENT_VERSION) {}
    QString address;
    QString label;
    CAmount amount;
    QString message;
    std::string sPaymentRequest;
    QString authenticatedMerchant;
    bool fSubtractFeeFromAmount;
    static const int CURRENT_VERSION = 1;
    int nVersion;
    SERIALIZE_METHODS(SendCoinsRecipient, obj)
    {
        std::string address_str, label_str, message_str, auth_merchant_str;
        SER_WRITE(obj, address_str = obj.address.toStdString());
        SER_WRITE(obj, label_str = obj.label.toStdString());
        SER_WRITE(obj, message_str = obj.message.toStdString());
        SER_WRITE(obj, auth_merchant_str = obj.authenticatedMerchant.toStdString());
        READWRITE(obj.nVersion, address_str, label_str, obj.amount, message_str, obj.sPaymentRequest, auth_merchant_str);
        SER_READ(obj, obj.address = QString::fromStdString(address_str));
        SER_READ(obj, obj.label = QString::fromStdString(label_str));
        SER_READ(obj, obj.message = QString::fromStdString(message_str));
        SER_READ(obj, obj.authenticatedMerchant = QString::fromStdString(auth_merchant_str));
    }
};
#endif
