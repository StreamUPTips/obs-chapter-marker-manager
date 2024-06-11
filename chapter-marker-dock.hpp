#ifndef CHAPTER_MARKER_DOCK_HPP
#define CHAPTER_MARKER_DOCK_HPP

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QListWidget>
#include <QDateTime>

class ChapterMarkerDock : public QWidget {
    Q_OBJECT

public:
    ChapterMarkerDock(QWidget *parent = nullptr);
    ~ChapterMarkerDock();

    QString getChapterName() const;
    void updateCurrentChapterLabel(const QString &chapterName);
    void setNoRecordingActive();
    void showFeedbackMessage(const QString &message, bool isError);
    QListWidget *chapterHistoryList;
    void setChapterFilePath(const QString &filePath);
    void writeChapterToFile(const QString &chapterName,
			    const QString &timestamp,
			    const QString &chapterSource);
    QString getCurrentRecordingTime() const;
    void addChapterMarker(const QString &chapterName,
			  const QString &chapterSource);
    void clearChapterHistory();
    void setAddChapterSourceEnabled(bool enabled);
    bool isAddChapterSourceEnabled() const;

private slots:
    void onSaveClicked();
    void onSettingsClicked();
    void onSceneChanged();
    void onRecordingStopped();
    void onHistoryItemSelected();
    void onHistoryItemDoubleClicked(QListWidgetItem *item);

private:
    QLineEdit *chapterNameEdit;
    QPushButton *saveButton;
    QPushButton *settingsButton;
    QLabel *currentChapterTextLabel;
    QLabel *currentChapterNameLabel;
    QLabel *feedbackLabel;
    QTimer feedbackTimer;

    QDialog *settingsDialog;
    QCheckBox *chapterOnSceneChangeCheckbox;
    QCheckBox *showChapterHistoryCheckbox;
    QCheckBox *exportChaptersCheckbox;
    bool chapterOnSceneChangeEnabled;
    bool showChapterHistoryEnabled;
    bool exportChaptersEnabled;
    QString chapterFilePath;
    QCheckBox *addChapterSourceCheckbox;
    bool addChapterSourceEnabled;

    QLabel *historyLabel;

    QStringList chapters; // List of chapters
    QStringList timestamps; // List of timestamps
};

#endif // CHAPTER_MARKER_DOCK_HPP
