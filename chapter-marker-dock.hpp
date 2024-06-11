#ifndef CHAPTER_MARKER_DOCK_HPP
#define CHAPTER_MARKER_DOCK_HPP

#include <QFrame>
#include <QGroupBox>
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
#include <QComboBox>

class ChapterMarkerDock : public QFrame {
	Q_OBJECT

public:
	ChapterMarkerDock(QWidget *parent = nullptr);
	~ChapterMarkerDock();

	QString getChapterName() const;
	void updateCurrentChapterLabel(const QString &chapterName);
	void setNoRecordingActive();
	void showFeedbackMessage(const QString &message, bool isError);
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
	bool exportChaptersEnabled;
	void setIgnoredScenes(const QStringList &scenes);
	QStringList getIgnoredScenes() const;
	void populateIgnoredScenesListWidget();
	void updatePreviousChaptersVisibility(bool visible);
	void onHotkeyTriggered(); // Declare the hotkey-triggered function
	QString getDefaultChapterName() const
	{
		return defaultChapterName;
	} 


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
	QString chapterFilePath;
	QCheckBox *addChapterSourceCheckbox;
	bool addChapterSourceEnabled;
	QComboBox *ignoredScenesComboBox;
	QStringList ignoredScenes;
	void populateIgnoredScenesComboBox();
	QListWidget *ignoredScenesListWidget;

	QGroupBox *previousChaptersGroup; // GroupBox for Previous Chapters
	QGroupBox *ignoredScenesGroup;    // GroupBox for Ignored Scenes

	QListWidget *chapterHistoryList;

	QStringList chapters;   // List of chapters
	QStringList timestamps; // List of timestamps

	QLineEdit *defaultChapterNameEdit;
	QString defaultChapterName;
	int chapterCount; // Add this member variable to keep track of the chapter count
};

#endif // CHAPTER_MARKER_DOCK_HPP
