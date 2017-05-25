#ifndef TRANSFERCONFIG_H
#define TRANSFERCONFIG_H

#include <QSettings>

class TransferConfig
{
public:
    static TransferConfig* getInstance();
    static QString getServerIP();
    static int getPort();
    static QString getClientIP();
    static int getMemLength(int memID);

    void setValue(const QString& key, const QVariant& value);
    void setValue(const QString& key, const QString& value);
    void setValue(const QString& key, int value);
    QVariant getValue(const QString& key);
    QString getStringValue(const QString& key);
    int getIntValue(const QString& key);

private:
    static TransferConfig* instance;

    QSettings* settings;

    TransferConfig();
    TransferConfig(const TransferConfig&);
    TransferConfig& operator=(const TransferConfig&);
};

#endif // TRANSFERCONFIG_H
