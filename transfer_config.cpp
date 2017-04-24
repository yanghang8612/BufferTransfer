#include "transfer_config.h"

TransferConfig* TransferConfig::getInstance()
{
    return instance;
}

QString TransferConfig::getServerIP()
{
    return instance->getStringValue("ServerAddress/ip");
}

int TransferConfig::getPort()
{
    return instance->getIntValue("ServerAddress/port");
}

int TransferConfig::getMemLength(int memID)
{
    if (memID > 6000 && memID < 6601) {
        return 1205;
    }
    return instance->getIntValue("MemLength/" + QString::number(memID));
}

void TransferConfig::setValue(const QString& key, const QVariant& value)
{
    settings->setValue(key, value);
}

void TransferConfig::setValue(const QString& key, const QString& value)
{
    settings->setValue(key, QVariant(value));
}

void TransferConfig::setValue(const QString& key, int value)
{
    settings->setValue(key, QVariant(value));
}

QVariant TransferConfig::getValue(const QString& key)
{
    return settings->value(key);
}

QString TransferConfig::getStringValue(const QString& key)
{
    return settings->value(key).toString();
}

int TransferConfig::getIntValue(const QString& key)
{
    bool ok = false;
    int value = settings->value(key).toInt(&ok);
    return (ok) ? value : -1;
}

TransferConfig* TransferConfig::instance = new TransferConfig();

TransferConfig::TransferConfig()
{
    settings = new QSettings("../conf/transfer_config.ini", QSettings::IniFormat);
    settings->setValue("ServerAddress/ip", "127.0.0.1");
    settings->setValue("ServerAddress/port", "8000");
    settings->setValue("MemLength/101", "640004");
    settings->setValue("MemLength/819", "5993");
    settings->setValue("MemLength/820", "19");
    settings->setValue("MemLength/1501", "580004");
    settings->setValue("MemLength/1601", "85600");
}

TransferConfig::TransferConfig(const TransferConfig&)
{}

