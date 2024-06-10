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

class ChapterMarkerDock : public QWidget {
	Q_OBJECT

public:
	ChapterMarkerDock(QWidget *parent = nullptr);
	~ChapterMarkerDock();

	QString getChapterName() const;
	void updateCurrentChapterLabel(const QString &chapterName);
	void setNoRecordingActive();
	void showFeedbackMessage(const QString &message, bool isError);

private slots:
	void onSaveClicked();
	void onSettingsClicked();
	void onSceneChanged();
	void onHistoryItemSelected();

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
	bool chapterOnSceneChangeEnabled;

	QListWidget *chapterHistoryList;
};

#endif // CHAPTER_MARKER_DOCK_HPP
