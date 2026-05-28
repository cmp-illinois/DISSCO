#ifndef UPDATER_HPP
#define UPDATER_HPP

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVersionNumber>

class QNetworkAccessManager;
class QNetworkReply;
class QWidget;

/**
 * @brief In-app "Check for Updates" client backed by the GitHub Releases API.
 *
 * GETs /releases/latest, parses the tag as a QVersionNumber, and compares
 * it against the build's compile-time DISSCO_VERSION. If newer, offers to
 * download the matching platform asset (DMG / AppImage / EXE) and hands it
 * to the OS to install.
 *
 * Triggered manually from the Help menu, and optionally on app startup
 * once per 24h.
 */
class Updater : public QObject {
    Q_OBJECT
    public:
        /// Whether the check is user-initiated (chatty) or background (silent).
        enum class Trigger { Manual, Auto };

        /// Build the updater, parented to the window that owns its dialogs.
        explicit Updater(QWidget *uiParent);
        ~Updater() override;

        // QSettings keys (flat camelCase to match the rest of the app).
        static constexpr const char *kAutoCheckKey = "updaterAutoCheck";
        static constexpr const char *kLastCheckUtcKey = "updaterLastCheckUtc";
        static constexpr const char *kSkipVersionKey = "updaterSkipVersion";

        /// Build's compile-time version (from DISSCOVersions.txt).
        static QVersionNumber currentVersion();
        /// Build's git HEAD commit SHA, or "unknown".
        static QString currentCommit();
        /// "owner/repo" slug for the GitHub repo being polled.
        static QString gitHubRepoSlug();

        /// Run a release check. Safe to call from the GUI thread.
        void checkForUpdates(Trigger trigger);

        /// True iff auto-check is on AND it has been >= 24h since the
        /// last check. MainWindow polls this on startup.
        static bool shouldAutoCheckNow();

    private:
        struct ReleaseInfo {
            QVersionNumber version;
            QString tagName;
            QString releaseNotes;
            QUrl htmlUrl;
            QUrl assetUrl;
            QString assetName;
            qint64 assetSize = -1;
        };

        void onReleaseReply(QNetworkReply *reply);
        void presentReleaseDialog(const ReleaseInfo &info);
        void downloadAndInstall(const ReleaseInfo &info);
        void launchInstaller(const QString &localPath);
        void reportError(const QString &message);

        QNetworkAccessManager *net_;
        QWidget *uiParent_;
        Trigger trigger_ = Trigger::Manual;
};

#endif  // UPDATER_HPP
