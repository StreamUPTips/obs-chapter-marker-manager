#pragma once

#ifndef CHAPTER_MARKER_DOCK_HPP
#define CHAPTER_MARKER_DOCK_HPP

#include <QFrame>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QListWidget>
#include <QCheckBox>
#include <QGroupBox>
#include <QDialog>
#include <QStringList>

class ChapterMarkerDock : public QFrame {
	Q_OBJECT

public:
	ChapterMarkerDock(QWidget *parent = nullptr);
	~ChapterMarkerDock();

	// Public interface
	QString getChapterName() const;
	void setChapterFilePath(const QString &filePath);
	void setAddChapterSourceEnabled(bool enabled);
	bool isAddChapterSourceEnabled() const;
	void setIgnoredScenes(const QStringList &scenes);
	QStringList getIgnoredScenes() const;
	bool isExportChaptersEnabled() const;

	QString getDefaultChapterName() const { return defaultChapterName; }

	QString getCurrentRecordingTime() const;
	void updateCurrentChapterLabel(const QString &chapterName);
	void setNoRecordingActive();
	void showFeedbackMessage(const QString &message, bool isError);
	void clearChapterHistory();
	void writeChapterToFile(const QString &chapterName,
				const QString &timestamp,
				const QString &chapterSource);
	void addChapterMarker(const QString &chapterName,
			      const QString &chapterSource);
	void loadSettings(); // Add this method declaration
	bool exportChaptersEnabled;
	QString chapterFilePath;
	QString defaultChapterName;
	QStringList ignoredScenes;
	bool chapterOnSceneChangeEnabled;
	bool showChapterHistoryEnabled;
	bool addChapterSourceEnabled;

private slots:
	// Event handlers
	void onSaveClicked();
	void onSettingsClicked();
	void onSceneChanged();
	void onRecordingStopped();
	void onHistoryItemSelected();
	void onHistoryItemDoubleClicked(QListWidgetItem *item);
	void onHotkeyTriggered();
	void saveSettingsAndCloseDialog();

private:
	// UI setup functions
	void setupUI();
	void setupCurrentChapterLayout(QVBoxLayout *mainLayout);
	void setupChapterNameEdit(QVBoxLayout *mainLayout);
	void setupSaveButtonLayout(QVBoxLayout *mainLayout);
	void setupFeedbackLabel(QVBoxLayout *mainLayout);
	void setupPreviousChaptersGroup(QVBoxLayout *mainLayout);

	// Signal connections
	void setupConnections();

	// OBS callback
	void setupOBSCallbacks();

	// initialise UI states
	void initialiseUIStates();

	// Settings dialog setup functions
	QDialog *createSettingsDialog();
	void setupGeneralSettingsGroup(QVBoxLayout *mainLayout);
	void setupSceneChangeSettingsGroup(QVBoxLayout *mainLayout);
	void initialiseSettingsDialog();

	// Utility functions
	void populateIgnoredScenesListWidget();
	void updatePreviousChaptersVisibility(bool visible);

	// UI components
	QLineEdit *chapterNameEdit;
	QPushButton *saveButton;
	QPushButton *settingsButton;
	QLabel *currentChapterTextLabel;
	QLabel *currentChapterNameLabel;
	QLabel *feedbackLabel;
	QTimer feedbackTimer;
	QListWidget *chapterHistoryList;
	QGroupBox *previousChaptersGroup;

	// Settings dialog components
	QDialog *settingsDialog;
	QLineEdit *defaultChapterNameEdit;
	QCheckBox *showChapterHistoryCheckbox;
	QCheckBox *exportChaptersCheckbox;
	QCheckBox *addChapterSourceCheckbox;
	QCheckBox *chapterOnSceneChangeCheckbox;
	QListWidget *ignoredScenesListWidget;
	QGroupBox *ignoredScenesGroup;

	// State variables

	int chapterCount;
	QStringList chapters;
	QStringList timestamps;
};

#endif // CHAPTER_MARKER_DOCK_HPP
