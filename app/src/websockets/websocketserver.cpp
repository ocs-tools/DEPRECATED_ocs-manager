#include "websocketserver.h"

#include <QHostAddress>
#include <QWebSocketServer>
#include <QWebSocket>

#include "qtil_json.h"

#include "handlers/confighandler.h"
#include "handlers/systemhandler.h"
#include "handlers/ocsapihandler.h"
#include "handlers/itemhandler.h"
#include "handlers/updatehandler.h"
#include "handlers/desktopthemehandler.h"

WebSocketServer::WebSocketServer(ConfigHandler *configHandler, const QString &serverName, quint16 serverPort, QObject *parent)
    : QObject(parent), configHandler_(configHandler), serverName_(serverName), serverPort_(serverPort)
{
    wsServer_ = new QWebSocketServer(serverName_, QWebSocketServer::NonSecureMode, this);

    connect(wsServer_, &QWebSocketServer::newConnection, this, &WebSocketServer::wsNewConnection);
    connect(wsServer_, &QWebSocketServer::closed, this, &WebSocketServer::stopped);

    configHandler_->setParent(this);
    systemHandler_ = new SystemHandler(this);
    ocsApiHandler_ = new OcsApiHandler(configHandler_, this);
    itemHandler_ = new ItemHandler(configHandler_, this);
    updateHandler_ = new UpdateHandler(configHandler_, this);
    desktopThemeHandler_ = new DesktopThemeHandler(this);

    connect(itemHandler_, &ItemHandler::metadataSetChanged, this, &WebSocketServer::itemHandlerMetadataSetChanged);
    connect(itemHandler_, &ItemHandler::downloadStarted, this, &WebSocketServer::itemHandlerDownloadStarted);
    connect(itemHandler_, &ItemHandler::downloadFinished, this, &WebSocketServer::itemHandlerDownloadFinished);
    connect(itemHandler_, &ItemHandler::downloadProgress, this, &WebSocketServer::itemHandlerDownloadProgress);
    connect(itemHandler_, &ItemHandler::saveStarted, this, &WebSocketServer::itemHandlerSaveStarted);
    connect(itemHandler_, &ItemHandler::saveFinished, this, &WebSocketServer::itemHandlerSaveFinished);
    connect(itemHandler_, &ItemHandler::installStarted, this, &WebSocketServer::itemHandlerInstallStarted);
    connect(itemHandler_, &ItemHandler::installFinished, this, &WebSocketServer::itemHandlerInstallFinished);
    connect(itemHandler_, &ItemHandler::uninstallStarted, this, &WebSocketServer::itemHandlerUninstallStarted);
    connect(itemHandler_, &ItemHandler::uninstallFinished, this, &WebSocketServer::itemHandlerUninstallFinished);

    connect(updateHandler_, &UpdateHandler::checkAllStarted, this, &WebSocketServer::updateHandlerCheckAllStarted);
    connect(updateHandler_, &UpdateHandler::checkAllFinished, this, &WebSocketServer::updateHandlerCheckAllFinished);
    connect(updateHandler_, &UpdateHandler::updateStarted, this, &WebSocketServer::updateHandlerUpdateStarted);
    connect(updateHandler_, &UpdateHandler::updateFinished, this, &WebSocketServer::updateHandlerUpdateFinished);
    connect(updateHandler_, &UpdateHandler::updateProgress, this, &WebSocketServer::updateHandlerUpdateProgress);
}

WebSocketServer::~WebSocketServer()
{
    stop();
    wsServer_->deleteLater();
}

bool WebSocketServer::start()
{
    if (wsServer_->listen(QHostAddress::Any, serverPort_)) {
        auto application = configHandler_->getUsrConfigApplication();
        application["websocket_url"] = serverUrl().toString();
        configHandler_->setUsrConfigApplication(application);

        emit started();
        return true;
    }
    return false;
}

void WebSocketServer::stop()
{
    auto application = configHandler_->getUsrConfigApplication();
    application["websocket_url"] = QString("");
    configHandler_->setUsrConfigApplication(application);

    wsServer_->close();
}

bool WebSocketServer::isError() const
{
    if (wsServer_->error() != QWebSocketProtocol::CloseCodeNormal) {
        return true;
    }
    return false;
}

QString WebSocketServer::errorString() const
{
    return wsServer_->errorString();
}

