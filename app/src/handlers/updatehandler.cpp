#include "updatehandler.h"

#include <QStringList>
#include <QJsonValue>
#include <QJsonArray>
#include <QDateTime>

#include "qtil_file.h"

#include "handlers/confighandler.h"

#ifdef QTIL_OS_UNIX
#include "updaters/appimageupdater.h"
#endif

UpdateHandler::UpdateHandler(ConfigHandler *configHandler, QObject *parent)
    : QObject(parent), configHandler_(configHandler)
{}

void UpdateHandler::checkAll()
{
    // Resets data
    QJsonObject updateAvailableItems;
    configHandler_->setUsrConfigUpdateAvailableItems(updateAvailableItems);

    auto installedItems = configHandler_->getUsrConfigInstalledItems();

    if (installedItems.isEmpty()) {
        emit checkAllStarted(false);
        return;
    }

    emit checkAllStarted(true);

    for (const auto &itemKey : installedItems.keys()) {
        auto installedItem = installedItems[itemKey].toObject();
        auto filename = installedItem["filename"].toString();
        auto installType = installedItem["install_type"].toString();

        QString filePath = "";
#ifdef QTIL_OS_UNIX
        filePath = configHandler_->getAppConfigInstallTypes()[installType].toObject()["destination"].toString() + "/" + filename;
#else
        filePath = configHandler_->getAppConfigInstallTypes()[installType].toObject()["generic_destination"].toString() + "/" + filename;
#endif

        QString updateMethod = "";

        if (installType == "bin") {
#ifdef QTIL_OS_UNIX
            if (filePath.endsWith(".appimage", Qt::CaseInsensitive)) {
                if (AppImageUpdater(itemKey, filePath).checkForChanges()) {
                    updateMethod = "appimageupdate";
                }
                //else if (OcsFileUpdater(url).checkFile()) {
                //    updateMethod = "appimageupdate_ocs";
                //}
            }
#endif
        }

        if (!updateMethod.isEmpty()) {
            QJsonObject updateAvailableItem;
            updateAvailableItem["installed_item"] = itemKey;
            updateAvailableItem["update_method"] = updateMethod;
            // Use installed item's key as unique key to the update available item
            configHandler_->setUsrConfigUpdateAvailableItemsItem(itemKey, updateAvailableItem);
        }
    }

    auto application = configHandler_->getUsrConfigApplication();
    application["update_checked_at"] = QDateTime::currentMSecsSinceEpoch();
    configHandler_->setUsrConfigApplication(application);

    emit checkAllFinished(true);
}

void UpdateHandler::update(const QString &itemKey)
{
    auto updateAvailableItems = configHandler_->getUsrConfigUpdateAvailableItems();

    if (!updateAvailableItems.contains(itemKey)) {
        emit updateStarted(itemKey, false);
        return;
    }

    auto updateMethod = updateAvailableItems[itemKey].toObject()["update_method"].toString();

#ifdef QTIL_OS_UNIX
    if (updateMethod == "appimageupdate") {
        updateAppImage(itemKey);
    }
    //else if (updateMethod == "appimageupdate_ocs") {
    //    updateAppImageOcs(itemKey);
    //}
#endif
}

#ifdef QTIL_OS_UNIX
void UpdateHandler::appImageUpdaterFinished(AppImageUpdater *updater)
{
    auto itemKey = updater->id();

    auto metadata = metadataSet_[itemKey].toObject();
    metadataSet_.remove(itemKey);

    if (!updater->isFinishedWithNoError()) {
        emit updateFinished(itemKey, false);
        updater->deleteLater();
        return;
    }

    auto installedItemKey = metadata["installed_item_key"].toString();
    auto installedItem = metadata["installed_item_obj"].toObject();
    auto newFilename = metadata["new_filename"].toString();
    auto filename = installedItem["filename"].toString();

    installedItem["filename"] = newFilename;
    QJsonArray files;
    files.append(QJsonValue(newFilename));
    installedItem["files"] = files;
    installedItem["installed_at"] = QDateTime::currentMSecsSinceEpoch();

    configHandler_->setUsrConfigInstalledItemsItem(installedItemKey, installedItem);

    if (newFilename != filename) {
        Qtil::File(updater->path()).remove();
    }

    configHandler_->removeUsrConfigUpdateAvailableItemsItem(itemKey);

    emit updateFinished(itemKey, true);
    updater->deleteLater();
}
#endif

#ifdef QTIL_OS_UNIX
void UpdateHandler::updateAppImage(const QString &itemKey)
{
    auto updateAvailableItem = configHandler_->getUsrConfigUpdateAvailableItems()[itemKey].toObject();
    auto installedItemKey = updateAvailableItem["installed_item"].toString();
    auto installedItems = configHandler_->getUsrConfigInstalledItems();

    if (!installedItems.contains(installedItemKey)) {
        emit updateStarted(itemKey, false);
        return;
    }

    auto installedItem = installedItems[installedItemKey].toObject();
    auto filename = installedItem["filename"].toString();
    auto installType = installedItem["install_type"].toString();
    auto filePath = configHandler_->getAppConfigInstallTypes()[installType].toObject()["destination"].toString() + "/" + filename;

    auto *updater = new AppImageUpdater(itemKey, filePath, this);
    connect(updater, &AppImageUpdater::updateProgress, this, &UpdateHandler::updateProgress);
    connect(updater, &AppImageUpdater::finished, this, &UpdateHandler::appImageUpdaterFinished);

    QJsonObject metadata;
    metadata["installed_item_key"] = installedItemKey;
    metadata["installed_item_obj"] = installedItem;
    metadata["new_filename"] = filename;

    QString assembledNewFilename = "";
    QString rawNewFilename = "";

    auto updateInformation = updater->describeAppImage();
    for (const auto &info : updateInformation.split("\n")) {
        if (info.endsWith(".zsync", Qt::CaseInsensitive)) {
            if (info.startsWith("Assembled ZSync URL:", Qt::CaseInsensitive)) {
                assembledNewFilename = info.split("/").last().replace(".zsync", "", Qt::CaseInsensitive);
            }
            else if (info.startsWith("Raw update information:", Qt::CaseInsensitive)) {
                rawNewFilename = info.split("|").last().split("/").last().replace(".zsync", "", Qt::CaseInsensitive);
            }
        }
    }

    if (!assembledNewFilename.isEmpty()) {
        metadata["new_filename"] = assembledNewFilename;
    }
    else if (!rawNewFilename.isEmpty() && !rawNewFilename.contains("*")) {
        metadata["new_filename"] = rawNewFilename;
    }

    metadataSet_[itemKey] = metadata;

    emit updateStarted(itemKey, true);
    updater->start();
}
#endif
