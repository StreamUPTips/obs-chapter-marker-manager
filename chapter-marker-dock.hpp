#ifndef CHAPTER_MARKER_DOCK_HPP
#define CHAPTER_MARKER_DOCK_HPP

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

class ChapterMarkerDock : public QWidget {
	Q_OBJECT

public:
	ChapterMarkerDock(QWidget *parent = nullptr);
	~ChapterMarkerDock();

	QString getChapterName() const;
	void updateCurrentChapterLabel(const QString &chapterName);
	void setNoRecordingActive();

private slots:
	void onSaveClicked();

private:
	QLineEdit *chapterNameEdit;
	QPushButton *saveButton;
	QLabel *currentChapterTextLabel;
	QLabel *currentChapterNameLabel;
};

#endif // CHAPTER_MARKER_DOCK_HPP