QUrl WebSocketServer::serverUrl() const
{
    return wsServer_->serverUrl();
}

void WebSocketServer::wsNewConnection()
{
    auto *wsClient = wsServer_->nextPendingConnection();

    connect(wsClient, &QWebSocket::disconnected, this, &WebSocketServer::wsDisconnected);
    connect(wsClient, &QWebSocket::textMessageReceived, this, &WebSocketServer::wsTextMessageReceived);
    connect(wsClient, &QWebSocket::binaryMessageReceived, this, &WebSocketServer::wsBinaryMessageReceived);

    wsClients_ << wsClient;
}

void WebSocketServer::wsDisconnected()
{
    auto *wsClient = qobject_cast<QWebSocket *>(sender());
    if (wsClient) {
        wsClients_.removeAll(wsClient);
        wsClient->deleteLater();
    }
}

void WebSocketServer::wsTextMessageReceived(const QString &message)
{
    auto *wsClient = qobject_cast<QWebSocket *>(sender());
    if (wsClient) {
        Qtil::Json json(message.toUtf8());
        if (json.isObject()) {
            auto object = json.toObject();
            receiveMessage(object["id"].toString(), object["func"].toString(), object["data"].toArray());
        }
    }
}

void WebSocketServer::wsBinaryMessageReceived(const QByteArray &message)
{
    auto *wsClient = qobject_cast<QWebSocket *>(sender());
    if (wsClient) {
        Qtil::Json json(message);
        if (json.isObject()) {
            auto object = json.toObject();
            receiveMessage(object["id"].toString(), object["func"].toString(), object["data"].toArray());
        }
    }
}

void WebSocketServer::itemHandlerMetadataSetChanged()
{
    QJsonArray data;
    sendMessage("", "ItemHandler::metadataSetChanged", data);
}

void WebSocketServer::itemHandlerDownloadStarted(QJsonObject result)
{
    QJsonArray data;
    data.append(result);
    sendMessage("", "ItemHandler::downloadStarted", data);
}

void WebSocketServer::itemHandlerDownloadFinished(QJsonObject result)
{
    QJsonArray data;
    data.append(result);
    sendMessage("", "ItemHandler::downloadFinished", data);
}

void WebSocketServer::itemHandlerDownloadProgress(QString id, qint64 bytesReceived, qint64 bytesTotal)
{
    QJsonArray data;
    data.append(id);
    data.append(bytesReceived);
    data.append(bytesTotal);
    sendMessage("", "ItemHandler::downloadProgress", data);
}

void WebSocketServer::itemHandlerSaveStarted(QJsonObject result)
{
    QJsonArray data;
    data.append(result);
    sendMessage("", "ItemHandler::saveStarted", data);
}

void WebSocketServer::itemHandlerSaveFinished(QJsonObject result)
{
    QJsonArray data;
    data.append(result);
    sendMessage("", "ItemHandler::saveFinished", data);
}

void WebSocketServer::itemHandlerInstallStarted(QJsonObject result)
{
    QJsonArray data;
    data.append(result);
    sendMessage("", "ItemHandler::installStarted", data);
}

void WebSocketServer::itemHandlerInstallFinished(QJsonObject result)
{
    QJsonArray data;
    data.append(result);
    sendMessage("", "ItemHandler::installFinished", data);
}

void WebSocketServer::itemHandlerUninstallStarted(QJsonObject result)
{
    QJsonArray data;
    data.append(result);
    sendMessage("", "ItemHandler::uninstallStarted", data);
}

void WebSocketServer::itemHandlerUninstallFinished(QJsonObject result)
{
    QJsonArray data;
    data.append(result);
    sendMessage("", "ItemHandler::uninstallFinished", data);
}

void WebSocketServer::updateHandlerCheckAllStarted(bool status)
{
    QJsonArray data;
    data.append(status);
    sendMessage("", "UpdateHandler::checkAllStarted", data);
}

void WebSocketServer::updateHandlerCheckAllFinished(bool status)
{
    QJsonArray data;
    data.append(status);
    sendMessage("", "UpdateHandler::checkAllFinished", data);
}

void WebSocketServer::updateHandlerUpdateStarted(QString itemKey, bool status)
{
    QJsonArray data;
    data.append(itemKey);
    data.append(status);
    sendMessage("", "UpdateHandler::updateStarted", data);
}

