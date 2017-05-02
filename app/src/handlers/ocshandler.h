#pragma once

#include <QObject>
#include <QJsonObject>

class ConfigHandler;

class OcsHandler : public QObject
{
    Q_OBJECT

public:
    explicit OcsHandler(ConfigHandler *configHandler, QObject *parent = 0);

public slots:
    bool addProviders(const QString &providerFileUrl);
    bool removeProvider(const QString &providerKey);
    bool updateAllCategories(bool force = false);
    bool updateCategories(const QString &providerKey, bool force = false);
    QJsonObject getContents(const QString &providerKeys = "", const QString &categoryKeys = "",
                            const QString &xdgTypes = "", const QString &packageTypes = "",
                            const QString &search = "", const QString &sortmode = "new", int pagesize = 25, int page = 0);
    QJsonObject getContent(const QString &providerKey, const QString &contentId);

private:
    ConfigHandler *configHandler_;
};