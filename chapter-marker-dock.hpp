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
#include <obs-frontend-api.h>

// Forward declaration of AnnotationDock
class AnnotationDock;

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
	bool exportChaptersEnabled;
	QString chapterFilePath;
	QString defaultChapterName;
	QStringList ignoredScenes;
	bool chapterOnSceneChangeEnabled;
	bool showChapterHistoryEnabled;
	bool addChapterSourceEnabled;
	int chapterCount;
	void applySettings(obs_data_t *settings);

	void setAnnotationDock(AnnotationDock *dock);
	void writeAnnotationToFile(const QString &chapterName,
				   const QString &timestamp,
				   const QString &chapterSource);

public slots:
	void onAddChapterMarkerButton();
	void onSettingsClicked();
	void onAnnotationClicked(); // New slot for annotation button
	void onSceneChanged();
	void onRecordingStopped();
	void onHistoryItemSelected();
	void onHistoryItemDoubleClicked(QListWidgetItem *item);
	void saveSettingsAndCloseDialog();

private:
	void setupUI();
	void setupCurrentChapterLayout(QVBoxLayout *mainLayout);
	void setupChapterNameEdit(QVBoxLayout *mainLayout);
	void setupSaveButtonLayout(QVBoxLayout *mainLayout);
	void setupFeedbackLabel(QVBoxLayout *mainLayout);
	void setupPreviousChaptersGroup(QVBoxLayout *mainLayout);
	void setupConnections();
	void setupOBSCallbacks();
	void initialiseUIStates();
	QDialog *createSettingsDialog();
	void setupGeneralSettingsGroup(QVBoxLayout *mainLayout);
	void setupSceneChangeSettingsGroup(QVBoxLayout *mainLayout);
	void initialiseSettingsDialog();
	void populateIgnoredScenesListWidget();
	void updatePreviousChaptersVisibility(bool visible);

	QLineEdit *chapterNameEdit;
	QPushButton *saveButton;
	QPushButton *settingsButton;
	QPushButton *annotationButton; // New button for annotation dock
	QLabel *currentChapterTextLabel;
	QLabel *currentChapterNameLabel;
	QLabel *feedbackLabel;
	QTimer feedbackTimer;
	QListWidget *chapterHistoryList;
	QGroupBox *previousChaptersGroup;

	QDialog *settingsDialog;
	QLineEdit *defaultChapterNameEdit;
	QCheckBox *showChapterHistoryCheckbox;
	QCheckBox *exportChaptersCheckbox;
	QCheckBox *addChapterSourceCheckbox;
	QCheckBox *chapterOnSceneChangeCheckbox;
	QListWidget *ignoredScenesListWidget;
	QGroupBox *ignoredScenesGroup;

	QStringList chapters;
	QStringList timestamps;

	AnnotationDock *annotationDock; // Pointer to the annotation dock
};

#endif // CHAPTER_MARKER_DOCK_HPP
