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

	QString getChapterName() const;
	void setExportTextFilePath(const QString &filePath);
	void setExportXMLFilePath(const QString &filePath);
	void writeChapterToXMLFile(const QString &chapterName,
				   const QString &timestamp,
				   const QString &chapterSource);

	QString getDefaultChapterName() const { return defaultChapterName; }

	QString getCurrentRecordingTime() const;
	void updateCurrentChapterLabel(const QString &chapterName);
	void createExportFiles();
	void showFeedbackMessage(const QString &message, bool isError);
	void clearPreviousChaptersGroup();
	void writeChapterToTextFile(const QString &chapterName,
				    const QString &timestamp,
				    const QString &chapterSource);
	void addChapterMarker(const QString &chapterName,
			      const QString &chapterSource);
	bool exportChaptersToTextEnabled;
	bool exportChaptersToXMLEnabled;
	bool exportChaptersToFileEnabled;
	QString exportTextFilePath;
	QString exportXMLFilePath;
	QString defaultChapterName;
	QStringList ignoredScenes;
	bool chapterOnSceneChangeEnabled;
	bool showPreviousChaptersEnabled;
	bool addChapterSourceEnabled;
	int chapterCount;
	void applySettings(obs_data_t *settings);

	void setAnnotationDock(AnnotationDock *dock);
	void writeAnnotationToFiles(const QString &chapterName,
				    const QString &timestamp,
				    const QString &chapterSource);
	void applyThemeIDToButton(QPushButton *button, const QString &themeID);
	QDialog *settingsDialog;

public slots:
	void onAddChapterMarkerButton();
	void onSettingsClicked();
	void
	onAnnotationClicked(bool startup); // New slot for annotation button
	void onSceneChanged();
	void onRecordingStopped();
	void onPreviousChapterSelected();
	void onPreviousChapterDoubleClicked(QListWidgetItem *item);
	void saveSettingsAndCloseDialog();
	void onSetIgnoredScenesClicked();

private:
	void setupMainDockUI();
	void setupMainDockCurrentChapterLayout(QVBoxLayout *mainLayout);
	void setupMainDockChapterInput(QVBoxLayout *mainLayout);
	void setupMainDockSaveButtonLayout(QVBoxLayout *mainLayout);
	void setupMainDockFeedbackLabel(QVBoxLayout *mainLayout);
	void setupMainDockPreviousChaptersGroup(QVBoxLayout *mainLayout);
	void setupConnections();
	void setupOBSCallbacks();
	void initialiseMainDockUI();
	QDialog *createSettingsUI();
	void setupSettingsGeneralGroup(QVBoxLayout *mainLayout);
	void setupSettingsAutoChapterGroup(QVBoxLayout *mainLayout);
	void initialiseSettingsDialog();
	void populateIgnoredScenesListWidget();
	void updatePreviousChaptersVisibility(bool visible);
	QCheckBox *exportChaptersToFileCheckbox;
	QCheckBox *exportChaptersToTextCheckbox;
	QCheckBox *exportChaptersToXMLCheckbox;
	QGroupBox *exportSettingsGroup;
	QCheckBox *insertChapterMarkersCheckbox;
	void setupSettingsExportGroup(QVBoxLayout *mainLayout);
	void onExportChaptersToFileToggled(bool checked);
	void onChapterOnSceneChangeToggled(bool checked);
	QDialog *createIgnoredScenesUI();

	QDialog *ignoredScenesDialog;
	QGroupBox *sceneChangeSettingsGroup;
	QLineEdit *chapterNameInput;

	QPushButton *saveButton;
	QPushButton *settingsButton;
	QPushButton *setIgnoredScenesButton;
	QPushButton *annotationButton; // New button for annotation dock
	QLabel *currentChapterTextLabel;
	QLabel *currentChapterNameLabel;
	QLabel *feedbackLabel;
	QTimer feedbackTimer;
	QListWidget *previousChaptersList;
	QGroupBox *previousChaptersGroup;

	QLineEdit *defaultChapterNameEdit;
	QCheckBox *showChapterHistoryCheckbox;
	QCheckBox *addChapterSourceCheckbox;
	QCheckBox *chapterOnSceneChangeCheckbox;
	QListWidget *ignoredScenesListWidget;
	QGroupBox *ignoredScenesGroup;

	QStringList chapters;
	QStringList timestamps;

	QHBoxLayout *textCheckboxLayout;
	QHBoxLayout *xmlCheckboxLayout;
	QVBoxLayout *exportSettingsLayout;
	AnnotationDock *annotationDock; // Pointer to the annotation dock
};

#endif // CHAPTER_MARKER_DOCK_HPP