void WebSocketServer::updateHandlerUpdateFinished(QString itemKey, bool status)
{
    QJsonArray data;
    data.append(itemKey);
    data.append(status);
    sendMessage("", "UpdateHandler::updateFinished", data);
}

void WebSocketServer::updateHandlerUpdateProgress(QString itemKey, double progress)
{
    QJsonArray data;
    data.append(itemKey);
    data.append(progress);
    sendMessage("", "UpdateHandler::updateProgress", data);
}

void WebSocketServer::receiveMessage(const QString &id, const QString &func, const QJsonArray &data)
{
    /* message object format
    {
        "id": "example",
        "func": "functionName",
        "data": ["value", 2, true]
    }
    */

    QJsonArray resultData;

    // WebSocketServer
    if (func == "WebSocketServer::stop") {
        stop();
    }
    else if (func == "WebSocketServer::isError") {
        resultData.append(isError());
    }
    else if (func == "WebSocketServer::errorString") {
        resultData.append(errorString());
    }
    else if (func == "WebSocketServer::serverUrl") {
        resultData.append(serverUrl().toString());
    }
    // ConfigHandler
    else if (func == "ConfigHandler::getAppConfigApplication") {
        resultData.append(configHandler_->getAppConfigApplication());
    }
    else if (func == "ConfigHandler::getAppConfigInstallTypes") {
        resultData.append(configHandler_->getAppConfigInstallTypes());
    }
    else if (func == "ConfigHandler::getUsrConfigApplication") {
        resultData.append(configHandler_->getUsrConfigApplication());
    }
    else if (func == "ConfigHandler::setUsrConfigApplication") {
        resultData.append(configHandler_->setUsrConfigApplication(data.at(0).toObject()));
    }
    else if (func == "ConfigHandler::getUsrConfigProviders") {
        resultData.append(configHandler_->getUsrConfigProviders());
    }
    else if (func == "ConfigHandler::setUsrConfigProviders") {
        resultData.append(configHandler_->setUsrConfigProviders(data.at(0).toObject()));
    }
    else if (func == "ConfigHandler::getUsrConfigCategories") {
        resultData.append(configHandler_->getUsrConfigCategories());
    }
    else if (func == "ConfigHandler::setUsrConfigCategories") {
        resultData.append(configHandler_->setUsrConfigCategories(data.at(0).toObject()));
    }
    else if (func == "ConfigHandler::getUsrConfigInstalledItems") {
        resultData.append(configHandler_->getUsrConfigInstalledItems());
    }
    else if (func == "ConfigHandler::setUsrConfigInstalledItems") {
        resultData.append(configHandler_->setUsrConfigInstalledItems(data.at(0).toObject()));
    }
    else if (func == "ConfigHandler::getUsrConfigUpdateAvailableItems") {
        resultData.append(configHandler_->getUsrConfigUpdateAvailableItems());
    }
    else if (func == "ConfigHandler::setUsrConfigUpdateAvailableItems") {
        resultData.append(configHandler_->setUsrConfigUpdateAvailableItems(data.at(0).toObject()));
    }
    else if (func == "ConfigHandler::setUsrConfigProvidersProvider") {
        resultData.append(configHandler_->setUsrConfigProvidersProvider(data.at(0).toString(), data.at(1).toObject()));
    }
    else if (func == "ConfigHandler::removeUsrConfigProvidersProvider") {
        resultData.append(configHandler_->removeUsrConfigProvidersProvider(data.at(0).toString()));
    }
    else if (func == "ConfigHandler::setUsrConfigCategoriesProvider") {
        resultData.append(configHandler_->setUsrConfigCategoriesProvider(data.at(0).toString(), data.at(1).toObject()));
    }
    else if (func == "ConfigHandler::removeUsrConfigCategoriesProvider") {
        resultData.append(configHandler_->removeUsrConfigCategoriesProvider(data.at(0).toString()));
    }
    else if (func == "ConfigHandler::setUsrConfigCategoriesInstallType") {
        resultData.append(configHandler_->setUsrConfigCategoriesInstallType(data.at(0).toString(), data.at(1).toString(), data.at(2).toString()));
    }
    else if (func == "ConfigHandler::setUsrConfigInstalledItemsItem") {
        resultData.append(configHandler_->setUsrConfigInstalledItemsItem(data.at(0).toString(), data.at(1).toObject()));
    }
    else if (func == "ConfigHandler::removeUsrConfigInstalledItemsItem") {
        resultData.append(configHandler_->removeUsrConfigInstalledItemsItem(data.at(0).toString()));
    }
    else if (func == "ConfigHandler::setUsrConfigUpdateAvailableItemsItem") {
        resultData.append(configHandler_->setUsrConfigUpdateAvailableItemsItem(data.at(0).toString(), data.at(1).toObject()));
    }
    else if (func == "ConfigHandler::removeUsrConfigUpdateAvailableItemsItem") {
        resultData.append(configHandler_->removeUsrConfigUpdateAvailableItemsItem(data.at(0).toString()));
    }
    // SystemHandler
    else if (func == "SystemHandler::isUnix") {
        resultData.append(systemHandler_->isUnix());
    }
    else if (func == "SystemHandler::isMobileDevice") {
        resultData.append(systemHandler_->isMobileDevice());
    }
    else if (func == "SystemHandler::openUrl") {
        resultData.append(systemHandler_->openUrl(data.at(0).toString()));
    }
    // OcsApiHandler
    else if (func == "OcsApiHandler::addProviders") {
        resultData.append(ocsApiHandler_->addProviders(data.at(0).toString()));
    }
    else if (func == "OcsApiHandler::removeProvider") {
        resultData.append(ocsApiHandler_->removeProvider(data.at(0).toString()));
    }
    else if (func == "OcsApiHandler::updateAllCategories") {
        resultData.append(ocsApiHandler_->updateAllCategories(data.at(0).toBool()));
    }
    else if (func == "OcsApiHandler::updateCategories") {
        resultData.append(ocsApiHandler_->updateCategories(data.at(0).toString(), data.at(1).toBool()));
    }
    else if (func == "OcsApiHandler::getContents") {
        resultData.append(ocsApiHandler_->getContents(data.at(0).toString(), data.at(1).toString(),
                                                      data.at(2).toString(), data.at(3).toString(),
                                                      data.at(4).toString(), data.at(5).toString(), data.at(6).toInt(), data.at(7).toInt()));
    }
    else if (func == "OcsApiHandler::getContent") {
        resultData.append(ocsApiHandler_->getContent(data.at(0).toString(), data.at(1).toString()));
    }
    // ItemHandler
    else if (func == "ItemHandler::metadataSet") {
        resultData.append(itemHandler_->metadataSet());
    }
    else if (func == "ItemHandler::getItem") {
        itemHandler_->getItem(data.at(0).toString(), data.at(1).toString(), data.at(2).toString(), data.at(3).toString(),
                              data.at(4).toString(), data.at(5).toString());
    }
    else if (func == "ItemHandler::getItemByOcsUrl") {
        itemHandler_->getItemByOcsUrl(data.at(0).toString(), data.at(1).toString(), data.at(2).toString());
    }
    else if (func == "ItemHandler::uninstall") {
        itemHandler_->uninstall(data.at(0).toString());
    }
    // UpdateHandler
    else if (func == "UpdateHandler::checkAll") {
        updateHandler_->checkAll();
    }
    else if (func == "UpdateHandler::update") {
        updateHandler_->update(data.at(0).toString());
    }
    // DesktopThemeHandler
    else if (func == "DesktopThemeHandler::desktopEnvironment") {
        resultData.append(desktopThemeHandler_->desktopEnvironment());
    }
    else if (func == "DesktopThemeHandler::isApplicableType") {
        resultData.append(desktopThemeHandler_->isApplicableType(data.at(0).toString()));
    }
    else if (func == "DesktopThemeHandler::applyTheme") {
        resultData.append(desktopThemeHandler_->applyTheme(data.at(0).toString(), data.at(1).toString()));
    }
    // Not supported
    else {
        return;
    }

    sendMessage(id, func, resultData);
}

void WebSocketServer::sendMessage(const QString &id, const QString &func, const QJsonArray &data)
{
    /* message object format
    {
        "id": "example",
        "func": "functionName",
        "data": ["value", 2, true]
    }
    */

    QJsonObject object;
    object["id"] = id;
    object["func"] = func;
    object["data"] = data;

    auto binaryMessage = Qtil::Json(object).toJson();
    auto textMessage = QString::fromUtf8(binaryMessage);

    for (auto *wsClient : wsClients_) {
        wsClient->sendTextMessage(textMessage);
        //wsClient->sendBinaryMessage(binaryMessage);
    }
}
