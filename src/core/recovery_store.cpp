/**
 * @file recovery_store.cpp
 * @brief RecoveryStore implementation.
 */
#include "recovery_store.h"
#include "project_serializer.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QStandardPaths>
#include <QUuid>
#include <algorithm>

namespace {
bool isValidId(const QString &id) {
  // Ids become file names; accept only what createSession() produces.
  return !id.isEmpty() && !QUuid::fromString(id).isNull() &&
         !id.contains(QLatin1Char('/')) && !id.contains(QLatin1Char('\\'));
}
} // namespace

RecoveryStore::RecoveryStore(const QString &directory) : directory_(directory) {
  if (directory_.isEmpty())
    directory_ =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
        QStringLiteral("/recovery");
  QDir().mkpath(directory_);
}

RecoveryStore::~RecoveryStore() = default;

QString RecoveryStore::snapshotPath(const QString &documentId) const {
  return directory_ + QLatin1Char('/') + documentId + QStringLiteral(".fspd");
}

QString RecoveryStore::metadataPath(const QString &documentId) const {
  return directory_ + QLatin1Char('/') + documentId + QStringLiteral(".json");
}

QString RecoveryStore::lockPath(const QString &documentId) const {
  return directory_ + QLatin1Char('/') + documentId + QStringLiteral(".lock");
}

QString RecoveryStore::createSession() {
  const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  adoptSession(id);
  return id;
}

bool RecoveryStore::adoptSession(const QString &documentId) {
  if (!isValidId(documentId))
    return false;
  if (locks_.contains(documentId))
    return true;
  auto lock = std::make_shared<QLockFile>(lockPath(documentId));
  lock->setStaleLockTime(0); // only a dead owner makes a lock stale
  if (!lock->tryLock(0)) {
    // A crashed owner leaves a stale lock behind; take it over.
    if (!lock->removeStaleLockFile() || !lock->tryLock(0))
      return false;
  }
  locks_.insert(documentId, lock);
  return true;
}

void RecoveryStore::releaseSession(const QString &documentId) {
  auto lock = locks_.take(documentId);
  if (lock)
    lock->unlock();
}

bool RecoveryStore::isLiveElsewhere(const QString &documentId) const {
  if (locks_.contains(documentId))
    return true; // our own live session
  QLockFile probe(lockPath(documentId));
  probe.setStaleLockTime(0);
  if (probe.tryLock(0)) {
    probe.unlock();
    return false;
  }
  // A running owner keeps an OS lock on the file, so removing it only
  // succeeds once that owner is gone (crashed).
  return !probe.removeStaleLockFile();
}

bool RecoveryStore::writeSnapshot(const QString &documentId,
                                  const QByteArray &projectData,
                                  const RecoverySnapshot &info,
                                  QString *errorMessage) {
  if (!isValidId(documentId) || projectData.isEmpty()) {
    if (errorMessage)
      *errorMessage = QStringLiteral("Invalid recovery snapshot.");
    return false;
  }
  QDir().mkpath(directory_);
  ProjectSerializer::SaveStatus status;
  if (!ProjectSerializer::writeFileAtomically(snapshotPath(documentId),
                                              projectData, &status)) {
    if (errorMessage)
      *errorMessage = status.message;
    return false;
  }
  QJsonObject meta;
  meta["documentId"] = documentId;
  meta["originalPath"] = info.originalPath;
  meta["displayName"] = info.displayName;
  meta["savedAt"] =
      (info.savedAt.isValid() ? info.savedAt : QDateTime::currentDateTimeUtc())
          .toString(Qt::ISODateWithMs);
  meta["layerCount"] = info.layerCount;
  meta["itemCount"] = info.itemCount;
  meta["formatVersion"] = ProjectSerializer::FORMAT_VERSION;
  if (!ProjectSerializer::writeFileAtomically(
          metadataPath(documentId),
          QJsonDocument(meta).toJson(QJsonDocument::Compact), &status)) {
    if (errorMessage)
      *errorMessage = status.message;
    return false;
  }
  return true;
}

QList<RecoverySnapshot> RecoveryStore::recoverableSnapshots() const {
  QList<RecoverySnapshot> result;
  const QDir dir(directory_);
  const QStringList metas =
      dir.entryList({QStringLiteral("*.json")}, QDir::Files);
  for (const QString &metaName : metas) {
    const QString id = QFileInfo(metaName).completeBaseName();
    if (!isValidId(id) || isLiveElsewhere(id))
      continue;
    QFile metaFile(metadataPath(id));
    if (!metaFile.open(QIODevice::ReadOnly))
      continue;
    const QJsonObject meta =
        QJsonDocument::fromJson(metaFile.readAll()).object();
    const QString path = snapshotPath(id);
    // The metadata is only written after its snapshot committed, but check
    // that the snapshot really is a readable project anyway.
    QFile snap(path);
    if (!snap.open(QIODevice::ReadOnly))
      continue;
    const QJsonDocument doc = QJsonDocument::fromJson(snap.readAll());
    if (!doc.isObject() || doc.object()["formatVersion"].toInt(0) < 1)
      continue;
    RecoverySnapshot s;
    s.documentId = id;
    s.snapshotPath = path;
    s.originalPath = meta["originalPath"].toString();
    s.displayName = meta["displayName"].toString();
    s.savedAt =
        QDateTime::fromString(meta["savedAt"].toString(), Qt::ISODateWithMs);
    if (!s.savedAt.isValid())
      s.savedAt = QFileInfo(path).lastModified();
    s.layerCount = meta["layerCount"].toInt();
    s.itemCount = meta["itemCount"].toInt();
    result.append(s);
  }
  std::sort(result.begin(), result.end(),
            [](const RecoverySnapshot &a, const RecoverySnapshot &b) {
              return a.savedAt > b.savedAt;
            });
  return result;
}

bool RecoveryStore::hasSnapshot(const QString &documentId) const {
  return isValidId(documentId) && QFileInfo::exists(snapshotPath(documentId));
}

void RecoveryStore::discard(const QString &documentId) {
  if (!isValidId(documentId))
    return;
  // Metadata first: without it the snapshot is never offered again, even
  // if deleting the (larger) project file fails.
  QFile::remove(metadataPath(documentId));
  QFile::remove(snapshotPath(documentId));
}
